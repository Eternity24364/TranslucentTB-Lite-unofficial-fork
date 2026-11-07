#pragma once
#include "arch.h"
#include <optional>
#include <windef.h>

#include "dynamicloader.hpp"
#include "tray/trayicon.hpp"

class Application;

class MainAppWindow final : public TrayIcon {
private:
	Application &m_App;
	std::optional<UINT> m_NewInstanceMessage;

	LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	void ShowContextMenu();
	void ResetDynamicState();
	void Exit();

public:
	MainAppWindow(Application &app, HINSTANCE hInstance, DynamicLoader &loader);
	~MainAppWindow();

	static void PostNewInstanceNotification();
};
