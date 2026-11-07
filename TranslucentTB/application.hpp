#pragma once
#include "arch.h"
#include <atomic>
#include <filesystem>
#include <optional>
#include <windef.h>

#include "dynamicloader.hpp"
#include "mainappwindow.hpp"
#include "taskbar/taskbarattributeworker.hpp"

class Application final {
	DynamicLoader m_Loader;

	TaskbarAttributeWorker m_Worker;
	MainAppWindow m_AppWindow;

	std::atomic<bool> m_ShuttingDown;

public:
	Application(HINSTANCE hInst, std::optional<std::filesystem::path> storageFolder);

	constexpr TaskbarAttributeWorker &GetWorker() noexcept { return m_Worker; }

	int Run();
	void Shutdown();
};
