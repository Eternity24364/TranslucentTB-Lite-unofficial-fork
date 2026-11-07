#pragma once
#include <filesystem>
#include <optional>
#include <string>

namespace UWP {
	std::optional<std::wstring> GetPackageFamilyName();
	std::optional<std::wstring> GetPackageFullName();
	std::optional<std::filesystem::path> GetAppStorageFolder();
}
