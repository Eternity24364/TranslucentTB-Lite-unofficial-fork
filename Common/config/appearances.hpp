#pragma once
#include <windef.h>

#include "../undoc/user32.hpp"
#include "../util/color.hpp"

// Describes how the taskbar should be painted. There is no user configuration:
// every value below is fixed at build time.
struct TaskbarAppearance {
	ACCENT_STATE Accent = ACCENT_NORMAL;
	Util::Color Color = { 0, 0, 0, 0 };
	bool ShowPeek = true;
	bool ShowLine = true;
	float BlurRadius = 9.0f;
};

// Transparent taskbar when only the desktop is visible.
inline constexpr TaskbarAppearance DESKTOP_APPEARANCE { ACCENT_ENABLE_TRANSPARENTGRADIENT, { 0, 0, 0, 0 }, false, false, 9.0f };

// Transparent taskbar when a window is visible. The peek button stays usable.
inline constexpr TaskbarAppearance WINDOW_APPEARANCE { ACCENT_ENABLE_TRANSPARENTGRADIENT, { 0, 0, 0, 0 }, true, false, 9.0f };

// Acrylic taskbar when a window is maximized.
inline constexpr TaskbarAppearance MAXIMISED_APPEARANCE { ACCENT_ENABLE_ACRYLICBLURBEHIND, { 0, 0, 0, 0 }, true, false, 9.0f };

// Transparent taskbar while Task View is open.
inline constexpr TaskbarAppearance TASK_VIEW_APPEARANCE { ACCENT_ENABLE_TRANSPARENTGRADIENT, { 0, 0, 0, 0 }, false, false, 9.0f };

// Restores the taskbar to its stock appearance (used when unloading).
inline constexpr TaskbarAppearance STOCK_APPEARANCE { ACCENT_NORMAL, { 0, 0, 0, 0 }, true, true, 0.0f };
