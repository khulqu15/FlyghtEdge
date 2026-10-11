// FlyghtNinno — login, guest access, persistent sessions, users & roles, API tokens.
//
// Sessions are stateless HMAC-signed tokens (see pure::tokenSign). The signing
// secret lives in NVS, so a session survives Wi-Fi reconnects AND reboots.
//
// * Owner: the password created in the setup wizard (always admin).
// * Guest access: feature "guest" (off by default).
// * Users & roles (RBAC): feature "rbac" (Pro). Extra accounts with
//   viewer / operator / admin rights; changes apply immediately.
//
//   auth.defaultPassword("rahasia123");          // skip the setup wizard
//   auth.allowAnonymous(true, fn::Role::Viewer);  // turn guest access on by default
#pragma once

#include "../pure/PureTypes.h"
#include "../web/Web.h"

namespace fn {

class Auth : public Module {
public:
  const char* id() const override { return "auth"; }
  const char* title() const override { return "Authentication"; }
  const char* deps() const override { return "web"; }

  // Code defaults. Applied on first boot only; the UI can change them later.
  Auth& defaultPassword(const char* pw) { defaultPw_ = pw; return *this; }
  Auth& allowAnonymous(bool on, uint8_t role = Role::Viewer) { defAnonSet_ = true; defAnon_ = on; defAnonRole_ = role; return *this; }
  Auth& sessionDays(uint16_t days) { defDays_ = days; return *this; }

  bool begin(App& app) override;
  Health health() const override;
  void describe(JsonRef out) const override;

  bool setupRequired() const { return pwHash_.length() == 0; }
  bool guestAllowed() const;
  bool rbacOn() const;
  void authenticate(Request& q);
  // Issues a session token and writes the Set-Cookie header.
  String issue(Response& s, uint8_t role, uint8_t uid, bool remember, uint32_t* expOut = nullptr);

  static constexpr uint8_t kOwner = 0, kGuest = 255;

private:
  struct ApiToken { String name, prefix, hash; uint8_t role; uint32_t created; };
  struct User { uint8_t id; String name, salt, hash; uint8_t role; };

  void routes(Web& web);
  void userRoutes(Web& web);
  void loadTokens();
  void saveTokens();
  void loadUsers();
  void saveUsers();
  static void hashPassword(const String& pw, String& salt, String& hash);
  static bool verifyPassword(const String& pw, const String& salt, const String& hash);
  void setPassword(const String& pw);
  void syncClock(const JsonRef& body);
  void loginDone(Request& q, Response& s, uint8_t role, uint8_t uid, const String& name, bool remember);
  const User* findUser(uint8_t id) const;
  static String sha256Hex(const String& s);
  static String randomHex(size_t bytes);
  static String newApiToken();

  Kv kv_{"fn.auth"};
  uint8_t secret_[32] = {0};
  uint32_t epoch_ = 1;
  String pwHash_, salt_;
  uint8_t anonRole_ = Role::Viewer;
  uint16_t days_ = 30;
  std::vector<ApiToken> tokens_;
  std::vector<User> users_;
  uint8_t nextUserId_ = 1;
  pure::RateLimiter limiter_;
  uint32_t logins_ = 0, failures_ = 0;

  String defaultPw_;
  bool defAnonSet_ = false, defAnon_ = false;
  uint8_t defAnonRole_ = Role::Viewer;
  uint16_t defDays_ = 30;
};

}  // namespace fn
