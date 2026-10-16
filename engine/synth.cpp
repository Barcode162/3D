#include "engine/synth.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace engine::synth {
namespace {

constexpr float kTwoPi = 6.28318530718f;

// Small fast random numbers (deterministic, so a sound is the same each time).
struct Noise {
  uint32_t s = 22222;
  float next() {
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return static_cast<float>(s & 0xffffff) / static_cast<float>(0x7fffff) - 1.0f;
  }
};

// A sine partial with an exponential decay, added into `out`. Uses a
// rotating phasor instead of sin() per sample (much faster).
void addPartial(std::vector<float>& out, float freq, float amp, float decaySeconds, float startPhase = 0) {
  if (freq >= kRate * 0.45f || amp <= 0) return;
  const float w = kTwoPi * freq / kRate;
  const float cw = std::cos(w), sw = std::sin(w);
  float c = std::cos(startPhase), s = std::sin(startPhase);
  const float k = std::exp(-6.9f / (decaySeconds * kRate));  // -60 dB after decaySeconds
  float a = amp;
  for (size_t i = 0; i < out.size(); ++i) {
    out[i] += s * a;
    const float nc = c * cw - s * sw;
    s = s * cw + c * sw;
    c = nc;
    a *= k;
    if ((i & 1023) == 0) {  // keep the phasor on the unit circle
      const float len = 1.0f / std::sqrt(c * c + s * s);
      c *= len;
      s *= len;
    }
  }
}

void normalize(std::vector<float>& v, float peak) {
  float m = 1e-6f;
  for (float x : v) m = std::max(m, std::fabs(x));
  for (float& x : v) x *= peak / m;
}

// Short fade at both ends so nothing clicks.
void fadeEnds(std::vector<float>& v, float inSeconds = 0.002f, float outSeconds = 0.04f) {
  const size_t a = std::min(v.size(), static_cast<size_t>(inSeconds * kRate));
  const size_t b = std::min(v.size(), static_cast<size_t>(outSeconds * kRate));
  for (size_t i = 0; i < a; ++i) v[i] *= static_cast<float>(i) / static_cast<float>(a);
  for (size_t i = 0; i < b; ++i) v[v.size() - 1 - i] *= static_cast<float>(i) / static_cast<float>(b);
}

// One-pole low-pass, in place.
void lowPass(std::vector<float>& v, float cutoff) {
  const float a = 1.0f - std::exp(-kTwoPi * cutoff / kRate);
  float y = 0;
  for (float& x : v) {
    y += a * (x - y);
    x = y;
  }
}

std::vector<float> samples(float seconds) { return std::vector<float>(static_cast<size_t>(seconds * kRate), 0.0f); }

}  // namespace

std::vector<float> pianoNote(int key) {
  const float f = 440.0f * std::pow(2.0f, static_cast<float>(key - 69) / 12.0f);
  // Low notes ring for a long time, high ones die quickly.
  const float ring = std::clamp(7.0f * std::pow(261.6f / f, 0.55f), 0.7f, 11.0f);
  const float seconds = std::clamp(ring * 0.55f, 0.9f, 3.6f);
  std::vector<float> out = samples(seconds);
  const float B = 0.00008f * std::pow(f / 27.5f, 0.9f);  // string stiffness (inharmonicity)
  const float detune[3] = {1.0f, 1.0011f, 0.9992f};  // three strings per note
  const int strings = key < 40 ? 2 : 3;
  for (int n = 1; n <= 16; ++n) {
    const float fn = f * static_cast<float>(n) * std::sqrt(1.0f + B * static_cast<float>(n * n));
    if (fn > 7000) break;
    // Brighter at the strike point; low notes carry many partials.
    const float amp = 1.0f / std::pow(static_cast<float>(n), key < 48 ? 0.9f : 1.4f) * (n == 1 && key < 36 ? 0.6f : 1.0f);
    const float decay = ring / (1.0f + 0.35f * static_cast<float>(n - 1));
    for (int s = 0; s < strings; ++s) {
      // Two-stage decay: a fast "prompt" part and a long aftersound.
      addPartial(out, fn * detune[s], amp * 0.6f, decay * 0.25f, 0.7f * static_cast<float>(s));
      addPartial(out, fn * detune[s], amp * 0.4f, decay, 1.3f * static_cast<float>(s));
    }
  }
  // Hammer knock: a short burst of filtered noise.
  Noise noise;
  const size_t knock = static_cast<size_t>(0.012f * kRate);
  float y = 0;
  for (size_t i = 0; i < knock && i < out.size(); ++i) {
    y += 0.3f * (noise.next() - y);
    out[i] += y * 0.25f * (1.0f - static_cast<float>(i) / static_cast<float>(knock));
  }
  normalize(out, 0.55f);
  fadeEnds(out, 0.003f, 0.25f);
  return out;
}

std::vector<float> pluck(float frequency, float seconds, float brightness) {
  std::vector<float> out = samples(seconds);
  const size_t period = std::max<size_t>(2, static_cast<size_t>(kRate / frequency));
  std::vector<float> line(period);
  Noise noise;
  for (float& x : line) x = noise.next();
  const float blend = 0.5f + 0.49f * brightness;
  size_t at = 0;
  for (float& o : out) {
    const size_t next = (at + 1) % period;
    const float v = line[at] * blend + line[next] * (1.0f - blend);
    o = line[at];
    line[at] = v * 0.996f;
    at = next;
  }
  normalize(out, 0.5f);
  fadeEnds(out);
  return out;
}

std::vector<float> effect(Effect e) {
  Noise noise;
  std::vector<float> out;
  switch (e) {
    case Effect::kBell: {
      out = samples(1.6f);
      const float partials[5] = {1.0f, 2.76f, 5.4f, 8.93f, 13.3f};  // a small bell's modes
      for (int i = 0; i < 5; ++i) addPartial(out, 1650.0f * partials[i], 1.0f / static_cast<float>(i + 1), 1.4f / (1 + i * 0.6f));
      break;
    }
    case Effect::kThunk: {
      out = samples(0.18f);
      addPartial(out, 140, 1.0f, 0.08f);
      addPartial(out, 410, 0.5f, 0.04f);
      for (size_t i = 0; i < 400 && i < out.size(); ++i) out[i] += noise.next() * 0.6f * (1.0f - static_cast<float>(i) / 400.0f);
      lowPass(out, 2500);
      break;
    }
    case Effect::kClink: {
      out = samples(0.7f);
      for (float f : {2350.0f, 3720.0f, 5110.0f}) addPartial(out, f, 1.0f, 0.5f);
      break;
    }
    case Effect::kPour: {
      out = samples(1.5f);
      float y = 0;
      for (size_t i = 0; i < out.size(); ++i) {
        y += 0.15f * (noise.next() - y);
        const float t = static_cast<float>(i) / kRate;
        const float bubbles = 0.5f + 0.5f * std::sin(t * 70.0f + std::sin(t * 13.0f) * 3.0f);
        out[i] = y * (0.6f + 0.4f * bubbles);
      }
      break;
    }
    case Effect::kShake: {
      out = samples(1.2f);
      for (size_t i = 0; i < out.size(); ++i) {
        const float t = static_cast<float>(i) / kRate;
        const float beat = std::pow(std::max(0.0f, std::sin(t * kTwoPi * 6.0f)), 8.0f);  // ice hitting the tin
        out[i] = noise.next() * beat;
      }
      lowPass(out, 5000);
      break;
    }
    case Effect::kClick: {
      out = samples(0.05f);
      for (size_t i = 0; i < out.size(); ++i) out[i] = noise.next() * std::exp(-static_cast<float>(i) / 60.0f);
      addPartial(out, 3000, 0.4f, 0.02f);
      break;
    }
    case Effect::kEngine: {
      // Exactly one second, harmonics of 50 Hz, so it loops without a seam.
      out = samples(1.0f);
      for (int n = 1; n <= 24; ++n) {
        const float amp = (n % 2 ? 1.0f : 0.6f) / static_cast<float>(n);
        const float w = kTwoPi * 50.0f * static_cast<float>(n) / kRate;
        for (size_t i = 0; i < out.size(); ++i) out[i] += amp * std::sin(w * static_cast<float>(i));
      }
      for (size_t i = 0; i < out.size(); ++i) out[i] += 0.15f * noise.next();
      normalize(out, 0.4f);
      return out;  // no fades: it loops
    }
    case Effect::kWhoosh: {
      out = samples(0.35f);
      float y = 0;
      for (size_t i = 0; i < out.size(); ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(out.size());
        y += (0.05f + 0.3f * t) * (noise.next() - y);
        out[i] = y * std::sin(t * 3.14159265f);
      }
      break;
    }
    case Effect::kKnock: {
      out = samples(0.9f);
      for (float at : {0.0f, 0.22f, 0.44f}) {
        std::vector<float> k = samples(0.15f);
        addPartial(k, 95, 1.0f, 0.06f);
        addPartial(k, 230, 0.6f, 0.04f);
        const size_t o = static_cast<size_t>(at * kRate);
        for (size_t i = 0; i < k.size() && o + i < out.size(); ++i) out[o + i] += k[i];
      }
      break;
    }
    case Effect::kCheer: {
      out = samples(1.8f);
      float y = 0;
      for (size_t i = 0; i < out.size(); ++i) {
        const float t = static_cast<float>(i) / kRate;
        y += 0.25f * (noise.next() - y);
        const float env = std::min(1.0f, t * 6.0f) * std::exp(-t * 1.4f);
        const float voices = 0.6f + 0.4f * std::sin(t * 31.0f) * std::sin(t * 17.0f);
        out[i] = y * env * voices;
      }
      lowPass(out, 3000);
      break;
    }
    case Effect::kCount: break;
  }
  normalize(out, 0.5f);
  fadeEnds(out);
  return out;
}

}  // namespace engine::synth
