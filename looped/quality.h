// Picture quality: how many pixels the game draws (RESTRUCTURE 6.7).
// Settings → Graphics: Auto, High, Medium, Low. High draws at up to 2 device
// pixels per CSS pixel (the old fixed cap), Medium 1.6, Low 1.3. On Auto the
// controller watches the frames and steps down when the GPU is what keeps the
// frame rate under 60 (frames slow while the game's own CPU work is small),
// and back up when the GPU has room to spare at the sharper level.
// Plain C++ (no browser), so tools/native/quality_test.cpp runs it.
#pragma once

#include <algorithm>

namespace engine {

class QualityController {
 public:
  enum Mode { kAuto = 0, kHigh = 1, kMedium = 2, kLow = 3 };
  static constexpr int kTiers = 3;
  static constexpr double kRatio[kTiers] = {2.0, 1.6, 1.3};  // device pixels per CSS pixel

  // Settings "quality" (0 Auto, 1 High, 2 Medium, 3 Low).
  void setMode(int mode, double nowMs) {
    mode = std::clamp(mode, 0, 3);
    if (mode == mode_) return;
    mode_ = mode;
    if (mode_ != kAuto) tier_ = mode_ - 1;
    resetWindow(nowMs);
    lastChangeMs_ = nowMs;
  }
  int mode() const { return mode_; }
  int tier() const { return tier_; }  // 0 sharpest
  double pixelRatioCap() const { return kRatio[tier_]; }

  // One finished frame: the time since the previous one (what you feel),
  // the game's CPU time, the newest GPU submit-to-done time. `drawing`
  // false (loading, another game has the screen, the page was hidden)
  // pauses watching. Returns true when the tier changed.
  bool frame(double nowMs, float intervalMs, float cpuMs, float gpuMs, bool drawing) {
    if (mode_ != kAuto || !drawing || intervalMs > 100) {  // a hitch or a pause: not a GPU signal
      if (!drawing) resetWindow(nowMs);
      return false;
    }
    if (windowStartMs_ < 0) resetWindow(nowMs);
    ++frames_;
    sumInterval_ += intervalMs;
    sumCpu_ += cpuMs;
    sumGpu_ += gpuMs;
    if (nowMs - windowStartMs_ < kWindowMs) return false;
    const float n = static_cast<float>(frames_);
    const float interval = sumInterval_ / n, cpu = sumCpu_ / n, gpu = sumGpu_ / n;
    resetWindow(nowMs);
    if (nowMs - lastChangeMs_ < kSettleMs) return false;
    // Slow frames while the CPU is idle: the GPU is the limit.
    if (interval > kSlowMs && cpu < kCpuBoundMs && gpu > kGpuBusyMs && tier_ < kTiers - 1) {
      ++tier_;
      if (nowMs - lastUpMs_ < kBounceMs) upBlockedUntilMs_ = nowMs + kUpBlockMs;  // went up, came straight back down
      lastChangeMs_ = nowMs;
      return true;
    }
    // Room to spare: the GPU time scaled to the sharper level's pixels
    // still fits well inside a frame, for a few windows in a row.
    if (tier_ > 0 && nowMs >= upBlockedUntilMs_) {
      const double k = kRatio[tier_ - 1] / kRatio[tier_];
      const bool roomy = interval < kSmoothMs && gpu * k * k < kGpuRoomMs;
      calmWindows_ = roomy ? calmWindows_ + 1 : 0;
      if (calmWindows_ >= kCalmWindows) {
        --tier_;
        calmWindows_ = 0;
        lastChangeMs_ = lastUpMs_ = nowMs;
        return true;
      }
    }
    return false;
  }

  static constexpr double kWindowMs = 2000;    // averages over 2 s
  static constexpr double kSettleMs = 4000;    // no change sooner than 4 s after the last
  static constexpr float kSlowMs = 18.5f;      // average frame slower than this: under 60
  static constexpr float kSmoothMs = 17.5f;
  static constexpr float kCpuBoundMs = 8.0f;   // above this, fewer pixels wouldn't help
  static constexpr float kGpuBusyMs = 12.0f;
  static constexpr float kGpuRoomMs = 11.0f;
  static constexpr int kCalmWindows = 3;        // 6 s of room before stepping up
  static constexpr double kBounceMs = 15000;
  static constexpr double kUpBlockMs = 60000;  // after a bounce, stay down for a minute

 private:
  void resetWindow(double nowMs) {
    windowStartMs_ = nowMs;
    frames_ = 0;
    sumInterval_ = sumCpu_ = sumGpu_ = 0;
  }
  int mode_ = kAuto;
  int tier_ = 0;
  double windowStartMs_ = -1;
  int frames_ = 0;
  float sumInterval_ = 0, sumCpu_ = 0, sumGpu_ = 0;
  double lastChangeMs_ = -1e9, lastUpMs_ = -1e9, upBlockedUntilMs_ = 0;
  int calmWindows_ = 0;
};

}  // namespace engine
