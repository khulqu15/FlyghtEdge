// FlyghtNinno — thin RAII wrapper over cJSON (bundled with the ESP32 core).
//
//   fn::Json j;
//   j.set("ok", true).set("ip", WiFi.localIP().toString());
//   auto nets = j.arr("networks");
//   nets.pushObj().set("ssid", "Office");
//   res.json(j);
#pragma once

#include <Arduino.h>
#include "cJSON.h"

namespace fn {

// Non-owning view of a cJSON node.
class JsonRef {
public:
  JsonRef() = default;
  explicit JsonRef(cJSON* node) : n_(node) {}

  cJSON* raw() const { return n_; }
  bool valid() const { return n_ != nullptr; }
  bool isObject() const { return n_ && cJSON_IsObject(n_); }
  bool isArray() const { return n_ && cJSON_IsArray(n_); }

  // --- object setters (replace existing key) ---
  JsonRef& set(const char* key, const char* v);
  JsonRef& set(const char* key, const String& v) { return set(key, v.c_str()); }
  JsonRef& set(const char* key, bool v);
  JsonRef& set(const char* key, double v);
  JsonRef& set(const char* key, float v) { return set(key, (double)v); }
  JsonRef& set(const char* key, int v) { return set(key, (double)v); }
  JsonRef& set(const char* key, unsigned v) { return set(key, (double)v); }
  JsonRef& set(const char* key, long v) { return set(key, (double)v); }
  JsonRef& set(const char* key, unsigned long v) { return set(key, (double)v); }
  JsonRef& setNull(const char* key);
  JsonRef& setRaw(const char* key, const char* json);  // embeds pre-rendered JSON
  JsonRef obj(const char* key);                           // creates (or replaces) a child object
  JsonRef arr(const char* key);                           // creates (or replaces) a child array

  // --- array helpers ---
  JsonRef pushObj();
  JsonRef pushArr();
  JsonRef& push(const char* v);
  JsonRef& push(const String& v) { return push(v.c_str()); }
  JsonRef& push(double v);
  JsonRef& push(bool v);
  JsonRef& push(int v) { return push((double)v); }
  size_t size() const { return n_ ? (size_t)cJSON_GetArraySize(n_) : 0; }
  JsonRef at(size_t i) const { return JsonRef(n_ ? cJSON_GetArrayItem(n_, (int)i) : nullptr); }

  // --- getters ---
  bool has(const char* key) const { return n_ && cJSON_GetObjectItemCaseSensitive(n_, key) != nullptr; }
  JsonRef get(const char* key) const { return JsonRef(n_ ? cJSON_GetObjectItemCaseSensitive(n_, key) : nullptr); }
  String str(const char* key, const char* def = "") const;
  double num(const char* key, double def = 0) const;
  bool flag(const char* key, bool def = false) const;

  // value of this node
  bool isString() const { return n_ && cJSON_IsString(n_); }
  bool isNumber() const { return n_ && cJSON_IsNumber(n_); }
  bool isBool() const { return n_ && cJSON_IsBool(n_); }
  String asString() const { return isString() ? String(n_->valuestring) : String(); }
  double asNumber() const { return isNumber() ? n_->valuedouble : 0; }
  bool asBool() const { return n_ && cJSON_IsTrue(n_); }
  const char* key() const { return n_ && n_->string ? n_->string : ""; }

  // iteration over object members / array items
  template <typename F>
  void each(F f) const {
    if (!n_) return;
    cJSON* c = nullptr;
    cJSON_ArrayForEach(c, n_) f(JsonRef(c));
  }

  String dump() const;

protected:
  cJSON* n_ = nullptr;
};

// Owning document.
class Json : public JsonRef {
public:
  Json() : JsonRef(cJSON_CreateObject()) {}
  static Json array() { return Json(cJSON_CreateArray()); }
  static Json parse(const char* text, size_t len);
  static Json parse(const String& text) { return parse(text.c_str(), text.length()); }

  explicit Json(cJSON* adopt) : JsonRef(adopt) {}
  Json(Json&& o) noexcept : JsonRef(o.n_) { o.n_ = nullptr; }
  Json& operator=(Json&& o) noexcept {
    if (this != &o) { if (n_) cJSON_Delete(n_); n_ = o.n_; o.n_ = nullptr; }
    return *this;
  }
  Json(const Json&) = delete;
  Json& operator=(const Json&) = delete;
  ~Json() { if (n_) cJSON_Delete(n_); }

  bool ok() const { return n_ != nullptr; }
};

}  // namespace fn
