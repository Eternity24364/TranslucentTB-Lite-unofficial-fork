#include "taskbarattributeworker.hpp"
#include <functional>
#include <member_thunk/member_thunk.hpp>
#include <set>
#include <tlhelp32.h>

#include "constants.hpp"
#include "../localization.hpp"
#include "../uwp/uwp.hpp"
#include "../../ProgramLog/error/win32.hpp"
#include "../../ProgramLog/error/winrt.hpp"
#include "undoc/explorer.hpp"
#include "undoc/user32.hpp"
#include "undoc/winuser.hpp"
#include "win32.hpp"
#include "winrt/Windows.Foundation.h"
#include "winrt/Windows.Foundation.Metadata.h"

class TaskbarAttributeWorker::AttributeRefresher {
private:
	TaskbarAttributeWorker &m_Worker;
	std::set<HMONITOR> m_ToRefresh;
	bool m_Armed;

public:
	AttributeRefresher(TaskbarAttributeWorker &worker, bool refresh = true) noexcept :
		m_Worker(worker), m_Armed(refresh) { }

	AttributeRefresher(const AttributeRefresher &) = delete;
	AttributeRefresher &operator =(const AttributeRefresher &) = delete;

	void refresh(taskbar_iterator it)
	{
		if (m_Armed)
		{
			m_ToRefresh.insert(it->first);
		}
	}

	void disarm() noexcept
	{
		m_Armed = false;
		m_ToRefresh.clear();
	}

	~AttributeRefresher() noexcept(false)
	{
		if (m_Armed)
		{
			for (const auto mon : m_ToRefresh)
			{
				if (const auto it = m_Worker.m_Taskbars.find(mon); it != m_Worker.m_Taskbars.end())
				{
					m_Worker.RefreshAttribute(it);
				}
			}
		}
	}
};

template<DWORD insert, DWORD remove>
void TaskbarAttributeWorker::WindowInsertRemove(DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD, DWORD)
{
	if (const Window window(hwnd); idObject == OBJID_WINDOW && idChild == CHILDID_SELF)
	{
		if (event == insert && window.valid())
		{
			InsertWindow(window, true);
		}
		else if (event == remove)
		{
			AttributeRefresher refresher(*this);
			for (auto it = m_Taskbars.begin(); it != m_Taskbars.end(); ++it)
			{
				RemoveWindow(window, it, refresher);
			}
		}
	}
}

void TaskbarAttributeWorker::OnAeroPeekEnterExit(DWORD event, HWND, LONG, LONG, DWORD, DWORD)
{
	m_PeekActive = event == EVENT_SYSTEM_PEEKSTART;
	MessagePrint(spdlog::level::debug, m_PeekActive ? L"Aero Peek entered" : L"Aero Peek exited");

	RefreshAllAttributes();
}

void TaskbarAttributeWorker::OnWindowStateChange(DWORD, HWND hwnd, LONG idObject, LONG idChild, DWORD, DWORD)
{
	if (const Window window(hwnd); idObject == OBJID_WINDOW && idChild == CHILDID_SELF && window.valid())
	{
		InsertWindow(window, true);
	}
}

void TaskbarAttributeWorker::OnWindowCreateDestroy(DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD, DWORD)
{
	if (const Window window(hwnd); idObject == OBJID_WINDOW && idChild == CHILDID_SELF)
	{
		if (event == EVENT_OBJECT_CREATE && window.valid())
		{
			if (const auto className = window.classname(); className && (*className == TASKBAR || *className == SECONDARY_TASKBAR))
			{
				MessagePrint(spdlog::level::debug, L"A taskbar got created, refreshing...");
				ResetState();
			}
			else
			{
				InsertWindow(window, true);
			}
		}
		else if (event == EVENT_OBJECT_DESTROY)
		{
			// events are asynchronous, the window might be invalid already
			// important to not try to query its info here, just go off the handle
			AttributeRefresher refresher(*this);
			for (auto it = m_Taskbars.begin(); it != m_Taskbars.end(); ++it)
			{
				if (it->second.Taskbar.TaskbarWindow == window)
				{
					MessagePrint(spdlog::level::debug, L"A taskbar got destroyed, refreshing...");
					ResetState();

					// iterators invalid
					refresher.disarm();
					return;
				}

				RemoveWindow<LogWindowRemovalDestroyed>(window, it, refresher);
			}
		}
	}
}

void TaskbarAttributeWorker::OnWindowOrderChange(DWORD, HWND hwnd, LONG idObject, LONG idChild, DWORD, DWORD)
{
	if (const Window window(hwnd); idObject == OBJID_WINDOW && idChild == CHILDID_SELF && window.valid())
	{
		if (const auto iter = m_Taskbars.find(window.monitor()); iter != m_Taskbars.end())
		{
			RefreshAttribute(iter);
		}
	}
}

void TaskbarAttributeWorker::OnTaskViewVisibilityChange(bool state)
{
	m_TaskViewActive = state;
	MessagePrint(spdlog::level::debug, m_TaskViewActive ? L"Task View opened" : L"Task View closed");

	RefreshAllAttributes();
}

void TaskbarAttributeWorker::OnForceRefreshTaskbar(Window taskbar)
{
	if (!m_TaskbarService)
	{
		m_disableAttributeRefreshReply = true;
		auto guard = wil::scope_exit([this]
		{
			m_disableAttributeRefreshReply = false;
		});

		taskbar.send_message(WM_DWMCOMPOSITIONCHANGED);
		guard.reset();
		taskbar.send_message(WM_DWMCOMPOSITIONCHANGED);
	}
}

LRESULT TaskbarAttributeWorker::OnSystemSettingsChange(UINT uiAction)
{
	if (uiAction == SPI_SETWORKAREA)
	{
		MessagePrint(spdlog::level::debug, L"Work area change detected, refreshing...");
		ResetState();
	}

	return 0;
}

LRESULT TaskbarAttributeWorker::OnRequestAttributeRefresh(LPARAM lParam)
{
	if (!m_disableAttributeRefreshReply && !m_TaskbarService)
	{
		const Window window = reinterpret_cast<HWND>(lParam);
		if (const auto iter = m_Taskbars.find(window.monitor()); iter != m_Taskbars.end() && iter->second.Taskbar.TaskbarWindow == window)
		{
			if (const auto appearance = GetAppearance(iter); appearance.Accent != ACCENT_NORMAL)
			{
				SetAttribute(iter, appearance);
				return 1;
			}
		}
	}

	return 0;
}

LRESULT TaskbarAttributeWorker::MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_SETTINGCHANGE)
	{
		if (InSendMessage())
		{
			// post the message back to ourself to process outside of SendMessage
			post_message(uMsg, wParam, lParam);
			return 0;
		}
		else
		{
			return OnSystemSettingsChange(static_cast<UINT>(wParam));
		}
	}
	else if (uMsg == WM_DISPLAYCHANGE)
	{
		MessagePrint(spdlog::level::debug, L"Monitor configuration change detected, refreshing...");
		ResetState();
		return 0;
	}
	else if (uMsg == m_TaskbarCreatedMessage)
	{
		MessagePrint(spdlog::level::debug, L"Main taskbar got created, refreshing...");
		ResetState();
		return 0;
	}
	else if (uMsg == m_RefreshRequestedMessage)
	{
		return OnRequestAttributeRefresh(lParam);
	}
	else if (uMsg == m_TaskViewVisibilityChangeMessage)
	{
		OnTaskViewVisibilityChange(wParam);
		return 0;
	}
	else if (uMsg == m_ForceRefreshTaskbar)
	{
		OnForceRefreshTaskbar(reinterpret_cast<HWND>(lParam));
		return 0;
	}
	return MessageWindow::MessageHandler(uMsg, wParam, lParam);
}

TaskbarAppearance TaskbarAttributeWorker::GetAppearance(taskbar_iterator taskbar) const
{
	if (m_TaskViewActive)
	{
		return TASK_VIEW_APPEARANCE;
	}

	// Task View is ignored by peek, so shall we
	if (m_PeekActive)
	{
		return DESKTOP_APPEARANCE;
	}

	if (!taskbar->second.MaximisedWindows.empty())
	{
		return MAXIMISED_APPEARANCE;
	}

	if (!taskbar->second.NormalWindows.empty())
	{
		return WINDOW_APPEARANCE;
	}

	return DESKTOP_APPEARANCE;
}

void TaskbarAttributeWorker::ShowAeroPeekButton(const TaskbarInfo &taskbar, bool show)
{
	if (const auto style = taskbar.PeekWindow.get_long_ptr(GWL_EXSTYLE))
	{
		const bool success = SetNewWindowExStyle(taskbar.PeekWindow, *style, show
			? (*style & ~WS_EX_LAYERED)
			: (*style | WS_EX_LAYERED));

		if (!show && success)
		{
			// Non-zero alpha makes the button still interactible, even if practically invisible.
			if (!SetLayeredWindowAttributes(taskbar.PeekWindow, 0, 1, LWA_ALPHA))
			{
				LastErrorHandle(spdlog::level::warn, L"Failed to set peek button layered attributes");
			}
		}
	}
}

void TaskbarAttributeWorker::SetAttribute(taskbar_iterator taskbar, TaskbarAppearance config)
{
	if (m_TaskbarService)
	{
		if (config.Accent == ACCENT_NORMAL)
		{
			HresultVerify(m_TaskbarService->ReturnTaskbarToDefaultAppearance(taskbar->second.Taskbar.TaskbarWindow), spdlog::level::info, L"Failed to restore taskbar to normal");
		}
		else if (config.Accent == ACCENT_ENABLE_BLURBEHIND)
		{
			HresultVerify(m_TaskbarService->SetTaskbarBlur(taskbar->second.Taskbar.TaskbarWindow, config.Color.ToABGR(), config.BlurRadius / 3), spdlog::level::info, L"Failed to set taskbar brush");
		}
		else
		{
			auto color = config.Color;
			TaskbarBrush brush = SolidColor;

			if (config.Accent == ACCENT_ENABLE_ACRYLICBLURBEHIND)
			{
				brush = Acrylic;
			}
			else if (config.Accent == ACCENT_ENABLE_GRADIENT)
			{
				color.A = 0xFF;
			}

			HresultVerify(m_TaskbarService->SetTaskbarAppearance(taskbar->second.Taskbar.TaskbarWindow, brush, color.ToABGR()), spdlog::level::info, L"Failed to set taskbar brush");
		}
	}
	else
	{
		const auto window = taskbar->second.Taskbar.TaskbarWindow;

		if (config.Accent != ACCENT_NORMAL)
		{
			m_NormalTaskbars.erase(window);

			const bool isAcrylic = config.Accent == ACCENT_ENABLE_ACRYLICBLURBEHIND;
			if (isAcrylic && config.Color.A == 0)
			{
				// Acrylic mode doesn't likes a completely 0 opacity
				config.Color.A = 1;
			}

			ACCENT_POLICY policy = {
				config.Accent,
				static_cast<UINT>(isAcrylic ? 0 : 2),
				config.Color.ToABGR(),
				0
			};

			const WINDOWCOMPOSITIONATTRIBDATA data = {
				WCA_ACCENT_POLICY,
				&policy,
				sizeof(policy)
			};

			if (!SetWindowCompositionAttribute(window, &data)) [[unlikely]]
			{
				LastErrorHandle(spdlog::level::info, L"Failed to set window composition attribute");
			}
		}
		else if (const auto [it, inserted] = m_NormalTaskbars.insert(window); inserted)
		{
			// If this is in response to a window being moved, we send the message way too often
			// and Explorer doesn't like that too much.
			window.send_message(WM_DWMCOMPOSITIONCHANGED, 1, 0);
		}
	}
}

void TaskbarAttributeWorker::RefreshAttribute(taskbar_iterator taskbar)
{
	// These functions may trigger Windows internal message loops,
	// do not pass any member of taskbar map by reference.
	// See comment in InsertWindow.
	const auto taskbarInfo = taskbar->second.Taskbar;

	const auto cfg = GetAppearance(taskbar);
	SetAttribute(taskbar, cfg);

	if (m_TaskbarService)
	{
		HresultVerify(m_TaskbarService->SetTaskbarBorderVisibility(taskbarInfo.TaskbarWindow, cfg.ShowLine), spdlog::level::info, L"Failed to set taskbar border visibility");
	}
	else if (taskbarInfo.PeekWindow)
	{
		// Ignore changes when peek is active
		if (!m_PeekActive)
		{
			ShowAeroPeekButton(taskbarInfo, cfg.ShowPeek);
		}
	}
}

void TaskbarAttributeWorker::RefreshAllAttributes()
{
	AttributeRefresher refresher(*this);
	for (auto it = m_Taskbars.begin(); it != m_Taskbars.end(); ++it)
	{
		refresher.refresh(it);
	}
}

void TaskbarAttributeWorker::LogWindowInsertion(const std::pair<std::unordered_set<Window>::iterator, bool> &result, std::wstring_view state, HMONITOR mon)
{
	if (result.second && Error::ShouldLog<spdlog::level::debug>())
	{
		MessagePrint(spdlog::level::debug, std::format(L"Inserting {} window {} to monitor {}", state, DumpWindow(*result.first), static_cast<void *>(mon)));
	}
}

void TaskbarAttributeWorker::LogWindowRemoval(std::wstring_view state, Window window, HMONITOR mon)
{
	if (Error::ShouldLog<spdlog::level::debug>())
	{
		MessagePrint(spdlog::level::debug, std::format(L"Removing {} window {} from monitor {}", state, DumpWindow(window), static_cast<void *>(mon)));
	}
}

void TaskbarAttributeWorker::LogWindowRemovalDestroyed(std::wstring_view state, Window window, HMONITOR mon)
{
	if (Error::ShouldLog<spdlog::level::debug>())
	{
		MessagePrint(spdlog::level::debug, std::format(L"Removing {} window {} [window destroyed] from monitor {}", state, static_cast<void *>(window.handle()), static_cast<void *>(mon)));
	}
}

void TaskbarAttributeWorker::InsertWindow(Window window, bool refresh)
{
	if (window.classname() == CORE_WINDOW) [[unlikely]]
	{
		// Windows.UI.Core.CoreWindow is always shell UI stuff
		// that we either have a dynamic mode for or should ignore.
		// so just skip it.
		return;
	}

	AttributeRefresher refresher(*this, refresh);

	// Note: The checks are done before iterating because
	// some methods (most notably Window::on_current_desktop)
	// will trigger a Windows internal message loop,
	// which pumps messages to the worker. When the DPI is
	// changing, it means m_Taskbars is cleared while we still
	// have an iterator to it. Acquiring the iterator after the
	// call to on_current_desktop resolves this issue.
	const bool windowMatches = window.is_user_window();
	const bool isMaximised = window.maximised();
	const bool isMinimised = window.minimised();
	const HMONITOR mon = window.monitor();

	for (auto it = m_Taskbars.begin(); it != m_Taskbars.end(); ++it)
	{
		auto &maximised = it->second.MaximisedWindows;
		auto &normal = it->second.NormalWindows;

		if (it->first == mon)
		{
			if (windowMatches && isMaximised)
			{
				if (normal.erase(window) > 0)
				{
					LogWindowRemoval(L"normal", window, mon);
				}

				LogWindowInsertion(maximised.insert(window), L"maximised", mon);

				refresher.refresh(it);
				continue;
			}
			else if (windowMatches && !isMinimised)
			{
				if (maximised.erase(window) > 0)
				{
					LogWindowRemoval(L"maximised", window, mon);
				}

				LogWindowInsertion(normal.insert(window), L"normal", mon);

				refresher.refresh(it);
				continue;
			}

			// fall out the if if the window is minimized
		}

		RemoveWindow(window, it, refresher);
	}
}

template<void(*logger)(std::wstring_view, Window, HMONITOR)>
void TaskbarAttributeWorker::RemoveWindow(Window window, taskbar_iterator it, AttributeRefresher& refresher)
{
	bool erased = false;
	if (it->second.MaximisedWindows.erase(window) > 0)
	{
		logger(L"maximised", window, it->first);
		erased = true;
	}

	if (it->second.NormalWindows.erase(window) > 0)
	{
		logger(L"normal", window, it->first);
		erased = true;
	}

	// only refresh the taskbar once in the case the window is in both
	if (erased)
	{
		refresher.refresh(it);
	}
}

bool TaskbarAttributeWorker::SetNewWindowExStyle(Window wnd, LONG_PTR oldStyle, LONG_PTR newStyle)
{
	if (oldStyle != newStyle)
	{
		return wnd.set_long_ptr(GWL_EXSTYLE, newStyle).has_value();
	}
	else
	{
		return true;
	}
}

std::wstring TaskbarAttributeWorker::DumpWindow(Window window)
{
	if (window)
	{
		std::wstring title, className, fileName;
		if (auto titleOpt = window.title())
		{
			title = std::move(*titleOpt);
			if (auto classNameOpt = window.classname())
			{
				className = std::move(*classNameOpt);
				if (const auto fileOpt = window.file())
				{
					fileName = fileOpt->filename().native();
				}
			}
		}

		return std::format(L"{} [{}] [{}] [{}]", static_cast<void *>(window.handle()), title, className, fileName);
	}
	else
	{
		return L"0x0";
	}
}

void TaskbarAttributeWorker::CreateTaskViewManager()
{
	UnregisterTaskViewCallbacks();

	m_TaskViewViewCoordinator = nullptr;

	if (m_TaskViewVisibilityChangeMessage && m_IsWindows11)
	{
		try
		{
			using winrt::WindowsUdk::UI::Shell::ShellView;
			using winrt::WindowsUdk::UI::Shell::ShellViewCoordinator;

			m_TaskViewViewCoordinator = ShellViewCoordinator { winrt::WindowsUdk::UI::Shell::ShellView::TaskView };

			m_TaskViewVisibilityChangedToken = m_TaskViewViewCoordinator.VisibilityChanged([this](const ShellViewCoordinator& coordinator, const wf::IInspectable&)
			{
				post_message(*m_TaskViewVisibilityChangeMessage, coordinator.Visibility() == winrt::WindowsUdk::UI::Shell::ViewVisibility::Visible);
			});
		}
		HresultErrorCatch(spdlog::level::warn, L"Failed to create ShellViewCoordinator");
	}
}

void TaskbarAttributeWorker::UnregisterTaskViewCallbacks() noexcept
{
	if (m_TaskViewViewCoordinator)
	{
		if (m_TaskViewVisibilityChangedToken)
		{
			m_TaskViewViewCoordinator.VisibilityChanged(m_TaskViewVisibilityChangedToken);
			m_TaskViewVisibilityChangedToken = { };
		}
	}
}

WINEVENTPROC TaskbarAttributeWorker::CreateThunk(void(CALLBACK TaskbarAttributeWorker:: *proc)(DWORD, HWND, LONG, LONG, DWORD, DWORD))
{
	return m_ThunkPage.make_thunk<WINEVENTPROC>(this, proc);
}

wil::unique_hwineventhook TaskbarAttributeWorker::CreateHook(DWORD eventMin, DWORD eventMax, WINEVENTPROC proc)
{
	if (wil::unique_hwineventhook hook { SetWinEventHook(eventMin, eventMax, nullptr, proc, 0, 0, WINEVENT_OUTOFCONTEXT) })
	{
		return hook;
	}
	else
	{
		MessagePrint(spdlog::level::critical, L"Failed to create a Windows event hook.");
	}
}

void TaskbarAttributeWorker::ReturnToStock()
{
	if (m_TaskbarService)
	{
		HresultVerify(m_TaskbarService->RestoreAllTaskbarsToDefault(), spdlog::level::info, L"Failed to restore taskbars");
	}
	else
	{
		// Copy taskbars to a vector because some functions
		// trigger Windows internal message loop.
		std::vector<TaskbarInfo> taskbarInfos;
		for (auto it = m_Taskbars.begin(); it != m_Taskbars.end(); ++it)
		{
			SetAttribute(it, { ACCENT_NORMAL, { 0, 0, 0, 0 }, true, true, 0.0f });

			taskbarInfos.push_back(it->second.Taskbar);
		}

		for (const auto& taskbarInfo : taskbarInfos)
		{
			if (taskbarInfo.PeekWindow)
			{
				ShowAeroPeekButton(taskbarInfo, true);
			}
		}
	}
}

void TaskbarAttributeWorker::InsertTaskbar(HMONITOR mon, Window window)
{
	TaskbarInfo taskbarInfo = { .TaskbarWindow = window };
	if (m_TaskbarType == TaskbarType::Classic)
	{
		taskbarInfo.PeekWindow = window.find_child(L"TrayNotifyWnd").find_child(L"TrayShowDesktopButtonWClass");
	}

	m_NormalTaskbars.insert(window);
	m_Taskbars.insert_or_assign(mon, MonitorInfo { taskbarInfo });

	if (m_TaskbarType != TaskbarType::XAML)
	{
		if (wil::unique_hhook hook { m_InjectExplorerHook(window) })
		{
			m_Hooks.push_back(std::move(hook));
		}
		else
		{
			LastErrorHandle(spdlog::level::critical, L"Failed to set hook.");
		}
	}
}

BOOL TaskbarAttributeWorker::MonitorEnumProc(HMONITOR hMonitor, HDC, LPRECT lprcMonitor, LPARAM dwData)
{
	const auto info = reinterpret_cast<MonitorEnumInfo*>(&dwData);
	for (const UINT edge : { ABE_BOTTOM, ABE_TOP, ABE_LEFT, ABE_RIGHT })
	{
		APPBARDATA data = { sizeof(data) };
		data.uEdge = edge;
		data.rc = *lprcMonitor;

		HWND hwndAutoHide = reinterpret_cast<HWND>(SHAppBarMessage(ABM_GETAUTOHIDEBAREX, &data));
		if (hwndAutoHide == info->window)
		{
			info->monitor = hMonitor;

			return false;
		}
	}

	return true;
}

HMONITOR TaskbarAttributeWorker::GetTaskbarMonitor(Window taskbar)
{
	MonitorEnumInfo info = { taskbar };
	if (EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, reinterpret_cast<LPARAM>(&info)) && info.monitor)
	{
		return info.monitor;
	}
	else
	{
		return taskbar.monitor();
	}
}

TaskbarType TaskbarAttributeWorker::GetTaskbarType(Window taskbar)
{
	wil::unique_cotaskmem_string system32;
	const HRESULT hr = SHGetKnownFolderPath(FOLDERID_System, KF_FLAG_DEFAULT, nullptr, system32.put());
	if (FAILED(hr)) [[unlikely]]
	{
		HresultHandle(hr, spdlog::level::critical, L"Failed to get System32 path");
	}

	std::filesystem::path taskbarDll = system32.get();
	taskbarDll /= L"Taskbar.dll";

	bool hasTaskbarDll = false;
	wil::unique_tool_help_snapshot snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, taskbar.process_id()));

	MODULEENTRY32 me = { .dwSize = sizeof(me) };

	if (!Module32First(snapshot.get(), &me))
	{
		// this can happen if explorer dies
		return TaskbarType::Unknown;
	}

	do
	{
		if (win32::IsSameFilename(me.szExePath, taskbarDll.native()))
		{
			hasTaskbarDll = true;
			break;
		}
	} while (Module32Next(snapshot.get(), &me));

	snapshot.reset();

	return hasTaskbarDll ? TaskbarType::XAML : TaskbarType::Classic;
}

TaskbarAttributeWorker::TaskbarAttributeWorker(HINSTANCE hInstance, DynamicLoader &loader, const std::optional<std::filesystem::path> &storageFolder) :
	MessageWindow(TTB_WORKERWINDOW, TTB_WORKERWINDOW, hInstance, WS_POPUP, WS_EX_NOREDIRECTIONBITMAP),
	SetWindowCompositionAttribute(loader.SetWindowCompositionAttribute()),
	ShouldSystemUseDarkMode(loader.ShouldSystemUseDarkMode()),
	m_TaskViewActive(false),
	m_PeekActive(false),
	m_disableAttributeRefreshReply(false),
	m_ResettingState(false),
	m_ResetStateReentered(false),
	m_TaskbarType(TaskbarType::Unknown),
	m_ThunkPage(member_thunk::allocate_page()),
	m_PeekUnpeekHook(CreateHook(EVENT_SYSTEM_PEEKSTART, EVENT_SYSTEM_PEEKEND, CreateThunk(&TaskbarAttributeWorker::OnAeroPeekEnterExit))),
	m_CloakUncloakHook(CreateHook(EVENT_OBJECT_CLOAKED, EVENT_OBJECT_UNCLOAKED, CreateThunk(&TaskbarAttributeWorker::WindowInsertRemove<EVENT_OBJECT_UNCLOAKED, EVENT_OBJECT_CLOAKED>))),
	m_MinimizeRestoreHook(CreateHook(EVENT_SYSTEM_MINIMIZESTART, EVENT_SYSTEM_MINIMIZEEND, CreateThunk(&TaskbarAttributeWorker::WindowInsertRemove<EVENT_SYSTEM_MINIMIZEEND, EVENT_SYSTEM_MINIMIZESTART>))),
	m_ShowHideHook(CreateHook(EVENT_OBJECT_SHOW, EVENT_OBJECT_HIDE, CreateThunk(&TaskbarAttributeWorker::WindowInsertRemove<EVENT_OBJECT_SHOW, EVENT_OBJECT_HIDE>))),
	m_CreateDestroyHook(CreateHook(EVENT_OBJECT_CREATE, EVENT_OBJECT_DESTROY, CreateThunk(&TaskbarAttributeWorker::OnWindowCreateDestroy))),
	m_OrderChangeHook(CreateHook(EVENT_OBJECT_REORDER, CreateThunk(&TaskbarAttributeWorker::OnWindowOrderChange))),
	m_TaskViewViewCoordinator(nullptr),
	m_TaskbarCreatedMessage(Window::RegisterMessage(WM_TASKBARCREATED)),
	m_RefreshRequestedMessage(Window::RegisterMessage(WM_TTBHOOKREQUESTREFRESH)),
	m_TaskViewVisibilityChangeMessage(Window::RegisterMessage(WM_TTBHOOKTASKVIEWVISIBILITYCHANGE)),
	m_IsTaskViewOpenedMessage(Window::RegisterMessage(WM_TTBHOOKISTASKVIEWOPENED)),
	m_ForceRefreshTaskbar(Window::RegisterMessage(WM_TTBFORCEREFRESHTASKBAR)),
	m_LastExplorerPid(0),
	m_HookDll(storageFolder, true, L"ExplorerHooks.dll"),
	m_InjectExplorerHook(m_HookDll.GetProc<PFN_INJECT_EXPLORER_HOOK>("InjectExplorerHook")),
	m_TAPDll(storageFolder, true, L"ExplorerTAP.dll"),
	m_InjectExplorerTAP(m_TAPDll.GetProc<PFN_INJECT_EXPLORER_TAP>("InjectExplorerTAP")),
	m_IsWindows11(win32::IsAtLeastBuild(22000))
{
	const auto stateThunk = CreateThunk(&TaskbarAttributeWorker::OnWindowStateChange);
	m_ResizeMoveHook = CreateHook(EVENT_OBJECT_LOCATIONCHANGE, stateThunk);
	m_ParentChangeHook = CreateHook(EVENT_OBJECT_PARENTCHANGE, stateThunk);
	m_ThunkPage.mark_executable();

	// we don't want to consider the first state reset as an Explorer restart.
	ResetState(true);
}

void TaskbarAttributeWorker::ResetState(bool manual)
{
	if (!m_ResettingState)
	{
		MessagePrint(spdlog::level::debug, L"Resetting worker state");

		m_ResettingState = true;
		m_ResetStateReentered = false;
		auto guard = wil::scope_exit([this]
		{
			m_ResettingState = false;
			m_ResetStateReentered = false;
		});

		// Clear state
		m_PeekActive = false;
		m_TaskViewActive = false;

		m_Taskbars.clear();
		m_NormalTaskbars.clear();

		m_TaskbarService = nullptr;

		// Keep old hooks alive while we rehook to avoid DLL unload.
		auto oldHooks = std::move(m_Hooks);
		m_Hooks.clear();

		for (const Window main_taskbar : Window::FindEnum(TASKBAR))
		{
			if (main_taskbar.file().value_or({}).filename() == L"explorer.exe")
			{
				const auto pid = main_taskbar.process_id();
				if (!manual)
				{
					if (m_LastExplorerPid != 0 && pid != m_LastExplorerPid)
					{
						const auto now = std::chrono::steady_clock::now();
						if (now < m_LastExplorerRestart + std::chrono::seconds(30)) [[unlikely]]
						{
							Localization::ShowLocalizedMessageBox(IDS_EXPLORER_RESTARTED_TOO_MUCH, MB_OK | MB_ICONWARNING | MB_SETFOREGROUND, hinstance()).join();
							ExitProcess(1);
						}

						m_LastExplorerRestart = now;
					}
				}

				m_LastExplorerPid = pid;

				m_TaskbarType = GetTaskbarType(main_taskbar);

				if (m_TaskbarType == TaskbarType::XAML)
				{
					const HRESULT hr = m_InjectExplorerTAP(main_taskbar, IID_PPV_ARGS(m_TaskbarService.put()));
					if (hr == HRESULT_FROM_WIN32(ERROR_PRODUCT_VERSION))
					{
						Localization::ShowLocalizedMessageBox(IDS_RESTART_REQUIRED, MB_OK | MB_ICONWARNING | MB_SETFOREGROUND, hinstance()).join();
						ExitProcess(1);
					}
					else
					{
						HresultVerify(hr, spdlog::level::critical, L"Failed to initialize XAML Diagnostics.");
					}

					HresultVerify(m_TaskbarService->RestoreAllTaskbarsToDefaultWhenProcessDies(GetCurrentProcessId()), spdlog::level::warn, L"Couldn't configure TAP to restore taskbar appearance once " APP_NAME L" dies.");

					if (const auto fullName = UWP::GetPackageFullName())
					{
						HresultVerify(m_TaskbarService->KillExplorerWhenPackageUninstalls(fullName->c_str()), spdlog::level::warn, L"Couldn't configure TAP to kill Explorer once " APP_NAME L" is uninstalled.");
					}
				}
				else if (m_TaskbarType == TaskbarType::Unknown)
				{
					// unknown taskbar type - we might've detected explorer too early. do nothing for now. we should get a refresh later and be able to detect it.
					return;
				}

				InsertTaskbar(GetTaskbarMonitor(main_taskbar), main_taskbar);
				break; // only one main taskbar.
			}
		}

		for (const Window secondtaskbar : Window::FindEnum(SECONDARY_TASKBAR))
		{
			InsertTaskbar(GetTaskbarMonitor(secondtaskbar), secondtaskbar);
		}

		// drop the old hooks.
		oldHooks.clear();

		CreateTaskViewManager();

		// This might race but it's not an issue because
		// it'll race on a single thread and only ends up
		// doing something twice.

		// TODO: check if aero peek is active

		if (m_TaskViewViewCoordinator)
		{
			m_TaskViewActive = m_TaskViewViewCoordinator.Visibility() == winrt::WindowsUdk::UI::Shell::ViewVisibility::Visible;
		}
		else if (const auto hookWnd = Window::Find(TTBHOOK_TASKVIEWMONITOR, TTBHOOK_TASKVIEWMONITOR); hookWnd && m_IsTaskViewOpenedMessage)
		{
			m_TaskViewActive = hookWnd.send_message(*m_IsTaskViewOpenedMessage);
		}

		for (const Window window : Window::FindEnum())
		{
			InsertWindow(window, false);
		}

		if (!m_ResetStateReentered)
		{
			// Apply the calculated effects
			RefreshAllAttributes();
		}
		else
		{
			// we got re-entrancy, all this might not even be valid anymore, try it again.
			guard.reset();
			ResetState(manual);
		}
	}
	else
	{
		MessagePrint(spdlog::level::debug, L"ResetState re-entrancy detected");

		m_ResetStateReentered = true;
	}
}

TaskbarAttributeWorker::~TaskbarAttributeWorker() noexcept(false)
{
	m_disableAttributeRefreshReply = true;
	UnregisterTaskViewCallbacks();
	ReturnToStock();
}
