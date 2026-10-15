#include "engine/json.h"

#include <cstdlib>

namespace engine {

namespace {
const Json kNullJson;
const std::vector<Json> kNoItems;
}  // namespace

class JsonParser {
 public:
  explicit JsonParser(std::string_view text) : s_(text) {}

  bool parseDocument(Json& out) {
    if (!parseValue(out, 0)) return false;
    skipSpace();
    return pos_ == s_.size() || fail("extra text after the end");
  }
  const std::string& error() const { return error_; }

 private:
  static constexpr int kMaxDepth = 64;

  bool fail(const char* what) {
    if (error_.empty()) error_ = std::string(what) + " at byte " + std::to_string(pos_);
    return false;
  }
  void skipSpace() {
    while (pos_ < s_.size() && (s_[pos_] == ' ' || s_[pos_] == '\n' || s_[pos_] == '\r' || s_[pos_] == '\t')) ++pos_;
  }
  bool literal(std::string_view word) {
    if (s_.substr(pos_, word.size()) != word) return fail("unknown word");
    pos_ += word.size();
    return true;
  }

  bool parseValue(Json& out, int depth) {
    if (depth > kMaxDepth) return fail("nested too deeply");
    skipSpace();
    if (pos_ >= s_.size()) return fail("unexpected end");
    const char c = s_[pos_];
    if (c == '{') return parseObject(out, depth);
    if (c == '[') return parseArray(out, depth);
    if (c == '"') {
      out.type_ = Json::Type::kString;
      return parseString(out.string_);
    }
    if (c == 't' || c == 'f') {
      out.type_ = Json::Type::kBool;
      out.bool_ = c == 't';
      return literal(c == 't' ? "true" : "false");
    }
    if (c == 'n') return literal("null");
    return parseNumber(out);
  }

  bool parseNumber(Json& out) {
    const size_t start = pos_;
    while (pos_ < s_.size() && std::string_view("+-0123456789.eE").find(s_[pos_]) != std::string_view::npos) ++pos_;
    if (pos_ == start) return fail("unexpected character");
    const std::string digits(s_.substr(start, pos_ - start));
    char* end = nullptr;
    out.number_ = std::strtod(digits.c_str(), &end);
    if (end != digits.c_str() + digits.size()) return fail("bad number");
    out.type_ = Json::Type::kNumber;
    return true;
  }

  static void appendUtf8(std::string& out, uint32_t cp) {
    if (cp < 0x80) {
      out += static_cast<char>(cp);
    } else if (cp < 0x800) {
      out += static_cast<char>(0xC0 | (cp >> 6));
      out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
      out += static_cast<char>(0xE0 | (cp >> 12));
      out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
      out += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
      out += static_cast<char>(0xF0 | (cp >> 18));
      out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
      out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
      out += static_cast<char>(0x80 | (cp & 0x3F));
    }
  }

  bool parseHex4(uint32_t& out) {
    if (pos_ + 4 > s_.size()) return fail("bad escape");
    out = 0;
    for (int i = 0; i < 4; ++i) {
      const char h = s_[pos_++];
      out <<= 4;
      if (h >= '0' && h <= '9') out |= static_cast<uint32_t>(h - '0');
      else if (h >= 'a' && h <= 'f') out |= static_cast<uint32_t>(h - 'a' + 10);
      else if (h >= 'A' && h <= 'F') out |= static_cast<uint32_t>(h - 'A' + 10);
      else return fail("bad escape");
    }
    return true;
  }

  bool parseString(std::string& out) {
    ++pos_;  // opening quote
    while (pos_ < s_.size()) {
      const char c = s_[pos_++];
      if (c == '"') return true;
      if (c != '\\') {
        out += c;
        continue;
      }
      if (pos_ >= s_.size()) break;
      const char e = s_[pos_++];
      switch (e) {
        case '"': out += '"'; break;
        case '\\': out += '\\'; break;
        case '/': out += '/'; break;
        case 'b': out += '\b'; break;
        case 'f': out += '\f'; break;
        case 'n': out += '\n'; break;
        case 'r': out += '\r'; break;
        case 't': out += '\t'; break;
        case 'u': {
          uint32_t cp;
          if (!parseHex4(cp)) return false;
          if (cp >= 0xD800 && cp < 0xDC00 && s_.substr(pos_, 2) == "\\u") {  // surrogate pair
            pos_ += 2;
            uint32_t low;
            if (!parseHex4(low)) return false;
            cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
          }
          appendUtf8(out, cp);
          break;
        }
        default: return fail("bad escape");
      }
    }
    return fail("unterminated string");
  }

  bool parseArray(Json& out, int depth) {
    ++pos_;
    out.type_ = Json::Type::kArray;
    out.array_ = std::make_shared<std::vector<Json>>();
    skipSpace();
    if (pos_ < s_.size() && s_[pos_] == ']') {
      ++pos_;
      return true;
    }
    while (true) {
      out.array_->emplace_back();
      if (!parseValue(out.array_->back(), depth + 1)) return false;
      skipSpace();
      if (pos_ >= s_.size()) return fail("unterminated array");
      const char c = s_[pos_++];
      if (c == ']') return true;
      if (c != ',') return fail("expected , or ]");
    }
  }

  bool parseObject(Json& out, int depth) {
    ++pos_;
    out.type_ = Json::Type::kObject;
    out.object_ = std::make_shared<std::map<std::string, Json>>();
    skipSpace();
    if (pos_ < s_.size() && s_[pos_] == '}') {
      ++pos_;
      return true;
    }
    while (true) {
      skipSpace();
      if (pos_ >= s_.size() || s_[pos_] != '"') return fail("expected a key");
      std::string key;
      if (!parseString(key)) return false;
      skipSpace();
      if (pos_ >= s_.size() || s_[pos_++] != ':') return fail("expected :");
      if (!parseValue((*out.object_)[key], depth + 1)) return false;
      skipSpace();
      if (pos_ >= s_.size()) return fail("unterminated object");
      const char c = s_[pos_++];
      if (c == '}') return true;
      if (c != ',') return fail("expected , or }");
    }
  }

  std::string_view s_;
  size_t pos_ = 0;
  std::string error_;
};

Json Json::parse(std::string_view text, std::string* error) {
  JsonParser parser(text);
  Json out;
  if (!parser.parseDocument(out)) {
    if (error) *error = parser.error();
    return Json();
  }
  return out;
}

const Json& Json::operator[](size_t i) const {
  return array_ && i < array_->size() ? (*array_)[i] : kNullJson;
}

const Json& Json::operator[](const std::string& key) const {
  if (!object_) return kNullJson;
  const auto it = object_->find(key);
  return it == object_->end() ? kNullJson : it->second;
}

const std::vector<Json>& Json::items() const { return array_ ? *array_ : kNoItems; }

}  // namespace engine
