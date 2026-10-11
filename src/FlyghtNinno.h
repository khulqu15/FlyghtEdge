// FlyghtNinno was renamed to FlyghtEdge in 2.1.0. This header only forwards to the new one so that
// existing sketches keep compiling; change the include to <FlyghtEdge.h>. It will be removed in 3.0.
#pragma once

#pragma message("FlyghtNinno.h is deprecated: use #include <FlyghtEdge.h>")
#include "FlyghtEdge.h"
