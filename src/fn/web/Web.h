// FlyghtEdge — HTTP + WebSocket server on top of ESP-IDF esp_http_server.
//
// No external dependency: esp_http_server ships with the ESP32 Arduino core.
// Handlers run on the HTTP server task, NOT on the loop task. Use
// App::Guard for shared state and app.post(...) to run code on the loop task.
//
//   web.get("/fn/api/v1/hello", fn::Role::Viewer, [](fn::Request& q, fn::Response& s) {
//     fn::Json j;  j.set("hello", q.user);  s.json(j);
//   });
#pragma once

#include <esp_http_server.h>
#include <list>

#include "../core/App.h"

namespace fn {

class Request {
public:
  explicit Request(httpd_req_t* r) : r_(r) {}
  httpd_req_t* raw() const { return r_; }
  int method() const { return r_->method; }
  String path() const;
  String query(const char* key) const;
  String header(const char* name) const;
  String cookie(const char* name) const;
  String clientIp() const;
  size_t contentLength() const { return r_->content_len; }
  // Reads the whole body (once). Returns "" and sets tooLarge() past maxLen.
  const String& body(size_t maxLen = 8192);
  Json json(size_t maxLen = 8192);
  bool tooLarge() const { return tooLarge_; }

  String tail;                 // part of the path matched by a trailing '*'
  uint8_t role = Role::None;   // filled by the authenticator
  String user;                 // "admin", "guest", "api:<name>"

private:
  httpd_req_t* r_;
  String body_;
  bool bodyRead_ = false;
  bool tooLarge_ = false;
};

class Response {
public:
  explicit Response(httpd_req_t* r) : r_(r) {}
  Response& header(const char* name, const String& value);
  void send(int code, const char* type, const char* data, size_t len);
  void json(int code, const JsonRef& j);
  void json(const JsonRef& j) { json(200, j); }
  void text(int code, const String& s, const char* type = "text/plain; charset=utf-8");
  void ok();                               // {"ok":true}
  void error(int code, const char* msg);   // {"error":"..."}
  void redirect(const String& url, int code = 302);
  void noContent();
  bool sent() const { return sent_; }

  // Chunked streaming (large files): beginStream, chunk..., endStream.
  void beginStream(int code, const char* type);
  bool chunk(const void* data, size_t len);
  void endStream();

private:
  httpd_req_t* r_;
  bool sent_ = false;
  std::list<String> keep_;  // esp_http_server keeps header pointers until send
};

using Handler = std::function<void(Request&, Response&)>;

// A UI page contributed by a module (e.g. fn::Gps adds a map page). The
// JavaScript lives gzip'd in flash next to the module, so it only exists in
// the firmware when that module is used.
struct UiPlugin {
  String id, title, icon, js;
};
using Fallback = std::function<bool(Request&, Response&)>;  // true = handled
using Authenticator = std::function<void(Request&)>;        // sets role + user

class Web : public Module {
public:
  explicit Web(uint16_t port = 80) : port_(port) {}
  const char* id() const override { return "web"; }
  const char* title() const override { return "Web server"; }
  bool begin(App& app) override;
  Health health() const override;
  void describe(JsonRef out) const override;

  // Routes. A trailing '*' matches the rest of the path (see Request::tail).
  // Register specific paths before wildcards that overlap them.
  void on(httpd_method_t method, const char* uri, uint8_t minRole, Handler h);
  void get(const char* uri, uint8_t minRole, Handler h) { on(HTTP_GET, uri, minRole, std::move(h)); }
  void post(const char* uri, uint8_t minRole, Handler h) { on(HTTP_POST, uri, minRole, std::move(h)); }
  void put(const char* uri, uint8_t minRole, Handler h) { on(HTTP_PUT, uri, minRole, std::move(h)); }
  void del(const char* uri, uint8_t minRole, Handler h) { on(HTTP_DELETE, uri, minRole, std::move(h)); }

  void setAuthenticator(Authenticator a) { auth_ = std::move(a); }
  void addFallback(Fallback f) { fallbacks_.push_back(std::move(f)); }
  // The single page app, gzip compressed, served at "/".
  void setIndex(const uint8_t* gz, size_t len, const char* etag) { index_ = gz; indexLen_ = len; etag_ = etag; }

  // Static gzip'd file served from flash (public, cached with ETag).
  // gzip=false for formats that are already compressed (woff2, png, jpg).
  void asset(const char* path, const char* type, const uint8_t* data, size_t len, const char* etag, bool gzip = true);
  // Registers a UI plugin page; its script is served at /fn/ui/p/<id>.js.
  void plugin(const char* id, const char* title, const char* icon, const uint8_t* gz, size_t len, const char* etag);
  const std::vector<UiPlugin>& plugins() const { return plugins_; }

  // WebSocket topics. A UI page asks for a topic with {"t":"sub","id":"imu"}
  // every few seconds; modules publish only while someone is watching.
  bool wanted(const char* topic) const;
  void publish(const char* topic, const JsonRef& data);

  // WebSocket at /fn/ws (authenticated with the same session cookie/token).
  void broadcast(const String& text);          // thread-safe
  void send(int fd, const String& text);       // thread-safe
  void onWsOpen(std::function<void(int fd)> f) { wsOpen_ = std::move(f); }
  void onWsMessage(std::function<void(int fd, const String& msg)> f) { wsMsg_ = std::move(f); }
  size_t wsClients() const;

  httpd_handle_t handle() const { return server_; }
  uint16_t port() const { return port_; }
  uint32_t requests() const { return requests_; }

private:
  struct Route {
    Web* self;
    httpd_method_t method;
    String uri;
    uint8_t minRole;
    Handler h;
  };

  static esp_err_t dispatch(httpd_req_t* r);
  static esp_err_t notFound(httpd_req_t* r, httpd_err_code_t err);
  static esp_err_t wsEntry(httpd_req_t* r);
  static esp_err_t serveIndex(httpd_req_t* r);
  static esp_err_t preflight(httpd_req_t* r);
  void queueWs(int fd, const String& text);
  void addWsFd(int fd);

  uint16_t port_;
  httpd_handle_t server_ = nullptr;
  std::vector<Route*> routes_;
  std::vector<Fallback> fallbacks_;
  Authenticator auth_;
  const uint8_t* index_ = nullptr;
  size_t indexLen_ = 0;
  const char* etag_ = "";
  std::function<void(int)> wsOpen_;
  std::function<void(int, const String&)> wsMsg_;
  std::vector<int> wsFds_;
  std::vector<UiPlugin> plugins_;
  struct Sub { String topic; uint32_t at; };
  std::vector<Sub> subs_;
  mutable SemaphoreHandle_t wsMtx_ = nullptr;
  uint32_t requests_ = 0;  // statistics only; benign races
  bool registerFailed_ = false;
};

}  // namespace fn
