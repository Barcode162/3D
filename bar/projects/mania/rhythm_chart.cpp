#include "bar/projects/mania/rhythm_chart.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>

#include "looped/log.h"

namespace app {
namespace {

constexpr float kPi = 3.14159265358979f;
constexpr int kFrame = 512;            // samples per analysis frame (after downsampling)
constexpr int kHop = 128;              // frame step: ~11.6 ms at 11025 Hz
constexpr float kTargetRate = 11025;   // analysis sample rate
constexpr int kBands = 4;
constexpr float kBandEdges[kBands + 1] = {30, 180, 600, 2200, 5500};  // Hz

// In-place radix-2 FFT (n is a power of two).
void fft(std::vector<std::complex<float>>& a) {
  const size_t n = a.size();
  for (size_t i = 1, j = 0; i < n; ++i) {
    size_t bit = n >> 1;
    for (; j & bit; bit >>= 1) j ^= bit;
    j ^= bit;
    if (i < j) std::swap(a[i], a[j]);
  }
  for (size_t len = 2; len <= n; len <<= 1) {
    const float ang = -2 * kPi / static_cast<float>(len);
    const std::complex<float> step(std::cos(ang), std::sin(ang));
    for (size_t i = 0; i < n; i += len) {
      std::complex<float> w(1, 0);
      for (size_t k = 0; k < len / 2; ++k) {
        const std::complex<float> u = a[i + k], v = a[i + k + len / 2] * w;
        a[i + k] = u + v;
        a[i + k + len / 2] = u - v;
        w *= step;
      }
    }
  }
}

// Value at a quantile (0..1) of a copy of `v`.
float quantile(std::vector<float> v, float q) {
  if (v.empty()) return 0;
  const size_t k = std::min(v.size() - 1, static_cast<size_t>(q * static_cast<float>(v.size())));
  std::nth_element(v.begin(), v.begin() + static_cast<long>(k), v.end());
  return v[k];
}

}  // namespace

Chart makeChart(const float* samples, size_t count, float rate, Difficulty difficulty) {
  Chart chart;
  chart.length = static_cast<double>(count) / rate;
  // 1. Downsample by averaging to about 11 kHz (plenty for finding beats).
  const int factor = std::max(1, static_cast<int>(std::lround(rate / kTargetRate)));
  const float srate = rate / static_cast<float>(factor);
  std::vector<float> x(count / static_cast<size_t>(factor));
  for (size_t i = 0; i < x.size(); ++i) {
    float s = 0;
    for (int k = 0; k < factor; ++k) s += samples[i * static_cast<size_t>(factor) + static_cast<size_t>(k)];
    x[i] = s / static_cast<float>(factor);
  }
  if (x.size() < static_cast<size_t>(kFrame) * 4) return chart;

  // 2. Spectral flux per band: how much new energy appears in each frame.
  const size_t frames = (x.size() - kFrame) / kHop;
  std::vector<float> window(kFrame);
  for (int i = 0; i < kFrame; ++i) window[static_cast<size_t>(i)] = 0.5f - 0.5f * std::cos(2 * kPi * static_cast<float>(i) / kFrame);
  int bandBin[kBands + 1];
  for (int b = 0; b <= kBands; ++b) {
    bandBin[b] = std::clamp(static_cast<int>(kBandEdges[b] / srate * kFrame), 1, kFrame / 2);
  }
  std::vector<float> prev(kFrame / 2, 0), mag(kFrame / 2);
  std::vector<float> novelty(frames, 0), centroid(frames, 0), level(frames, 0);
  std::vector<std::array<float, kBands>> bandFlux(frames);
  std::vector<std::complex<float>> buf(kFrame);
  for (size_t f = 0; f < frames; ++f) {
    const float* p = &x[f * kHop];
    for (int i = 0; i < kFrame; ++i) buf[static_cast<size_t>(i)] = {p[i] * window[static_cast<size_t>(i)], 0};
    fft(buf);
    float num = 0, den = 1e-6f;
    for (int i = 0; i < kFrame / 2; ++i) {
      mag[static_cast<size_t>(i)] = std::log1p(40.0f * std::abs(buf[static_cast<size_t>(i)]));
      num += mag[static_cast<size_t>(i)] * static_cast<float>(i);
      den += mag[static_cast<size_t>(i)];
    }
    centroid[f] = num / den;
    level[f] = den;
    float total = 0;
    for (int b = 0; b < kBands; ++b) {
      float flux = 0;
      for (int i = bandBin[b]; i < bandBin[b + 1]; ++i) flux += std::max(0.0f, mag[static_cast<size_t>(i)] - prev[static_cast<size_t>(i)]);
      flux /= static_cast<float>(std::max(1, bandBin[b + 1] - bandBin[b]));
      bandFlux[f][static_cast<size_t>(b)] = flux;
      total += flux * (b == 0 ? 1.5f : 1.0f);  // the kick drum and bass carry the beat
    }
    novelty[f] = total;
    prev = mag;
  }

  // 3. Keep only what stands out from its surroundings (subtract a local
  // average over about half a second).
  const double frameSec = static_cast<double>(kHop) / srate;
  const int avgHalf = static_cast<int>(0.25 / frameSec);
  std::vector<float> peak(frames, 0);
  {
    double sum = 0;
    int lo = 0, hi = -1;
    for (int f = 0; f < static_cast<int>(frames); ++f) {
      while (hi < std::min(static_cast<int>(frames) - 1, f + avgHalf)) sum += novelty[static_cast<size_t>(++hi)];
      while (lo < f - avgHalf) sum -= novelty[static_cast<size_t>(lo++)];
      const float avg = static_cast<float>(sum / (hi - lo + 1));
      peak[static_cast<size_t>(f)] = std::max(0.0f, novelty[static_cast<size_t>(f)] - avg * 1.1f);
    }
  }

  // 4. Tempo: the lag (70-180 BPM) where the onset curve best matches a
  // shifted copy of itself, leaning toward ~120 BPM; then the beat's phase.
  const int minLag = static_cast<int>(60.0 / 180.0 / frameSec), maxLag = static_cast<int>(60.0 / 70.0 / frameSec);
  double bestScore = -1;
  int bestLag = minLag;
  for (int lag = minLag; lag <= maxLag; ++lag) {
    double s = 0;
    for (size_t f = static_cast<size_t>(lag); f < frames; ++f) s += peak[f] * peak[f - static_cast<size_t>(lag)];
    const double bpm = 60.0 / (lag * frameSec);
    const double pref = std::exp(-0.5 * std::pow(std::log2(bpm / 120.0) / 0.6, 2));
    s *= 0.5 + pref;
    if (s > bestScore) {
      bestScore = s;
      bestLag = lag;
    }
  }
  const double period = bestLag * frameSec;
  chart.bpm = 60.0 / period;
  double bestPhase = 0, bestPhaseScore = -1;
  for (int k = 0; k < bestLag; ++k) {
    double s = 0;
    for (size_t f = static_cast<size_t>(k); f < frames; f += static_cast<size_t>(bestLag)) s += peak[f];
    if (s > bestPhaseScore) {
      bestPhaseScore = s;
      bestPhase = k * frameSec;
    }
  }

  // 5. Candidate notes on the beat grid: beats (easy), eighths (normal) or
  // sixteenths (hard). Off-beat spots must be clearly louder to count.
  const int d = static_cast<int>(difficulty);
  static constexpr int kSubdiv[4] = {1, 2, 4, 4};
  static constexpr double kPerSec[4] = {1.5, 2.8, 4.8, 6.8}, kMinGap[4] = {0.28, 0.14, 0.09, 0.07};
  static constexpr float kChord[4] = {1.0f, 0.97f, 0.9f, 0.82f};
  static constexpr double kHoldShare[4] = {0, 0.12, 0.2, 0.25};
  static constexpr const char* kName[4] = {"easy", "normal", "hard", "insane"};
  const int subdiv = kSubdiv[d];
  const double targetPerSec = kPerSec[d];
  const double minGap = kMinGap[d];
  struct Candidate {
    double t;
    float strength;
    size_t frame;
  };
  std::vector<Candidate> cands;
  const double step = period / subdiv;
  const int search = std::max(1, static_cast<int>(0.03 / frameSec));
  for (int k = 0;; ++k) {
    const double t = bestPhase + k * step;
    const auto f = static_cast<int>(t / frameSec);
    if (f >= static_cast<int>(frames) - search) break;
    if (t < 1.0) continue;
    float best = 0;
    size_t bestF = static_cast<size_t>(f);
    for (int d = -search; d <= search; ++d) {
      const int g = f + d;
      if (g < 0 || g >= static_cast<int>(frames)) continue;
      if (peak[static_cast<size_t>(g)] > best) {
        best = peak[static_cast<size_t>(g)];
        bestF = static_cast<size_t>(g);
      }
    }
    const int pos = k % subdiv;
    const float weight = pos == 0 ? 1.0f : (subdiv == 4 && pos % 2 == 1) ? 0.65f : 0.8f;
    cands.push_back({t, best * weight, bestF});
  }
  if (cands.empty()) return chart;
  std::vector<float> strengths;
  for (const Candidate& c : cands) strengths.push_back(c.strength);
  const double duration = cands.back().t - cands.front().t;
  const float keepFraction = static_cast<float>(std::clamp(targetPerSec * duration / static_cast<double>(cands.size()), 0.05, 0.95));
  const float threshold = std::max(quantile(strengths, 1.0f - keepFraction), quantile(strengths, 0.5f) * 0.25f + 1e-4f);
  const float chordLevel = quantile(strengths, kChord[d]);
  const float tripleLevel = quantile(strengths, 0.97f);  // insane: three keys on the biggest hits

  // 6. Lanes follow the pitch (spectral centroid), spread evenly by
  // quantiles so all four lanes get used.
  std::vector<float> cents;
  for (const Candidate& c : cands) {
    if (c.strength >= threshold) cents.push_back(centroid[c.frame]);
  }
  const float q1 = quantile(cents, 0.25f), q2 = quantile(cents, 0.5f), q3 = quantile(cents, 0.75f);
  int lastLane = -1;
  double lastTime = -10;
  int repeats = 0;
  for (const Candidate& c : cands) {
    if (c.strength < threshold || c.t - lastTime < minGap - 1e-6) continue;
    const float ce = centroid[c.frame];
    int lane = ce < q1 ? 0 : ce < q2 ? 1 : ce < q3 ? 2 : 3;
    // Fast repeats on one key ("jacks") feel bad: step aside.
    if (lane == lastLane && (c.t - lastTime < 0.25 || ++repeats >= 2)) {
      lane = (lane + (ce >= q2 ? 3 : 1)) % 4;
      repeats = 0;
    } else if (lane != lastLane) {
      repeats = 0;
    }
    chart.notes.push_back({c.t, lane});
    if (difficulty != Difficulty::kEasy && c.strength >= chordLevel) {
      chart.notes.push_back({c.t, (lane + 2) % 4});  // a two-key chord on the loud hits
      if (difficulty == Difficulty::kInsane && c.strength >= tripleLevel) chart.notes.push_back({c.t, (lane + 1) % 4});
    }
    lastLane = lane;
    lastTime = c.t;
  }
  // 7. Hold notes: a note whose sound keeps ringing (the level a beat later
  // is still most of what it was) and whose lane is free long enough is
  // held until just before the next note in that lane (at most two beats).
  int holds = 0;
  if (difficulty != Difficulty::kEasy) {
    std::sort(chart.notes.begin(), chart.notes.end(), [](const ChartNote& a, const ChartNote& b) { return a.time < b.time; });
    const double maxShare = kHoldShare[d];
    for (size_t i = 0; i < chart.notes.size(); ++i) {
      ChartNote& n = chart.notes[i];
      if (static_cast<double>(holds) > maxShare * static_cast<double>(chart.notes.size())) break;
      double next = chart.length;
      bool chord = false;
      for (size_t j = i + 1; j < chart.notes.size(); ++j) {
        if (chart.notes[j].time - n.time < 1e-6) chord = true;
        if (chart.notes[j].lane == n.lane) {
          next = chart.notes[j].time;
          break;
        }
      }
      if (i > 0 && n.time - chart.notes[i - 1].time < 1e-6) chord = true;
      if (chord && difficulty != Difficulty::kInsane) continue;
      const double room = next - n.time;
      if (room < period * 1.5) continue;
      const size_t f0 = static_cast<size_t>(n.time / frameSec);
      const size_t f1 = std::min(frames - 1, f0 + static_cast<size_t>(period / frameSec));
      if (f0 >= frames || level[f1] < 0.82f * level[f0]) continue;
      const double len = std::min(room - period * 0.5, period * 2.0);
      n.end = n.time + std::floor(len / step + 1e-6) * step;
      if (n.end - n.time < period * 0.5) n.end = 0;
      else ++holds;
    }
  }
  LOG_INFO("Rhythm chart: %.0f BPM, %zu notes (%d held) in %.0f s (%s).", chart.bpm, chart.notes.size(), holds, chart.length,
           kName[d]);
  return chart;
}

}  // namespace app
