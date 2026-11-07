#include "application.hpp"
#include <spdlog/spdlog.h>

#include "constants.hpp"
#include "../ProgramLog/error/win32.hpp"

Application::Application(HINSTANCE hInst, std::optional<std::filesystem::path> storageFolder) :
	m_Loader(),
	m_Worker(hInst, m_Loader, storageFolder),
	m_AppWindow(*this, hInst, m_Loader),
	m_ShuttingDown(false)
{
}

int Application::Run()
{
	while (true)
	{
		switch (MsgWaitForMultipleObjectsEx(0, nullptr, INFINITE, QS_ALLINPUT, MWMO_ALERTABLE | MWMO_INPUTAVAILABLE))
		{
		case WAIT_OBJECT_0:
			for (MSG msg; PeekMessage(&msg, 0, 0, 0, PM_REMOVE);)
			{
				if (msg.message != WM_QUIT)
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
				else
				{
					return static_cast<int>(msg.wParam);
				}
			}
			[[fallthrough]];
		case WAIT_IO_COMPLETION:
			continue;

		case WAIT_FAILED:
			LastErrorHandle(spdlog::level::critical, L"Failed to enter alertable wait state!");

		default:
			MessagePrint(spdlog::level::critical, L"MsgWaitForMultipleObjectsEx returned an unexpected value!");
		}
	}
}

void Application::Shutdown()
{
	// Shutdown can be requested several times (for example by both WM_CLOSE and WM_ENDSESSION).
	if (!m_ShuttingDown.exchange(true))
	{
		PostQuitMessage(0);
	}
}
