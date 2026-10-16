// FlyghtEdge — the one address a build carries for Flyght Cloud.
//
// Moving the cloud to another address never needs every sketch to be rebuilt:
//
//   * Devices that have talked to the server remember the site the server last named
//     (claim and config answers carry the full URLs; the device uses them as given and
//     saves the site they belong to). Answer with the new URLs from the old address
//     once and the whole fleet follows.
//   * A saved site that fails kMaxSiteFailures times in a row is dropped and the address
//     below is used again, so a wrong move cannot cut a device off for good.
//   * A new library release only has to change FN_CLOUD_SITE here.
//
// The address is never a local setting: the dashboard does not offer it and
// PUT /fn/api/v1/cloud refuses a body that carries "site" (403), so a device follows only the
// address this build carries or the one the server named. Building from source with
// -DFN_CLOUD_SITE=... or dev.cloud().site(...) is the way to point a build somewhere else.
//
// Define FN_CLOUD_SITE in the build flags (-DFN_CLOUD_SITE=\"https://...\") to override it
// when building the library from source.
#pragma once

#ifndef FN_CLOUD_SITE
#define FN_CLOUD_SITE "https://flyght.obayan.id"
#endif

namespace fn {

// Consecutive failures to reach a saved site before the built-in address is used again.
constexpr int kMaxSiteFailures = 5;

}  // namespace fn
