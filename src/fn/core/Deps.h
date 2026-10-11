// FlyghtEdge — ESP32 core libraries used inside the library.
//
// Included by FlyghtEdge.h so the Arduino build finds and links them also
// when FlyghtEdge is a precompiled archive (Arduino does not scan the
// sources of precompiled libraries for their dependencies).
#pragma once

#include <soc/soc_caps.h>

#include <DNSServer.h>
#include <ESPmDNS.h>
#include <FS.h>
#include <HTTPClient.h>
#include <Network.h>
#include <NetworkClientSecure.h>
#include <SD.h>
#include <SPI.h>
#include <Update.h>
#include <WiFi.h>
#include <Wire.h>
#if SOC_SDMMC_HOST_SUPPORTED
#include <SD_MMC.h>
#endif
