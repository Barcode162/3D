#include "app/racing.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

using engine::Vec2;
using engine::Vec4;

namespace app {
namespace {

constexpr float kSeg = 200;           // length of one road segment (game units)
constexpr int kSegments = 640;        // segments per lap
constexpr float kTrack = kSeg * kSegments;
constexpr float kRoadWidth = 2000;    // half width of the road
constexpr float kCameraHeight = 1400;
constexpr float kDepth = 0.84f;       // 1 / tan(half the field of view)
constexpr float kMaxSpeed = 12000;    // units per second (about 220 km/h on the speedo)
constexpr int kLaps = 3;

// The track: curves and hills per segment, made from a few overlapping
// waves so it winds and rolls but always joins up at the start line.
struct Track {
  float curve[kSegments];
  float hill[kSegments];
  Track() {
    for (int i = 0; i < kSegments; ++i) {
      const float t = static_cast<float>(i) / kSegments * 2 * engine::kPi;
      float c = 0.9f * std::sin(t * 3) + 0.6f * std::sin(t * 7 + 1.3f);
      if (std::fabs(c) < 0.35f) c = 0;  // straights between the bends
      curve[i] = c * 4.0f;
      hill[i] = 1600 * std::sin(t * 4 + 0.5f) + 700 * std::sin(t * 9);
    }
  }
};
const Track& track() {
  static const Track t;
  return t;
}

struct Rect {
  float x, y, w, h;
  bool contains(float px, float py) const { return px >= x && px <= x + w && py >= y && py <= y + h; }
};
// Buttons (device pixels): steer left, steer right, brake, gas; stand up.
Rect button(int i, float W, float H) {
  const float bw = W * 0.23f, bh = H * 0.15f, y = H * 0.83f;
  const float xs[4] = {W * 0.02f, W * 0.27f, W * 0.52f, W * 0.75f};
  return {xs[i], y, bw, bh};
}
Rect standButton(float W, float H) { return {W * 0.74f, H * 0.1f, W * 0.24f, H * 0.05f}; }
Rect viewArea(float W, float H) { return {0, H * 0.09f, W, H * 0.71f}; }

std::string lapTime(double ms) {
  char b[32];
  const int total = static_cast<int>(ms / 10);
  std::snprintf(b, sizeof(b), "%d:%02d.%02d", total / 6000, (total / 100) % 60, total % 100);
  return b;
}

}  // namespace

void Racing::init(const engine::Sound* engineHum) { hum_ = engineHum; }

void Racing::start() {
  open_ = true;
  position_ = 0;
  x_ = 0;
  speed_ = 0;
  lap_ = 1;
  finished_ = false;
  raceStartMs_ = lapStartMs_ = 0;
  cars_.clear();
  const float lanes[5] = {-0.5f, 0.5f, 0.0f, -0.6f, 0.6f};
  for (int i = 0; i < 5; ++i) {
    cars_.push_back(kSeg * (6.0f + 7.0f * static_cast<float>(i)));       // ahead of you on the grid
    cars_.push_back(lanes[i]);
    cars_.push_back(kMaxSpeed * (0.62f + 0.07f * static_cast<float>(i)));  // the last one is quick
  }
  if (hum_ && !humVoice_) humVoice_ = hum_->play(0.0f, nullptr, 0, 0.6f, engine::Sound::kEffects, true);
}

void Racing::close() {
  open_ = false;
  held_.clear();
  left_ = right_ = gas_ = brake_ = false;
  engine::Sound::stop(humVoice_, 0, 0.3f);
  humVoice_ = 0;
}

bool Racing::update(double nowMs, float dt, const std::vector<engine::Input::RawPress>& presses, float W, float H) {
  bool stand = false;
  for (const engine::Input::RawPress& p : presses) {
    const float x = p.pos.x * W, y = p.pos.y * H;
    int b = -1;
    if (p.key >= 0 && p.key < 4) b = p.key;  // D F J K / arrows
    if (p.key == 4 && p.down) stand = true;
    if (p.key < 0) {
      for (int i = 0; i < 4; ++i) {
        if (button(i, W, H).contains(x, y)) b = i;
      }
      if (p.down && standButton(W, H).contains(x, y)) stand = true;
      if (p.down && finished_ && viewArea(W, H).contains(x, y)) start();  // tap to race again
    }
    if (p.down && b >= 0) held_.push_back({p.id, b});
    if (!p.down) held_.erase(std::remove_if(held_.begin(), held_.end(), [&](const Held& h) { return h.id == p.id; }),
                             held_.end());
  }
  left_ = right_ = gas_ = brake_ = false;
  for (const Held& h : held_) {
    if (h.button == 0) left_ = true;
    if (h.button == 1) right_ = true;
    if (h.button == 2) brake_ = true;
    if (h.button == 3) gas_ = true;
  }
  if (raceStartMs_ == 0) raceStartMs_ = lapStartMs_ = nowMs + 2000;  // 2 s countdown
  const bool racing = nowMs >= raceStartMs_ && !finished_;

  // Steering eases toward the buttons.
  const float target = (right_ ? 1.0f : 0.0f) - (left_ ? 1.0f : 0.0f);
  steer_ += (target - steer_) * std::min(1.0f, dt * 6.0f);
  const Track& t = track();
  const int seg = static_cast<int>(position_ / kSeg) % kSegments;
  const float speedPct = speed_ / kMaxSpeed;
  if (racing) {
    if (gas_) speed_ += 5200 * dt;
    else speed_ -= 1800 * dt;
    if (brake_) speed_ -= 11000 * dt;
    if (std::fabs(x_) > 1.0f && speed_ > kMaxSpeed * 0.35f) speed_ -= 9000 * dt;  // off the road: grass slows you
  } else {
    speed_ -= 6000 * dt;
  }
  speed_ = std::clamp(speed_, 0.0f, kMaxSpeed);
  x_ += steer_ * dt * 2.2f * speedPct;
  x_ -= t.curve[seg] * dt * speedPct * speedPct * 0.55f;  // bends push you outward
  x_ = std::clamp(x_, -2.2f, 2.2f);
  position_ += speed_ * dt;

  // AI cars drive their line; bumping into one from behind slows you down.
  for (size_t i = 0; i < cars_.size(); i += 3) {
    if (nowMs >= raceStartMs_) cars_[i] += cars_[i + 2] * dt;
    const float myTotal = position_ + static_cast<float>(lap_ - 1) * kTrack;
    const float gap = cars_[i] - myTotal;
    if (gap > 0 && gap < kSeg * 1.2f && std::fabs(cars_[i + 1] - x_) < 0.45f && speed_ > cars_[i + 2]) {
      speed_ = cars_[i + 2] * 0.85f;
    }
  }
  if (position_ >= kTrack) {
    position_ -= kTrack;
    lastLapMs_ = nowMs - lapStartMs_;
    if (bestLapMs_ == 0 || lastLapMs_ < bestLapMs_) bestLapMs_ = lastLapMs_;
    lapStartMs_ = nowMs;
    if (++lap_ > kLaps) {
      finished_ = true;
      lap_ = kLaps;
      finishMs_ = nowMs - raceStartMs_;
    }
  }
  if (!finished_) {
    const float myTotal = position_ + static_cast<float>(lap_ - 1) * kTrack;
    place_ = 1;
    for (size_t i = 0; i < cars_.size(); i += 3) place_ += cars_[i] > myTotal ? 1 : 0;
  }
  engine::Sound::set(humVoice_, 0.08f + 0.22f * speedPct, 0.55f + 1.5f * speedPct);
  return stand;
}

void Racing::stateFor(float out[5]) const {
  out[0] = position_;
  out[1] = x_;
  out[2] = speed_;
  out[3] = static_cast<float>(lap_);
  out[4] = steer_;
}

void Racing::render(const View& view, int segments, float aspect, const Emit& emit, const std::vector<float>* cars) {
  const Track& t = track();
  // Sky, then a ground color under everything.
  const Vec2 sky[4] = {{0, 0}, {1, 0}, {1, 0.62f}, {0, 0.62f}};
  emit(sky, {0.35f, 0.55f, 0.95f, 1});
  const Vec2 ground[4] = {{0, 0.5f}, {1, 0.5f}, {1, 1}, {0, 1}};
  emit(ground, {0.12f, 0.45f, 0.15f, 1});
  const int base = static_cast<int>(view.position / kSeg);
  const float offset = std::fmod(view.position, kSeg);
  const float camX = view.x * kRoadWidth;
  const float camY = kCameraHeight + t.hill[base % kSegments];
  float worldX = 0, dx = 0;
  float maxV = 1.0f;
  // The near edge of the first segment.
  float pu = 0.5f, pv = 1.2f, pw = 2.0f;
  struct Sprite {
    float u, v, w;
    Vec4 color;
  };
  std::vector<Sprite> sprites;
  for (int n = 0; n < segments; ++n) {
    const int i = (base + n) % kSegments;
    const float z = static_cast<float>(n + 1) * kSeg - offset;
    dx += t.curve[i];
    worldX += dx;
    const float scale = kDepth / z;
    const float u = 0.5f + 0.5f * scale * (worldX - camX - dx * (offset / kSeg));
    const float v = 0.5f - 0.5f * scale * (t.hill[i] - camY) * aspect;
    const float w = 0.5f * scale * kRoadWidth;
    if (v < maxV && n > 0) {
      const bool stripe = (i / 3) % 2 == 0;
      const Vec2 grass[4] = {{0, v}, {1, v}, {1, pv}, {0, pv}};
      emit(grass, stripe ? Vec4{0.16f, 0.55f, 0.2f, 1} : Vec4{0.13f, 0.48f, 0.17f, 1});
      const Vec2 rumble[4] = {{u - w * 1.15f, v}, {u + w * 1.15f, v}, {pu + pw * 1.15f, pv}, {pu - pw * 1.15f, pv}};
      emit(rumble, stripe ? Vec4{0.9f, 0.1f, 0.1f, 1} : Vec4{0.95f, 0.95f, 0.95f, 1});
      const Vec2 road[4] = {{u - w, v}, {u + w, v}, {pu + pw, pv}, {pu - pw, pv}};
      emit(road, stripe ? Vec4{0.38f, 0.38f, 0.4f, 1} : Vec4{0.35f, 0.35f, 0.37f, 1});
      if (stripe) {
        const Vec2 line[4] = {{u - w * 0.03f, v}, {u + w * 0.03f, v}, {pu + pw * 0.03f, pv}, {pu - pw * 0.03f, pv}};
        emit(line, {0.95f, 0.95f, 0.85f, 1});
      }
      if (i == 0) {  // the start/finish line
        const Vec2 fin[4] = {{u - w, v}, {u + w, v}, {pu + pw, pv}, {pu - pw, pv}};
        emit(fin, {0.95f, 0.95f, 0.95f, 1});
      }
      maxV = v;
    }
    // AI cars on this segment.
    if (cars) {
      for (size_t c = 0; c + 2 < cars->size(); c += 3) {
        const float cp = std::fmod((*cars)[c], kTrack);
        if (static_cast<int>(cp / kSeg) % kSegments != i) continue;
        const float cw = w * 0.35f;
        if (v < maxV + 0.001f || n < 2) {
          static const Vec4 kColors[5] = {{0.95f, 0.75f, 0.1f, 1}, {0.1f, 0.5f, 0.95f, 1}, {0.2f, 0.85f, 0.3f, 1},
                                          {0.85f, 0.2f, 0.85f, 1}, {0.95f, 0.45f, 0.1f, 1}};
          sprites.push_back({u + (*cars)[c + 1] * w, v, cw, kColors[(c / 3) % 5]});
        }
      }
    }
    pu = u;
    pv = v;
    pw = w;
    if (v < 0.3f) break;  // far up the screen: done
  }
  // Cars far to near so the near ones cover the far ones.
  for (auto it = sprites.rbegin(); it != sprites.rend(); ++it) {
    const float h = it->w * 0.55f * aspect;
    const Vec2 body[4] = {{it->u - it->w, it->v - h}, {it->u + it->w, it->v - h}, {it->u + it->w, it->v}, {it->u - it->w, it->v}};
    emit(body, it->color);
    const Vec2 glass[4] = {{it->u - it->w * 0.6f, it->v - h * 0.95f}, {it->u + it->w * 0.6f, it->v - h * 0.95f},
                           {it->u + it->w * 0.6f, it->v - h * 0.6f}, {it->u - it->w * 0.6f, it->v - h * 0.6f}};
    emit(glass, {0.1f, 0.1f, 0.15f, 1});
  }
  // Your car at the bottom.
  const float cw = 0.17f, ch = 0.09f * aspect, cu = 0.5f, cv = 0.97f;
  const Vec2 body[4] = {{cu - cw, cv - ch}, {cu + cw, cv - ch}, {cu + cw, cv}, {cu - cw, cv}};
  emit(body, {0.85f, 0.08f, 0.1f, 1});
  const Vec2 roof[4] = {{cu - cw * 0.55f, cv - ch * 1.6f}, {cu + cw * 0.55f, cv - ch * 1.6f}, {cu + cw * 0.7f, cv - ch},
                        {cu - cw * 0.7f, cv - ch}};
  emit(roof, {0.6f, 0.05f, 0.08f, 1});
  for (float s : {-1.0f, 1.0f}) {
    const Vec2 light[4] = {{cu + s * cw * 0.95f - 0.03f, cv - ch * 0.8f}, {cu + s * cw * 0.95f + 0.03f, cv - ch * 0.8f},
                           {cu + s * cw * 0.95f + 0.03f, cv - ch * 0.5f}, {cu + s * cw * 0.95f - 0.03f, cv - ch * 0.5f}};
    emit(light, {1.0f, 0.3f, 0.2f, 1});
  }
  (void)view.speed;
}

void Racing::draw(engine::TextRenderer& text, float W, float H, float px, double nowMs) const {
  text.addRect(0, 0, W, H, {0.02f, 0.02f, 0.03f, 1});
  const Rect area = viewArea(W, H);
  const View view{position_, x_, speed_};
  render(view, 140, area.w / area.h, [&](const Vec2* c, Vec4 color) {
    Vec2 p[4];
    for (int i = 0; i < 4; ++i) p[i] = {area.x + c[i].x * area.w, area.y + c[i].y * area.h};
    text.addScreenQuad(p, color);
  }, &cars_);
  // Cover what spills past the view (near road pieces are wider than it).
  text.addRect(0, 0, W, area.y, {0.02f, 0.02f, 0.03f, 1});
  text.addRect(0, area.y + area.h, W, H - area.y - area.h, {0.02f, 0.02f, 0.03f, 1});

  engine::TextStyle s;
  char line[96];
  s.color = {1.0f, 0.86f, 0.62f, 1};
  std::snprintf(line, sizeof(line), "LAP %d/%d   POS %d/6   %3.0f KM/H", lap_, kLaps, place_, speed_ / kMaxSpeed * 220);
  text.addScreen(line, W * 0.04f, H * 0.115f, 10 * px, s);
  const double lapMs = nowMs > lapStartMs_ && !finished_ ? nowMs - lapStartMs_ : 0;
  std::snprintf(line, sizeof(line), "TIME %s   BEST %s", lapTime(lapMs).c_str(),
                bestLapMs_ > 0 ? lapTime(bestLapMs_).c_str() : "-");
  text.addScreen(line, W * 0.04f, H * 0.145f, 8 * px, s);
  s.align = 0.5f;
  if (nowMs < raceStartMs_) {
    const int count = static_cast<int>((raceStartMs_ - nowMs) / 1000) + 1;
    s.color = {1, 1, 0.4f, 1};
    text.addScreen(count >= 2 ? "READY" : "SET", W * 0.5f, H * 0.35f, 26 * px, s);
  } else if (nowMs < raceStartMs_ + 800) {
    s.color = {0.4f, 1, 0.4f, 1};
    text.addScreen("GO!", W * 0.5f, H * 0.35f, 30 * px, s);
  }
  if (finished_) {
    s.color = {1, 1, 1, 1};
    std::snprintf(line, sizeof(line), "FINISHED %s!\nTime %s\nTap the road to race again", place_ == 1 ? "1ST" : place_ == 2 ? "2ND" : place_ == 3 ? "3RD" : (std::to_string(place_) + "TH").c_str(),
                  lapTime(finishMs_).c_str());
    text.addRect(W * 0.08f, H * 0.3f, W * 0.84f, H * 0.17f, {0, 0, 0, 0.7f});
    text.addScreen(line, W * 0.5f, H * 0.32f, 14 * px, s);
  }
  // Buttons.
  const char* labels[4] = {"<", ">", "BRAKE", "GAS"};
  const bool down[4] = {left_, right_, brake_, gas_};
  for (int i = 0; i < 4; ++i) {
    const Rect r = button(i, W, H);
    text.addRect(r.x, r.y, r.w, r.h, down[i] ? Vec4{1.0f, 0.42f, 0.78f, 0.95f} : Vec4{0.18f, 0.14f, 0.22f, 0.95f});
    engine::TextStyle b;
    b.align = 0.5f;
    text.addScreen(labels[i], r.x + r.w / 2, r.y + r.h / 2 - 7 * px, (i < 2 ? 18 : 12) * px, b);
  }
  const Rect st = standButton(W, H);
  text.addRect(st.x, st.y, st.w, st.h, {0.18f, 0.14f, 0.22f, 0.95f});
  engine::TextStyle b;
  b.align = 0.5f;
  text.addScreen("STAND UP", st.x + st.w / 2, st.y + st.h / 2 - 5 * px, 9 * px, b);
}

}  // namespace app
