#pragma once
// A small JSON reader (enough for glTF model files). Parses the whole text
// into a tree of values; lookups on missing keys return a null value, so
// optional fields can be read without checks: `v["a"]["b"].number(1)`.
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace engine {

class Json {
 public:
  enum class Type { kNull, kBool, kNumber, kString, kArray, kObject };

  // Returns a null value (and sets `error`) if the text isn't valid JSON.
  static Json parse(std::string_view text, std::string* error = nullptr);

  Type type() const { return type_; }
  bool isNull() const { return type_ == Type::kNull; }
  bool isObject() const { return type_ == Type::kObject; }
  bool isArray() const { return type_ == Type::kArray; }
  bool has(const std::string& key) const { return object_ && object_->count(key); }

  double number(double fallback = 0) const { return type_ == Type::kNumber ? number_ : fallback; }
  int integer(int fallback = 0) const { return type_ == Type::kNumber ? static_cast<int>(number_) : fallback; }
  bool boolean(bool fallback = false) const { return type_ == Type::kBool ? bool_ : fallback; }
  const std::string& string() const { return string_; }

  size_t size() const { return array_ ? array_->size() : 0; }
  const Json& operator[](size_t i) const;
  const Json& operator[](int i) const { return i < 0 ? (*this)[size()] : (*this)[static_cast<size_t>(i)]; }
  const Json& operator[](const std::string& key) const;
  const Json& operator[](const char* key) const { return (*this)[std::string(key)]; }
  const std::vector<Json>& items() const;

 private:
  friend class JsonParser;
  Type type_ = Type::kNull;
  bool bool_ = false;
  double number_ = 0;
  std::string string_;
  std::shared_ptr<std::vector<Json>> array_;
  std::shared_ptr<std::map<std::string, Json>> object_;
};

}  // namespace engine
