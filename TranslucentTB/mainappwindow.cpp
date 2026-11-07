#include "mainappwindow.hpp"
#include <shellapi.h>

#include "application.hpp"
#include "constants.hpp"
#include "localization.hpp"
#include "resources/ids.h"
#include "../ProgramLog/log.hpp"
#include "../ProgramLog/error/win32.hpp"

static constexpr UINT EXIT_MENU_ID = 1;

LRESULT MainAppWindow::MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_HOTKEY:
		if (wParam == RESET_STATE_GLOBAL_HOTKEY_ID)
		{
			ResetDynamicState();
		}
		return 0;

	case WM_CLOSE:
		Exit();
		return 1;

	case WM_QUERYENDSESSION:
		if (lParam & ENDSESSION_CLOSEAPP)
		{
			// The app is being queried if it can close for an update.
			RegisterApplicationRestart(nullptr, 0);
		}
		return 1;

	case WM_ENDSESSION:
		if (wParam)
		{
			// The app can be killed after processing this message, but we'll try doing it gracefully
			Exit();
		}

		return 0;

	case TRAY_CALLBACK:
		switch (LOWORD(lParam))
		{
		case WM_CONTEXTMENU:
		case WM_LBUTTONUP:
		case NIN_KEYSELECT:
		case NIN_SELECT:
			ShowContextMenu();
			break;
		}
		return 0;

	default:
		if (uMsg == m_NewInstanceMessage)
		{
			SendNotification(IDS_ALREADY_RUNNING, NIF_REALTIME, NIIF_INFO);
			ResetDynamicState();
			return 0;
		}
		else
		{
			return TrayIcon::MessageHandler(uMsg, wParam, lParam);
		}
	}
}

void MainAppWindow::ShowContextMenu()
{
	// Required so that the menu gets dismissed when clicking elsewhere.
	SetForegroundWindow(handle());

	HMENU menu = CreatePopupMenu();
	if (!menu) [[unlikely]]
	{
		LastErrorHandle(spdlog::level::warn, L"Failed to create the tray context menu.");
		return;
	}

	const auto exitText = Localization::LoadLocalizedResourceString(IDS_EXIT, hinstance());
	AppendMenuW(menu, MF_STRING, EXIT_MENU_ID, exitText.c_str());

	POINT pt;
	GetCursorPos(&pt);

	const UINT command = TrackPopupMenuEx(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, handle(), nullptr);
	DestroyMenu(menu);

	// The menu swallows the next click unless the window is foreground again.
	post_message(WM_NULL);
	ReturnFocus();

	if (command == EXIT_MENU_ID)
	{
		Exit();
	}
}

void MainAppWindow::ResetDynamicState()
{
	m_App.GetWorker().ResetState(true);
}

void MainAppWindow::Exit()
{
	m_App.Shutdown();
}

MainAppWindow::MainAppWindow(Application &app, HINSTANCE hInstance, DynamicLoader &loader) :
	MessageWindow(TRAY_WINDOW, APP_NAME, hInstance, WS_POPUP, WS_EX_TOPMOST | WS_EX_NOREDIRECTIONBITMAP),
	TrayIcon(TRAY_GUID, MAKEINTRESOURCE(IDI_TRAYWHITEICON), MAKEINTRESOURCE(IDI_TRAYBLACKICON), loader.ShouldSystemUseDarkMode()),
	m_App(app),
	m_NewInstanceMessage(Window::RegisterMessage(WM_TTBNEWINSTANCESTARTED))
{
	if (const auto admfw = loader.AllowDarkModeForWindow())
	{
		admfw(m_WindowHandle, true);
	}

	// Register global hotkey for resetting dynamic state
	const BOOL hotkeyRegistered = RegisterHotKey(
		handle(),
		RESET_STATE_GLOBAL_HOTKEY_ID,
		MOD_CONTROL | MOD_ALT | MOD_SHIFT | MOD_NOREPEAT,
		VK_F1
	);
	if (!hotkeyRegistered)
	{
		LastErrorHandle(spdlog::level::warn, L"Unable to register global hotkey for dynamic state reset");
	}

	Show();
}

MainAppWindow::~MainAppWindow()
{
	UnregisterHotKey(handle(), RESET_STATE_GLOBAL_HOTKEY_ID);
}

void MainAppWindow::PostNewInstanceNotification()
{
	if (const auto msg = Window::RegisterMessage(WM_TTBNEWINSTANCESTARTED))
	{
		if (const auto runningInstance = Window::Find(TRAY_WINDOW, APP_NAME))
		{
			AllowSetForegroundWindow(runningInstance.process_id());
			runningInstance.post_message(*msg);
		}
	}
}
