#pragma once
#include "util/string_macros.hpp"

// Technical identity: used for the executable name, window classes, log folders and
// anything that must stay stable across builds.
#define UTF8_APP_NAME "TranslucentTB"
#define APP_NAME UTIL_WIDEN(UTF8_APP_NAME)

// User-facing name.
#define UTF8_APP_DISPLAY_NAME "TranslucentTB Lite"
#define APP_DISPLAY_NAME UTIL_WIDEN(UTF8_APP_DISPLAY_NAME)

#define APP_COPYRIGHT_YEAR_NUM 2025
#define APP_COPYRIGHT_YEAR UTIL_STRINGIFY(APP_COPYRIGHT_YEAR_NUM)

#define APP_VERSION_FIXED 1,0,0,1
#define UTF8_APP_VERSION "1.0.0.1"
#define APP_VERSION UTIL_WIDEN(UTF8_APP_VERSION)
