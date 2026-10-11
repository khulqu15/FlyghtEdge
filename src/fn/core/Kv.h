// FlyghtEdge — small NVS key/value store, one namespace per module.
//
// NVS limits namespace and key names to 15 characters, so modules use
// namespaces like "fn.wifi" and short keys.
#pragma once

#include <Arduino.h>

namespace fn {

class Kv {
public:
  explicit Kv(const char* ns) : ns_(ns) {}

  String str(const char* key, const String& def = String()) const;
  int32_t i32(const char* key, int32_t def = 0) const;
  uint32_t u32(const char* key, uint32_t def = 0) const;
  bool flag(const char* key, bool def = false) const;
  float f32(const char* key, float def = 0) const;
  size_t bytes(const char* key, void* buf, size_t len) const;
  String blob(const char* key) const;  // a text stored with putBytes (no 4 KB string limit)
  bool has(const char* key) const;

  bool put(const char* key, const String& value);
  bool putI32(const char* key, int32_t value);
  bool putU32(const char* key, uint32_t value);
  bool putFlag(const char* key, bool value);
  bool putF32(const char* key, float value);
  bool putBytes(const char* key, const void* buf, size_t len);

  bool remove(const char* key);
  bool clear();
  const char* ns() const { return ns_; }

private:
  const char* ns_;
};

}  // namespace fn
