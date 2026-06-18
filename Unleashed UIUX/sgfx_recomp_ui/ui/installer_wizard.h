#pragma once

// DECOUPLED: <api/SWA.h> (only pulled in for std::filesystem / std types) -> the
// std header the struct's Run() signature actually needs. Body unchanged.
#include <filesystem>

struct InstallerWizard
{
    static inline bool s_isVisible = false;

    static void Init();
    static void Draw();
    static void Shutdown();
    static bool Run(std::filesystem::path installPath, bool skipGame);
};
