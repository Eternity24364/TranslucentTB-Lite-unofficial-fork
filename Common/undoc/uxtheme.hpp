#pragma once
#include "../arch.h"
#include <windef.h>

typedef BOOL(WINAPI* PFN_ALLOW_DARK_MODE_FOR_WINDOW)(HWND window, bool allow);
typedef BOOL(WINAPI* PFN_SHOULD_SYSTEM_USE_DARK_MODE)();
