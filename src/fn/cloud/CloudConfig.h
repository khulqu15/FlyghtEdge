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
//   * An admin can set the site by hand: PUT /fn/api/v1/cloud {"site":"https://..."}
//     (an empty string goes back to this default).
//   * A new library release only has to change FN_CLOUD_SITE here.
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
