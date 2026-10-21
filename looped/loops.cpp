#include "looped/loops.h"

#include <algorithm>
#include <cmath>

#include "looped/json.h"

namespace engine::loops {

namespace {

constexpr double kTau = 6.283185307179586;
// The golden ratio's fraction: frequencies spaced by it never line up, so a
// swarm's path doesn't visibly repeat.
constexpr double kGolden = 0.6180339887498949;

// A repeatable pseudo-random number in [0, 1) from integers.
double hash01(uint32_t a, uint32_t b) {
  uint32_t h = a * 0x9E3779B1u ^ (b + 0x7F4A7C15u) * 0x85EBCA77u;
  h ^= h >> 15;
  h *= 0x2C1B3C6Du;
  h ^= h >> 12;
  h *= 0x297A2D39u;
  h ^= h >> 15;
  return (h & 0xFFFFFF) / 16777216.0;
}

// Smooth noise over time: a random value per whole step, blended between.
double noise(double t, uint32_t seed) {
  const double i = std::floor(t);
  const double f = t - i;
  const double u = f * f * (3 - 2 * f);
  const auto k = static_cast<uint32_t>(static_cast<int64_t>(i) & 0xFFFFFFFF);
  return hash01(seed, k) * (1 - u) + hash01(seed, k + 1) * u;
}

// Cycles done so far: in beats when the loop is on the beat and music plays.
double cycles(const Recipe& r, const Clock& c, int index) {
  const double t = r.onBeat && c.music ? c.beats : c.seconds;
  const double p = std::max(0.05, static_cast<double>(r.period));
  return t / p + r.spread * index;
}

Vec3 vec3(const Json& j, Vec3 fallback) {
  if (!j.isArray() || j.size() < 3) return fallback;
  return {static_cast<float>(j[0].number()), static_cast<float>(j[1].number()), static_cast<float>(j[2].number())};
}

}  // namespace

std::vector<Recipe> parse(const std::string& text, std::string* error) {
  std::vector<Recipe> out;
  std::string err;
  const Json root = Json::parse(text, &err);
  if (!root.isObject()) {
    if (error) *error = err.empty() ? "not a recipe file" : err;
    return out;
  }
  for (const Json& j : root["loops"].items()) {
    Recipe r;
    r.id = j["id"].string();
    r.about = j["about"].string();
    const std::string& kind = j["kind"].string();
    if (kind == "swarm") r.kind = Kind::kSwarm;
    else if (kind == "pulse") r.kind = Kind::kPulse;
    else if (kind == "flicker") r.kind = Kind::kFlicker;
    else {
      if (error) *error += "loop \"" + r.id + "\": unknown kind \"" + kind + "\". ";
      continue;
    }
    r.period = static_cast<float>(j["period"].number(r.period));
    r.onBeat = j["onBeat"].boolean(false);
    r.amount = static_cast<float>(j["amount"].number(r.amount));
    r.seed = static_cast<uint32_t>(j["seed"].number(1));
    r.spread = static_cast<float>(j["spread"].number(0));
    r.count = std::clamp(j["count"].integer(3), 1, 16);
    r.center = vec3(j["center"], r.center);
    r.radius = static_cast<float>(j["radius"].number(r.radius));
    r.height = static_cast<float>(j["height"].number(r.height));
    r.size = static_cast<float>(j["size"].number(r.size));
    r.color = vec3(j["color"], r.color);
    r.glow = static_cast<float>(j["glow"].number(r.glow));
    r.shy = static_cast<float>(j["shy"].number(0));
    for (const Json& l : j["lights"].items()) r.lights.push_back(l.integer(-1));
    for (const Json& m : j["models"].items()) r.models.push_back(m.string());
    if (j["near"].isArray()) {
      r.hasNear = true;
      r.near = vec3(j["near"], {});
      r.within = static_cast<float>(j["within"].number(1));
    }
    r.enabled = j["enabled"].boolean(true);
    r.musicOnly = j["musicOnly"].boolean(false);
    r.when = j["when"].string();
    out.push_back(std::move(r));
  }
  return out;
}

float pulse(const Recipe& r, const Clock& c, int index) {
  if (r.musicOnly && !c.music) return 1.0f;
  const double x = cycles(r, c, index);
  if (r.onBeat && c.music) {
    // A thump on each beat that fades quickly, like a kick drum.
    const double f = x - std::floor(x);
    return 1.0f + r.amount * static_cast<float>(std::exp(-f * 7.0) * 2.0 - 0.35);
  }
  return 1.0f + r.amount * static_cast<float>(std::sin(x * kTau));
}

float flicker(const Recipe& r, const Clock& c, int index) {
  // Fast noise for the buzz, slow noise for when a dip happens.
  const double t = c.seconds / std::max(0.05, static_cast<double>(r.period));
  const uint32_t seed = r.seed * 131u + static_cast<uint32_t>(index) * 7919u;
  const double slow = noise(t * 0.7, seed);
  const double fast = noise(t * 23.0, seed + 1);
  double v = 1.0 - 0.15 * r.amount * fast;
  if (slow > 0.82) v -= r.amount * (slow - 0.82) / 0.18 * (0.5 + 0.5 * fast);
  return static_cast<float>(std::max(0.0, v));
}

Vec3 swarmPosition(const Recipe& r, const Clock& c, int member) {
  const double t = c.seconds / std::max(0.05, static_cast<double>(r.period));
  const uint32_t s = r.seed * 977u + static_cast<uint32_t>(member) * 31u;
  const double ph1 = hash01(s, 1) * kTau, ph2 = hash01(s, 2) * kTau, ph3 = hash01(s, 3) * kTau;
  const double f = 1.0 + 0.3 * hash01(s, 4);
  // Three motions at unrelated speeds: around, in and out, up and down.
  const double a = t * kTau * f + ph1;
  const double wob = 0.65 + 0.35 * std::sin(t * kTau * f * (1 + kGolden) + ph2);
  const double up = std::sin(t * kTau * f * (2 - kGolden) * 1.7 + ph3);
  // A little jitter, like wings.
  const double jx = noise(t * 9.0, s + 5) - 0.5, jz = noise(t * 9.0, s + 6) - 0.5;
  return {r.center.x + static_cast<float>(r.radius * wob * std::cos(a) + 0.04 * jx),
          r.center.y + static_cast<float>(r.height * up),
          r.center.z + static_cast<float>(r.radius * wob * std::sin(a) + 0.04 * jz)};
}

float swarmTwinkle(const Recipe& r, const Clock& c, int member) {
  const double t = c.seconds / std::max(0.05, static_cast<double>(r.period));
  return static_cast<float>(0.55 + 0.45 * noise(t * 3.0, r.seed * 613u + static_cast<uint32_t>(member)));
}

}  // namespace engine::loops
