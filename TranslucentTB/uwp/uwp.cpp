#include "uwp.hpp"
#include <Windows.h>
#include <appmodel.h>
#include <ShlObj_core.h>
#include <wil/resource.h>

#include "../ProgramLog/error/win32.hpp"

std::optional<std::wstring> UWP::GetPackageFamilyName()
{
	static constexpr std::wstring_view FAILED_TO_GET = L"Failed to get package family name";

	UINT32 length = 0;
	LONG result = GetCurrentPackageFamilyName(&length, nullptr);
	if (result != ERROR_INSUFFICIENT_BUFFER) [[unlikely]]
	{
		if (result == APPMODEL_ERROR_NO_PACKAGE)
		{
			return std::nullopt;
		}
		else
		{
			HresultHandle(HRESULT_FROM_WIN32(result), spdlog::level::critical, FAILED_TO_GET);
		}
	}

	std::wstring familyName;
	familyName.resize_and_overwrite(length - 1, [](wchar_t* data, std::size_t count)
	{
		UINT32 length = static_cast<UINT32>(count) + 1;
		const LONG result = GetCurrentPackageFamilyName(&length, data);
		if (result != ERROR_SUCCESS) [[unlikely]]
		{
			HresultHandle(HRESULT_FROM_WIN32(result), spdlog::level::critical, FAILED_TO_GET);
		}

		return length - 1;
	});

	return familyName;
}

std::optional<std::wstring> UWP::GetPackageFullName()
{
	static constexpr std::wstring_view FAILED_TO_GET = L"Failed to get package full name";

	UINT32 length = 0;
	LONG result = GetCurrentPackageFullName(&length, nullptr);
	if (result != ERROR_INSUFFICIENT_BUFFER) [[unlikely]]
	{
		if (result == APPMODEL_ERROR_NO_PACKAGE)
		{
			return std::nullopt;
		}
		else
		{
			HresultHandle(HRESULT_FROM_WIN32(result), spdlog::level::critical, FAILED_TO_GET);
		}
	}

	std::wstring familyName;
	familyName.resize_and_overwrite(length - 1, [](wchar_t* data, std::size_t count)
		{
			UINT32 length = static_cast<UINT32>(count) + 1;
			const LONG result = GetCurrentPackageFullName(&length, data);
			if (result != ERROR_SUCCESS) [[unlikely]]
			{
				HresultHandle(HRESULT_FROM_WIN32(result), spdlog::level::critical, FAILED_TO_GET);
			}

			return length - 1;
		});

	return familyName;
}

std::optional<std::filesystem::path> UWP::GetAppStorageFolder()
{
	if (const auto familyName = UWP::GetPackageFamilyName())
	{
		wil::unique_cotaskmem_string appdata;
		HresultVerify(
			SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_NO_PACKAGE_REDIRECTION, nullptr, appdata.put()),
			spdlog::level::critical,
			L"Failed to get local app data folder"
		);

		std::filesystem::path storage = appdata.get();
		storage /= L"Packages";
		storage /= *familyName;
		return std::move(storage);
	}
	else
	{
		return std::nullopt;
	}
}
