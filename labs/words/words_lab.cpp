// The word lab (J116.2): Looped Script's words in the browser. The page
// (labs/words/index.html) hands it the dictionary and a phrase; it hands
// back what the phrase means (as JSON) and, moment by moment, the lights it
// gives. Built with em++ by tools/labs.mjs.
#include <emscripten/emscripten.h>

#include <string>

#include "looped/json.h"
#include "looped/words/words.h"

using namespace engine;
using namespace engine::words;

namespace {

Words* g_words = nullptr;
Thing g_thing;
bool g_have = false;
std::string g_out;

std::string esc(const std::string& s) {
  std::string o;
  for (char c : s) {
    if (c == '"' || c == '\\') o += '\\', o += c;
    else if (c == '\n') o += "\\n";
    else if (static_cast<unsigned char>(c) < 0x20) o += ' ';
    else o += c;
  }
  return o;
}

std::string num(double v) {
  char b[32];
  std::snprintf(b, sizeof b, "%.6g", v);
  return b;
}

std::string problemsJson(const Problems& p) {
  std::string s = "[";
  for (size_t i = 0; i < p.size(); ++i) {
    if (i) s += ",";
    s += "{\"line\":" + std::to_string(p[i].line) + ",\"column\":" + std::to_string(p[i].column) + ",\"what\":\"" +
         esc(p[i].what) + "\",\"hint\":\"" + esc(p[i].hint) + "\",\"text\":\"" + esc(p[i].text()) + "\",\"warning\":" +
         (p[i].warning ? "true" : "false") + "}";
  }
  return s + "]";
}

std::string vec(Vec3 v) { return "[" + num(v.x) + "," + num(v.y) + "," + num(v.z) + "]"; }

std::string thingJson(const Thing& t) {
  std::string s = "{\"word\":\"" + esc(t.word) + "\",\"count\":" + std::to_string(t.count) + ",\"model\":\"" + esc(t.model) +
                  "\",\"box\":" + vec(t.box) + ",\"at\":" + vec(t.at);
  if (t.glows) s += ",\"glow\":" + vec(t.glow);
  if (t.hasLight) {
    const LightSpec& l = t.light;
    s += ",\"light\":{\"lumens\":" + num(l.lumens) + ",\"kelvin\":" + (l.rgb ? std::string("null") : num(l.kelvin)) +
         ",\"colour\":" + vec(l.tint()) + ",\"shape\":\"" + (l.shape == 0 ? "ball" : l.shape == 1 ? "tube" : "panel") +
         "\",\"a\":" + num(l.a) + ",\"b\":" + num(l.b) + ",\"oneSided\":" + (l.oneSided ? "true" : "false") + ",\"doings\":[";
    for (size_t i = 0; i < t.doings.size(); ++i) {
      if (i) s += ",";
      s += "{\"word\":\"" + esc(t.doings[i].word) + "\",\"settings\":{";
      for (size_t k = 0; k < t.doings[i].settings.size(); ++k) {
        const auto& [name, v] = t.doings[i].settings[k];
        s += (k ? "," : "") + std::string("\"") + esc(name) + "\":\"" + (name == "amount" ? num(v.v * 100) + "%" : num(v.v) + (v.unit.empty() ? "" : " " + v.unit)) + "\"";
      }
      s += "}}";
    }
    s += "]}";
  }
  s += ",\"trail\":[";
  for (size_t i = 0; i < t.trail.size(); ++i) s += (i ? "," : "") + std::string("\"") + esc(t.trail[i]) + "\"";
  s += "],\"parts\":[";
  for (size_t i = 0; i < t.parts.size(); ++i) s += (i ? "," : "") + thingJson(t.parts[i]);
  return s + "]}";
}

}  // namespace

extern "C" {

// The dictionary (one file's text); returns its problems (JSON list).
EMSCRIPTEN_KEEPALIVE const char* lw_load(const char* text, const char* file) {
  delete g_words;
  g_words = new Words();
  g_have = false;
  Problems p;
  g_words->load(text, file, p);
  g_out = problemsJson(p);
  return g_out.c_str();
}

// A phrase → {"ok":…, "problems":[…], "thing":{…}}.
EMSCRIPTEN_KEEPALIVE const char* lw_read(const char* phrase) {
  if (!g_words) return "{\"ok\":false,\"problems\":[]}";
  Problems p;
  Thing t;
  const bool ok = g_words->read(phrase, t, p);
  g_have = ok;
  if (ok) g_thing = t;
  g_out = std::string("{\"ok\":") + (ok ? "true" : "false") + ",\"problems\":" + problemsJson(p) + (ok ? ",\"thing\":" + thingJson(t) : "") + "}";
  return g_out.c_str();
}

// The last good phrase's lights at `seconds`: [{at, colour, lumens, shape, a, b, oneSided}].
EMSCRIPTEN_KEEPALIVE const char* lw_lights(double seconds, double beats) {
  g_out = "[";
  if (g_words && g_have) {
    world::Clock c;
    c.seconds = seconds;
    c.beats = beats;
    c.music = beats > 0;
    std::vector<Words::Glow> glows;
    g_words->lights(g_thing, c, glows);
    for (size_t i = 0; i < glows.size(); ++i) {
      const Words::Glow& g = glows[i];
      g_out += (i ? "," : "") + std::string("{\"at\":") + vec(g.at) + ",\"colour\":" + vec(g.colour) + ",\"lumens\":" + num(g.lumens) +
               ",\"shape\":" + std::to_string(g.spec->shape) + ",\"a\":" + num(g.spec->a) + ",\"b\":" + num(g.spec->b) + "}";
    }
  }
  g_out += "]";
  return g_out.c_str();
}

// Every word: [{name, kind, describes, forms, like, file, line}] (the map of words).
EMSCRIPTEN_KEEPALIVE const char* lw_words() {
  g_out = "[";
  if (g_words) {
    const auto& all = g_words->dictionary().all();
    for (size_t i = 0; i < all.size(); ++i) {
      const Word& w = all[i];
      g_out += (i ? "," : "") + std::string("{\"name\":\"") + esc(w.name) + "\",\"kind\":\"" + kindName(w.kind) + "\",\"describes\":\"" +
               esc(w.describes) + "\",\"parent\":\"" + esc(w.parent) + "\",\"line\":" + std::to_string(w.line) + ",\"forms\":[";
      for (size_t k = 0; k < w.forms.size(); ++k) g_out += (k ? "," : "") + std::string("\"") + esc(w.forms[k]) + "\"";
      g_out += "],\"like\":[";
      for (size_t k = 0; k < w.like.size(); ++k) g_out += (k ? "," : "") + std::string("\"") + esc(w.like[k]) + "\"";
      g_out += "],\"meaning\":[";
      for (size_t k = 0; k < w.lines.size(); ++k) g_out += (k ? "," : "") + std::string("\"") + esc(joinWords(w.lines[k], 0, w.lines[k].tokens.size())) + "\"";
      g_out += "]}";
    }
  }
  g_out += "]";
  return g_out.c_str();
}

}  // extern "C"
