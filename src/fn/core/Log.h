// FlyghtEdge — tagged logging with an in-memory ring buffer.
//
//   FN_LOGI("wifi", "Connected ip=%s", ip.c_str());
//
// Lines go to Serial and to a ring buffer that the web UI reads through
// GET /fn/api/v1/sys/logs. Identical consecutive lines are collapsed into
// "(repeated xN)" so reconnect loops do not flood the console.
#pragma once

#include <Arduino.h>
#include <functional>

namespace fn {

enum class LogLevel : uint8_t { Error = 1, Warn = 2, Info = 3, Debug = 4 };

namespace log {

struct Line {
  uint32_t seq;
  uint32_t ms;
  uint8_t level;
  String tag;
  String msg;
};

// Where lines are printed. Bound to `Serial` by fn::App's constructor, which
// is compiled in your sketch, so the right port is used for every board
// setting (UART or native USB). nullptr = ring buffer only.
void setOutput(Print* out);
void flush();
void setLevel(LogLevel level);
LogLevel level();
void write(LogLevel level, const char* tag, const char* fmt, ...) __attribute__((format(printf, 3, 4)));
// Visits buffered lines with seq > sinceSeq, oldest first. Thread-safe.
void each(uint32_t sinceSeq, const std::function<void(const Line&)>& fn);
uint32_t lastSeq();

}  // namespace log
}  // namespace fn

#define FN_LOGE(tag, ...) ::fn::log::write(::fn::LogLevel::Error, tag, __VA_ARGS__)
#define FN_LOGW(tag, ...) ::fn::log::write(::fn::LogLevel::Warn, tag, __VA_ARGS__)
#define FN_LOGI(tag, ...) ::fn::log::write(::fn::LogLevel::Info, tag, __VA_ARGS__)
#define FN_LOGD(tag, ...) ::fn::log::write(::fn::LogLevel::Debug, tag, __VA_ARGS__)
