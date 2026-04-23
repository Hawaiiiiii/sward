#pragma once

#include <filesystem>
#include <vector>

#include <sward/ui_runtime/profiles.hpp>

namespace sward::ui_runtime
{
[[nodiscard]] ScreenContract loadContractFromJsonFile(const std::filesystem::path& path);
[[nodiscard]] ScreenContract loadBundledContract(ReferenceProfile profile);
[[nodiscard]] std::filesystem::path bundledContractPath(ReferenceProfile profile);
[[nodiscard]] std::vector<std::filesystem::path> bundledContractPaths();
} // namespace sward::ui_runtime
