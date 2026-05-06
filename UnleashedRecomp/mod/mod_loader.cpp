#include "mod_loader.h"
#include "ini_file.h"

#include <api/Hedgehog/Base/System/hhAllocator.h>
#include <cpu/guest_stack_var.h>
#include <kernel/function.h>
#include <kernel/heap.h>
#include <user/config.h>
#include <user/paths.h>
#include <os/logger.h>
#include <os/process.h>
#include <patches/ui_lab_patches.h>
#include <xxHashMap.h>

enum class ModType
{
    HMM,
    UMM
};

struct Mod
{
    ModType type{};
    std::vector<std::filesystem::path> includeDirs;
    bool merge = false;
    ankerl::unordered_dense::set<std::filesystem::path> readOnly;
};

static std::vector<Mod> g_mods;
static std::filesystem::path g_sgPreflightOverrideDir;
static ankerl::unordered_dense::map<std::string, std::filesystem::path> g_sgPreflightLooseOverrideIndex;
static ankerl::unordered_dense::set<std::string> g_sgPreflightObservedArchiveEntries;

static std::string NormalizeSgPreflightAssetKey(std::string_view value)
{
    std::string key(value);
    std::replace(key.begin(), key.end(), '\\', '/');
    std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c)
    {
        return static_cast<char>(std::tolower(c));
    });
    return key;
}

static void IndexSgPreflightLooseOverrides(const std::filesystem::path& overrideDir)
{
    g_sgPreflightLooseOverrideIndex.clear();

    std::error_code ec;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(overrideDir, ec))
    {
        if (ec) break;
        if (!entry.is_regular_file(ec)) continue;

        const auto path = entry.path();
        const auto filename = path.filename().u8string();
        if (filename == u8"sg_text_overrides.json" || filename == u8"README.md")
            continue;

        const auto rel = path.lexically_relative(overrideDir).generic_u8string();
        const std::string relKey = NormalizeSgPreflightAssetKey(
            std::string_view(reinterpret_cast<const char*>(rel.data()), rel.size()));
        g_sgPreflightLooseOverrideIndex[relKey] = path;

        const std::string fileKey = NormalizeSgPreflightAssetKey(
            std::string_view(reinterpret_cast<const char*>(filename.data()), filename.size()));
        auto [it, inserted] = g_sgPreflightLooseOverrideIndex.emplace(fileKey, path);
        if (!inserted && it->second != path)
            it->second.clear();
    }
}

static bool IsSgPreflightLoadLoggingEnabled()
{
    const char* logEnv = std::getenv("SG_PREFLIGHT_LOG_LOADS");
    return logEnv != nullptr && std::string_view(logEnv) != "0";
}

static void EmitSgPreflightArchiveEntryProbe(std::string_view assetName)
{
    if (!IsSgPreflightLoadLoggingEnabled() || assetName.empty()) return;
    if (g_sgPreflightObservedArchiveEntries.size() >= 128) return;

    std::string key(assetName);
    std::replace(key.begin(), key.end(), '\\', '/');
    if (!g_sgPreflightObservedArchiveEntries.emplace(key).second) return;

    UiLab::EmitBridgeScreenEntered("Asset:ArchiveEntry:" + key);
}

std::filesystem::path ModLoader::ResolvePath(std::string_view path)
{
    std::string_view root;

    size_t sepIndex = path.find(":\\");
    if (sepIndex != std::string_view::npos)
    {
        root = path.substr(0, sepIndex);
        path.remove_prefix(sepIndex + 2);
    }

    if (root == "save")
    {
        if (!ModLoader::s_saveFilePath.empty())
        {
            if (path == "SYS-DATA")
                return ModLoader::s_saveFilePath;
            else
                return ModLoader::s_saveFilePath.parent_path() / path;
        }

        return {};
    }

    if (g_mods.empty())
        return {};

    thread_local xxHashMap<std::filesystem::path> s_cache;

    XXH64_hash_t hash = XXH3_64bits(path.data(), path.size());
    auto findResult = s_cache.find(hash);
    if (findResult != s_cache.end())
        return findResult->second;

    std::string pathStr(path);
    std::replace(pathStr.begin(), pathStr.end(), '\\', '/');
    std::filesystem::path fsPath(std::move(pathStr));

    bool canBeMerged = 
        path.find(".arl") == (path.size() - 4) ||
        path.find(".ar.") == (path.size() - 6) ||
        path.find(".ar") == (path.size() - 3);

    for (auto& mod : g_mods)
    {
        if (mod.type == ModType::UMM && mod.merge && canBeMerged && !mod.readOnly.contains(fsPath))
            continue;

        for (auto& includeDir : mod.includeDirs)
        {
            std::filesystem::path modPath = includeDir / fsPath;
            if (std::filesystem::exists(modPath))
                return s_cache.emplace(hash, modPath).first->second;
        }
    }

    return s_cache.emplace(hash, std::filesystem::path{}).first->second;
}

std::vector<std::filesystem::path>* ModLoader::GetIncludeDirectories(size_t modIndex)
{
    return modIndex < g_mods.size() ? &g_mods[modIndex].includeDirs : nullptr;
}

// Phase 359: SG-Preflight override mount.
//
// In addition to the standard HMM/UMM mod stack (cpkredir.ini-based),
// honor a host-supplied override directory. This is the path
// sg-preflight uses to swap retail Sonic Unleashed assets for
// BMW / Seriengrafik replacements one file at a time without
// authoring a full HMM mod manifest.
//
// Activation precedence (first match wins):
//   1. SG_PREFLIGHT_OVERRIDE_DIR environment variable (absolute path)
//   2. <userPath>/sg_preflight_overrides/  (auto-detected if exists)
//
// Layout: drop replacement files under the same root-relative path
// requested by the guest. For example, `game:\Title\ui_title.yncp`
// resolves as:
//   <override-dir>/Title/ui_title.yncp
//
// If you are copying from the extracted install tree, that usually
// means setting SG_PREFLIGHT_OVERRIDE_DIR to the extracted `game`
// folder or mirroring the root-relative folders under your override dir.
//
// e.g.
//   <override-dir>/Title/ui_title.yncp
//   <override-dir>/MainMenu/ui_mm_contentstext.dds
//
// The override mod is inserted at the FRONT of g_mods so it takes
// priority over every cpkredir mod. When SG_PREFLIGHT_LOG_LOADS=1
// is also set, every file load is logged with its resolved path
// (override vs install) so you can see exactly what's being
// substituted as you build the override pack incrementally.
static void registerSgPreflightOverrideMod()
{
    std::filesystem::path overrideDir;
    if (const char* env = std::getenv("SG_PREFLIGHT_OVERRIDE_DIR");
        env != nullptr && env[0] != '\0')
    {
        overrideDir = std::filesystem::path(std::u8string_view(
            reinterpret_cast<const char8_t*>(env)));
    }
    else
    {
        const auto candidate = GetUserPath() / "sg_preflight_overrides";
        std::error_code ec;
        if (std::filesystem::is_directory(candidate, ec))
            overrideDir = candidate;
    }

    if (overrideDir.empty()) return;

    std::error_code ec;
    if (!std::filesystem::is_directory(overrideDir, ec)) return;
    g_sgPreflightOverrideDir = std::filesystem::weakly_canonical(overrideDir, ec);
    if (ec)
        g_sgPreflightOverrideDir = overrideDir;
    IndexSgPreflightLooseOverrides(g_sgPreflightOverrideDir);

    Mod mod;
    // UMM with merge=false so every file in the dir is treated as a
    // straight override (no .ar merging gymnastics) -- matches the
    // "drop a file, see it replace retail" expectation.
    mod.type = ModType::UMM;
    mod.merge = false;
    mod.includeDirs.emplace_back(overrideDir);

    // Insert at front so this mod wins against any cpkredir mods
    // that get appended afterwards.
    g_mods.insert(g_mods.begin(), std::move(mod));

    LOGF_IMPL(Utility, "SG-Preflight",
              "override mount active: \"{}\" (priority=top)",
              reinterpret_cast<const char*>(overrideDir.u8string().c_str()));

    // Auto-enable per-load console logging when the override mount
    // is in use; the user wants to see substitution events as they
    // build the override pack. Honor an explicit
    // SG_PREFLIGHT_LOG_LOADS=0 to opt out.
    if (const char* logEnv = std::getenv("SG_PREFLIGHT_LOG_LOADS");
        logEnv == nullptr || std::string_view(logEnv) != "0")
    {
        ModLoader::s_isLogTypeConsole = true;
    }
}

bool ModLoader::IsSgPreflightOverridePath(const std::filesystem::path& path)
{
    if (g_sgPreflightOverrideDir.empty() || path.empty()) return false;

    std::error_code ec;
    const auto base = std::filesystem::weakly_canonical(g_sgPreflightOverrideDir, ec);
    if (ec) return false;

    const auto candidate = std::filesystem::weakly_canonical(path, ec);
    if (ec) return false;

    const auto rel = std::filesystem::relative(candidate, base, ec);
    if (ec || rel.empty()) return false;

    auto it = rel.begin();
    return it != rel.end() && *it != "..";
}

std::filesystem::path ModLoader::ResolveSgPreflightLooseAssetOverride(std::string_view assetName)
{
    if (g_sgPreflightOverrideDir.empty() || assetName.empty()) return {};

    const std::string relKey = NormalizeSgPreflightAssetKey(assetName);
    auto it = g_sgPreflightLooseOverrideIndex.find(relKey);
    if (it != g_sgPreflightLooseOverrideIndex.end())
        return it->second;

    const size_t slash = relKey.find_last_of('/');
    const std::string_view fileKey =
        slash == std::string::npos ? std::string_view(relKey) : std::string_view(relKey).substr(slash + 1);
    it = g_sgPreflightLooseOverrideIndex.find(std::string(fileKey));
    if (it == g_sgPreflightLooseOverrideIndex.end() || it->second.empty())
        return {};

    return it->second;
}

void ModLoader::Init()
{
    const std::filesystem::path& userPath = GetUserPath();

    // Phase 359: register the SG-Preflight override mount FIRST so
    // it takes priority over any cpkredir mods. This makes the mod
    // system usable without authoring an HMM/UMM manifest -- just
    // drop files in the override dir and go.
    registerSgPreflightOverrideMod();

    IniFile configIni;
    if (!configIni.read(userPath / "cpkredir.ini"))
    {
        configIni = {};

        if (!configIni.read(GAME_INSTALL_DIRECTORY "/cpkredir.ini"))
            return;
    }

    if (!configIni.getBool("CPKREDIR", "Enabled", true))
        return;

    if (configIni.getBool("CPKREDIR", "EnableSaveFileRedirection", false))
    {
        std::string saveFilePathU8 = configIni.getString("CPKREDIR", "SaveFileFallback", "");
        if (!saveFilePathU8.empty())
            ModLoader::s_saveFilePath = std::u8string_view((const char8_t*)saveFilePathU8.c_str());
        else
            ModLoader::s_saveFilePath = userPath / "mlsave";

        ModLoader::s_saveFilePath /= "SYS-DATA";
    }

    if (configIni.getString("CPKREDIR", "LogType", std::string()) == "console")
    {
        os::process::ShowConsole();
        s_isLogTypeConsole = true;
    }

    std::string modsDbIniFilePathU8 = configIni.getString("CPKREDIR", "ModsDbIni", "");
    if (modsDbIniFilePathU8.empty())
        return;

    IniFile modsDbIni;
    if (!modsDbIni.read(std::u8string_view((const char8_t*)modsDbIniFilePathU8.c_str())))
        return;

    bool foundModSaveFilePath = false;

    size_t activeModCount = modsDbIni.get<size_t>("Main", "ActiveModCount", 0);
    for (size_t i = 0; i < activeModCount; ++i)
    {
        std::string modId = modsDbIni.getString("Main", fmt::format("ActiveMod{}", i), "");
        if (modId.empty())
            continue;

        std::string modIniFilePathU8 = modsDbIni.getString("Mods", modId, "");
        if (modIniFilePathU8.empty())
            continue;

        std::filesystem::path modIniFilePath(std::u8string_view((const char8_t*)modIniFilePathU8.c_str()));

        IniFile modIni;
        if (!modIni.read(modIniFilePath))
            continue;

        auto modDirectoryPath = modIniFilePath.parent_path();
        std::string modSaveFilePathU8;

        Mod mod;

        if (modIni.contains("Details") || modIni.contains("Filesystem")) // UMM
        {
            mod.type = ModType::UMM;
            mod.includeDirs.emplace_back(modDirectoryPath);
            mod.merge = modIni.getBool("Details", "Merge", modIni.getBool("Filesystem", "Merge", false));

            std::string readOnly = modIni.getString("Details", "Read-only", modIni.getString("Filesystem", "Read-only", std::string()));
            std::replace(readOnly.begin(), readOnly.end(), '\\', '/');
            std::string_view readOnlySplit = readOnly;

            while (!readOnlySplit.empty())
            {
                size_t index = readOnlySplit.find(',');
                if (index == std::string_view::npos)
                {
                    mod.readOnly.emplace(readOnlySplit);
                    break;
                }

                mod.readOnly.emplace(readOnlySplit.substr(0, index));
                readOnlySplit.remove_prefix(index + 1);
            }

            if (!foundModSaveFilePath)
                modSaveFilePathU8 = modIni.getString("Details", "Save", modIni.getString("Filesystem", "Save", std::string()));
        }
        else // HMM
        {
            mod.type = ModType::HMM;

            size_t includeDirCount = modIni.get<size_t>("Main", "IncludeDirCount", 0);
            for (size_t j = 0; j < includeDirCount; j++)
            {
                std::string includeDirU8 = modIni.getString("Main", fmt::format("IncludeDir{}", j), "");
                if (!includeDirU8.empty())
                {
                    std::replace(includeDirU8.begin(), includeDirU8.end(), '\\', '/');
                    mod.includeDirs.emplace_back(modDirectoryPath / std::u8string_view((const char8_t*)includeDirU8.c_str()));
                }
            }

            if (!foundModSaveFilePath)
                modSaveFilePathU8 = modIni.getString("Main", "SaveFile", std::string());
        }

        if (!modSaveFilePathU8.empty())
        {
            std::replace(modSaveFilePathU8.begin(), modSaveFilePathU8.end(), '\\', '/');
            ModLoader::s_saveFilePath = modDirectoryPath / std::u8string_view((const char8_t*)modSaveFilePathU8.c_str());

            // Save file paths in HMM mods are treated as folders.
            if (mod.type == ModType::HMM)
                ModLoader::s_saveFilePath /= "SYS-DATA";

            foundModSaveFilePath = true;
        }

        if (!mod.includeDirs.empty())
            g_mods.emplace_back(std::move(mod));
    }

    auto codeCount = modsDbIni.get<size_t>("Codes", "CodeCount", 0);

    if (codeCount)
    {
        std::vector<std::string> codes{};

        for (size_t i = 0; i < codeCount; i++)
        {
            auto name = modsDbIni.getString("Codes", fmt::format("Code{}", i), "");

            if (name.empty())
                continue;

            codes.push_back(name);
        }

        for (auto& def : g_configDefinitions)
        {
            if (!def->IsHidden() || def->GetSection() != "Codes")
                continue;

            /* NOTE: this is inefficient, but it happens
               once on boot for a handful of codes at release
               and is temporary until we support real code mods. */
            for (size_t i = 0; i < codes.size(); i++)
            {
                if (def->GetName() == codes[i])
                {
                    LOGF_IMPL(Utility, "Mod Loader", "Loading code: \"{}\"", codes[i]);
                    *(bool*)def->GetValue() = true;
                    break;
                }
            }
        }
    }
}

static constexpr uint32_t LZX_SIGNATURE = 0xFF512EE;

static std::span<uint8_t> decompressLzx(PPCContext& ctx, uint8_t* base, const uint8_t* compressedData, size_t compressedDataSize, be<uint32_t>* scratchSpace)
{
    assert(g_memory.IsInMemoryRange(compressedData));

    bool shouldFreeScratchSpace = false;
    if (scratchSpace == nullptr)
    {
        scratchSpace = reinterpret_cast<be<uint32_t>*>(g_userHeap.Alloc(sizeof(uint32_t) * 2));
        shouldFreeScratchSpace = true;
    }

    // Initialize decompressor
    ctx.r3.u32 = 1;
    ctx.r4.u32 = uint32_t((compressedData + 0xC) - base);
    ctx.r5.u32 = *reinterpret_cast<const be<uint32_t>*>(compressedData + 0x8);
    ctx.r6.u32 = uint32_t(reinterpret_cast<uint8_t*>(scratchSpace) - base);
    sub_831CE1A0(ctx, base);

    uint64_t decompressedDataSize = *reinterpret_cast<const be<uint64_t>*>(compressedData + 0x18);
    uint8_t* decompressedData = reinterpret_cast<uint8_t*>(g_userHeap.Alloc(decompressedDataSize));

    uint32_t blockSize = *reinterpret_cast<const be<uint32_t>*>(compressedData + 0x28);
    size_t decompressedDataOffset = 0;
    size_t compressedDataOffset = 0x30;

    while (decompressedDataOffset < decompressedDataSize)
    {
        size_t decompressedBlockSize = decompressedDataSize - decompressedDataOffset;

        if (decompressedBlockSize > blockSize)
            decompressedBlockSize = blockSize;

        *(scratchSpace + 1) = decompressedBlockSize;

        uint32_t compressedBlockSize = *reinterpret_cast<const be<uint32_t>*>(compressedData + compressedDataOffset);

        // Decompress
        ctx.r3.u32 = *scratchSpace;
        ctx.r4.u32 = uint32_t((decompressedData + decompressedDataOffset) - base);
        ctx.r5.u32 = uint32_t(reinterpret_cast<uint8_t*>(scratchSpace + 1) - base);
        ctx.r6.u32 = uint32_t((compressedData + compressedDataOffset + 0x4) - base);
        ctx.r7.u32 = compressedBlockSize;
        sub_831CE0D0(ctx, base);

        decompressedDataOffset += *(scratchSpace + 1);
        compressedDataOffset += 0x4 + compressedBlockSize;
    }

    // Deinitialize decompressor
    ctx.r3.u32 = *scratchSpace;
    sub_831CE150(ctx, base);

    if (shouldFreeScratchSpace)
        g_userHeap.Free(scratchSpace);

    return { decompressedData, decompressedDataSize };
}

// Hedgehog::Database::CDatabaseLoader::ReadArchiveList
PPC_FUNC_IMPL(__imp__sub_82E0D3E8);
PPC_FUNC(sub_82E0D3E8)
{
    if (g_mods.empty())
    {
        __imp__sub_82E0D3E8(ctx, base);
        return;
    }

    thread_local ankerl::unordered_dense::set<std::string> s_fileNames;
    s_fileNames.clear();

    auto parseArlFileData = [&](const uint8_t* arlFileData, size_t arlFileSize)
        {
            struct ArlHeader
            {
                uint32_t signature;
                uint32_t splitCount;
            };

            auto* arlHeader = reinterpret_cast<const ArlHeader*>(arlFileData);
            size_t arlHeaderSize = sizeof(ArlHeader) + arlHeader->splitCount * sizeof(uint32_t);
            const uint8_t* arlFileNames = arlFileData + arlHeaderSize;

            while (arlFileNames < arlFileData + arlFileSize)
            {
                uint8_t fileNameSize = *arlFileNames;
                ++arlFileNames;

                s_fileNames.emplace(reinterpret_cast<const char*>(arlFileNames), fileNameSize);

                arlFileNames += fileNameSize;
            }

            return arlHeaderSize;
        };

    auto parseArFileData = [&](const uint8_t* arFileData, size_t arFileSize)
        {
            struct ArEntry
            {
                uint32_t entrySize;
                uint32_t dataSize;
                uint32_t dataOffset;
                uint32_t fileDateLow;
                uint32_t fileDateHigh;
            };

            for (size_t i = 16; i < arFileSize; )
            {
                auto entry = reinterpret_cast<const ArEntry*>(arFileData + i);
                s_fileNames.emplace(reinterpret_cast<const char*>(entry + 1));
                i += entry->entrySize;
            }
        };

    auto r3 = ctx.r3;
    auto r4 = ctx.r4;
    auto r5 = ctx.r5;
    auto r6 = ctx.r6;

    auto loadFile = [&]<typename TFunction>(const std::filesystem::path& filePath, const TFunction& function)
    {
        std::ifstream stream(filePath, std::ios::binary);
        if (stream.good())
        {
            if (ModLoader::s_isLogTypeConsole)
                LOGF_IMPL(Utility, "Mod Loader", "Loading file: \"{}\"", reinterpret_cast<const char*>(filePath.u8string().c_str()));

            be<uint32_t> signature{};
            stream.read(reinterpret_cast<char*>(&signature), sizeof(signature));

            stream.seekg(0, std::ios::end);
            size_t arlFileSize = stream.tellg();
            stream.seekg(0, std::ios::beg);

            if (signature == LZX_SIGNATURE)
            {
                void* compressedFileData = g_userHeap.Alloc(arlFileSize);
                stream.read(reinterpret_cast<char*>(compressedFileData), arlFileSize);
                stream.close();

                auto fileData = decompressLzx(ctx, base, reinterpret_cast<uint8_t*>(compressedFileData), arlFileSize, nullptr);

                g_userHeap.Free(compressedFileData);

                function(fileData.data(), fileData.size());

                g_userHeap.Free(fileData.data());
            }
            else
            {
                thread_local std::vector<uint8_t> s_fileData;

                s_fileData.resize(arlFileSize);
                stream.read(reinterpret_cast<char*>(s_fileData.data()), arlFileSize);
                stream.close();

                function(s_fileData.data(), arlFileSize);
            }

            return true;
        }

        return false;
    };

    thread_local xxHashMap<std::vector<std::pair<std::filesystem::path, bool>>> s_cache;

    std::u8string_view arlFilePathU8(reinterpret_cast<const char8_t*>(base + PPC_LOAD_U32(ctx.r4.u32)));
    XXH64_hash_t hash = XXH3_64bits(arlFilePathU8.data(), arlFilePathU8.size());
    auto findResult = s_cache.find(hash);

    if (findResult != s_cache.end())
    {
        for (const auto& [arlFilePath, isArchiveList] : findResult->second)
        {
            if (isArchiveList)
                loadFile(arlFilePath, parseArlFileData);
            else
                loadFile(arlFilePath, parseArFileData);
        }
    }
    else
    {
        std::vector<std::pair<std::filesystem::path, bool>> arlFilePaths;
        std::filesystem::path arlFilePath;
        std::filesystem::path arFilePath;
        std::filesystem::path appendArlFilePath;

        for (auto& mod : g_mods)
        {
            for (auto& includeDir : mod.includeDirs)
            {
                auto loadUncachedFile = [&](const std::filesystem::path& filePath, bool isArchiveList)
                {
                    if (mod.type == ModType::UMM && mod.readOnly.contains(filePath))
                        return false;

                    std::filesystem::path combinedFilePath = includeDir / filePath;

                    bool success;
                    if (isArchiveList)
                        success = loadFile(combinedFilePath, parseArlFileData);
                    else
                        success = loadFile(combinedFilePath, parseArFileData);

                    if (success)
                        arlFilePaths.emplace_back(std::move(combinedFilePath), isArchiveList);

                    return success;
                };

                if (mod.type == ModType::UMM)
                {
                    if (mod.merge)
                    {
                        if (arlFilePath.empty())
                        {
                            arlFilePath = arlFilePathU8;
                            arlFilePath += ".arl";
                        }

                        if (!loadUncachedFile(arlFilePath, true))
                        {
                            if (arFilePath.empty())
                            {
                                arFilePath = arlFilePathU8;
                                arFilePath += ".ar";
                            }

                            if (!loadUncachedFile(arFilePath, false))
                            {
                                thread_local std::filesystem::path s_tempPath;

                                for (uint32_t i = 0; ; i++)
                                {
                                    s_tempPath = arFilePath;
                                    s_tempPath += fmt::format(".{:02}", i);

                                    if (!loadUncachedFile(s_tempPath, false))
                                        break;
                                }
                            }
                        }
                    }
                }
                else if (mod.type == ModType::HMM)
                {
                    if (appendArlFilePath.empty())
                    {
                        if (arlFilePath.empty())
                        {
                            arlFilePath = arlFilePathU8;
                            arlFilePath += ".arl";
                        }

                        appendArlFilePath = arlFilePath.parent_path();
                        appendArlFilePath /= "+";
                        appendArlFilePath += arlFilePath.filename();
                    }

                    loadUncachedFile(appendArlFilePath, true);
                }
            }
        }

        s_cache.emplace(hash, std::move(arlFilePaths));
    }

    ctx.r3 = r3;
    ctx.r4 = r4;
    ctx.r5 = r5;
    ctx.r6 = r6;

    if (s_fileNames.empty())
    {
        __imp__sub_82E0D3E8(ctx, base);
        return;
    }

    size_t arlHeaderSize = parseArlFileData(base + ctx.r5.u32, ctx.r6.u32);
    size_t arlFileSize = arlHeaderSize;

    for (auto& fileName : s_fileNames)
    {
        arlFileSize += 1;
        arlFileSize += fileName.size();
    }

    uint8_t* newArlFileData = reinterpret_cast<uint8_t*>(g_userHeap.Alloc(arlFileSize));
    memcpy(newArlFileData, base + ctx.r5.u32, arlHeaderSize);

    uint8_t* arlFileNames = newArlFileData + arlHeaderSize;
    for (auto& fileName : s_fileNames)
    {
        *arlFileNames = uint8_t(fileName.size());
        ++arlFileNames;
        memcpy(arlFileNames, fileName.data(), fileName.size());
        arlFileNames += fileName.size();
    }

    ctx.r5.u32 = uint32_t(newArlFileData - base);
    ctx.r6.u32 = uint32_t(arlFileSize);

    __imp__sub_82E0D3E8(ctx, base);

    g_userHeap.Free(newArlFileData);
}

// Load elements have an unused "pretty name" field. We will use this field to store the archive file path,
// prefixed with a magic string. When the first load detects this string, it will load append archives
// and then clear the field to prevent remaining splits from loading the append archives again.
// We cannot rely on .ar.00 being the first split to be loaded, so this approach is necessary.
static thread_local uint32_t g_prefixedArFilePath = NULL;

// Hedgehog::Database::CDatabaseLoader::LoadArchives
PPC_FUNC_IMPL(__imp__sub_82E0CC38);
PPC_FUNC(sub_82E0CC38)
{
    if (g_mods.empty())
    {
        __imp__sub_82E0CC38(ctx, base);
        return;
    }

    auto r3 = ctx.r3;
    auto r4 = ctx.r4;
    auto r5 = ctx.r5;
    auto r6 = ctx.r6;
    auto r7 = ctx.r7;
    auto r8 = ctx.r8;

    const char* arFilePath = reinterpret_cast<const char*>(base + PPC_LOAD_U32(r5.u32));

    // __HH_ALLOC
    ctx.r3.u32 = 22 + strlen(arFilePath);
    sub_822C0988(ctx, base);
    char* prefixedArFilePath = reinterpret_cast<char*>(base + ctx.r3.u32);

    *reinterpret_cast<be<uint32_t>*>(prefixedArFilePath) = 1;
    strcpy(prefixedArFilePath + 0x4, "/UnleashedRecomp/");
    strcpy(prefixedArFilePath + 0x15, arFilePath);

    ctx.r1.u32 -= 0x10;
    uint32_t stackSpace = ctx.r1.u32;
    PPC_STORE_U32(stackSpace, static_cast<uint32_t>(reinterpret_cast<uint8_t*>(prefixedArFilePath) - base) + 0x4);
    g_prefixedArFilePath = stackSpace;

    ctx.r3 = r3;
    ctx.r4 = r4;
    ctx.r5 = r5;
    ctx.r6 = r6;
    ctx.r7 = r7;
    ctx.r8 = r8;
    __imp__sub_82E0CC38(ctx, base);

    // Hedgehog::Base::CSharedString::~CSharedString
    ctx.r3.u32 = stackSpace;
    sub_82DFB148(ctx, base);

    g_prefixedArFilePath = NULL;
    ctx.r1.u32 += 0x10;
}

// Hedgehog::Database::SLoadElement::SLoadElement
PPC_FUNC_IMPL(__imp__sub_82E140D8);
PPC_FUNC(sub_82E140D8)
{
    // Store the prefixed archive file path as the pretty name. It's unused for archives we want to append to.
    if (!g_mods.empty() && PPC_LOAD_U32(ctx.r5.u32) == 0x8200A621 && g_prefixedArFilePath != NULL)
        ctx.r5.u32 = g_prefixedArFilePath;

    __imp__sub_82E140D8(ctx, base);
}

// Hedgehog::Database::CDatabaseLoader::CCreateFromArchive::CreateCallback
PPC_FUNC_IMPL(__imp__sub_82E0B500);
PPC_FUNC(sub_82E0B500)
{
    if (g_mods.empty())
    {
        __imp__sub_82E0B500(ctx, base);
        return;
    }

    uint32_t prefixedArFilePath = PPC_LOAD_U32(ctx.r5.u32);
    std::u8string_view arFilePathU8(reinterpret_cast<const char8_t*>(base + prefixedArFilePath));
    if (!arFilePathU8.starts_with(u8"/UnleashedRecomp/"))
    {
        const std::string_view assetName(
            reinterpret_cast<const char*>(arFilePathU8.data()), arFilePathU8.size());
        EmitSgPreflightArchiveEntryProbe(assetName);

        const std::filesystem::path looseOverridePath =
            ModLoader::ResolveSgPreflightLooseAssetOverride(assetName);
        if (!looseOverridePath.empty())
        {
            std::ifstream stream(looseOverridePath, std::ios::binary);
            if (stream.good())
            {
                stream.seekg(0, std::ios::end);
                const size_t fileSize = stream.tellg();
                stream.seekg(0, std::ios::beg);

                void* fileData = g_userHeap.Alloc(fileSize);
                stream.read(reinterpret_cast<char*>(fileData), fileSize);
                stream.close();

                auto fileDataHolder = reinterpret_cast<be<uint32_t>*>(g_userHeap.Alloc(sizeof(uint32_t) * 2));
                fileDataHolder[0] = g_memory.MapVirtual(fileData);
                fileDataHolder[1] = NULL;

                auto r6 = ctx.r6;
                auto r7 = ctx.r7;
                ctx.r6.u32 = g_memory.MapVirtual(fileDataHolder);
                ctx.r7.u32 = static_cast<uint32_t>(fileSize);

                UiLab::EmitBridgeScreenEntered(
                    "Asset:OverrideHit:" + std::string(assetName));
                if (ModLoader::s_isLogTypeConsole)
                    LOGF_IMPL(Utility, "SG-Preflight",
                              "loose asset override: {} -> \"{}\"",
                              std::string(assetName),
                              reinterpret_cast<const char*>(looseOverridePath.u8string().c_str()));

                __imp__sub_82E0B500(ctx, base);

                ctx.r6 = r6;
                ctx.r7 = r7;
                g_userHeap.Free(fileDataHolder);
                g_userHeap.Free(fileData);
                return;
            }
        }

        __imp__sub_82E0B500(ctx, base);
        return;
    }

    // Immediately clear the string, so the remaining splits don't load append archives again.
    PPC_STORE_U8(prefixedArFilePath, 0x00);
    arFilePathU8.remove_prefix(0x11);

    auto r3 = ctx.r3; // Callback
    auto r4 = ctx.r4; // Database
    auto r5 = ctx.r5; // Name
    auto r6 = ctx.r6; // Data
    auto r7 = ctx.r7; // Size
    auto r8 = ctx.r8; // Callback data

    auto loadArchive = [&](const std::filesystem::path& arFilePath)
        {
            std::ifstream stream(arFilePath, std::ios::binary);
            if (stream.good())
            {
                if (ModLoader::s_isLogTypeConsole)
                    LOGF_IMPL(Utility, "Mod Loader", "Loading file: \"{}\"", reinterpret_cast<const char*>(arFilePath.u8string().c_str()));

                stream.seekg(0, std::ios::end);
                size_t arFileSize = stream.tellg();

                void* arFileData = g_userHeap.Alloc(arFileSize);
                stream.seekg(0, std::ios::beg);
                stream.read(reinterpret_cast<char*>(arFileData), arFileSize);
                stream.close();

                auto arFileDataHolder = reinterpret_cast<be<uint32_t>*>(g_userHeap.Alloc(sizeof(uint32_t) * 2));

                if (*reinterpret_cast<be<uint32_t>*>(arFileData) == LZX_SIGNATURE)
                {
                    auto fileData = decompressLzx(ctx, base, reinterpret_cast<uint8_t*>(arFileData), arFileSize, arFileDataHolder);

                    g_userHeap.Free(arFileData);

                    arFileData = fileData.data();
                    arFileSize = fileData.size();
                }

                arFileDataHolder[0] = g_memory.MapVirtual(arFileData);
                arFileDataHolder[1] = NULL;

                ctx.r3 = r3;
                ctx.r4 = r4;
                ctx.r5 = r5;
                ctx.r6.u32 = g_memory.MapVirtual(arFileDataHolder);
                ctx.r7.u32 = uint32_t(arFileSize);
                ctx.r8 = r8;

                __imp__sub_82E0B500(ctx, base);

                g_userHeap.Free(arFileDataHolder);
                g_userHeap.Free(arFileData);

                return true;
            }

            return false;
        };

    thread_local xxHashMap<std::vector<std::filesystem::path>> s_cache;

    XXH64_hash_t hash = XXH3_64bits(arFilePathU8.data(), arFilePathU8.size());
    auto findResult = s_cache.find(hash);
    if (findResult != s_cache.end())
    {
        for (const auto& arFilePath : findResult->second)
            loadArchive(arFilePath);
    }
    else
    {
        std::vector<std::filesystem::path> arFilePaths;
        std::filesystem::path arFilePath;
        std::filesystem::path appendArFilePath;

        for (auto& mod : g_mods)
        {
            for (auto& includeDir : mod.includeDirs)
            {
                auto loadUncachedArchive = [&](const std::filesystem::path& arFilePath)
                    {
                        if (mod.type == ModType::UMM && mod.readOnly.contains(arFilePath))
                            return false;

                        std::filesystem::path combinedFilePath = includeDir / arFilePath;
                        bool success = loadArchive(combinedFilePath);
                        if (success)
                            arFilePaths.emplace_back(std::move(combinedFilePath));

                        return success;
                    };

                auto loadArchives = [&](const std::filesystem::path& arFilePath)
                    {
                        thread_local std::filesystem::path s_tempPath;
                        s_tempPath = arFilePath;
                        s_tempPath += "l";

                        if (mod.type == ModType::UMM && mod.readOnly.contains(s_tempPath))
                            return;

                        std::ifstream stream(includeDir / s_tempPath, std::ios::binary);
                        if (stream.good())
                        {
                            be<uint32_t> signature{};
                            uint32_t splitCount{};
                            stream.read(reinterpret_cast<char*>(&signature), sizeof(signature));

                            if (signature == LZX_SIGNATURE)
                            {
                                stream.seekg(0, std::ios::end);
                                size_t arlFileSize = stream.tellg();
                                stream.seekg(0, std::ios::beg);

                                void* compressedFileData = g_userHeap.Alloc(arlFileSize);
                                stream.read(reinterpret_cast<char*>(compressedFileData), arlFileSize);
                                stream.close();

                                auto fileData = decompressLzx(ctx, base, reinterpret_cast<uint8_t*>(compressedFileData), arlFileSize, nullptr);

                                g_userHeap.Free(compressedFileData);

                                splitCount = *reinterpret_cast<uint32_t*>(fileData.data() + 0x4);

                                g_userHeap.Free(fileData.data());
                            }
                            else
                            {
                                stream.read(reinterpret_cast<char*>(&splitCount), sizeof(splitCount));
                                stream.close();
                            }

                            if (splitCount == 0)
                            {
                                loadUncachedArchive(arFilePath);
                            }
                            else
                            {
                                for (uint32_t i = 0; i < splitCount; i++)
                                {
                                    s_tempPath = arFilePath;
                                    s_tempPath += fmt::format(".{:02}", i);
                                    loadUncachedArchive(s_tempPath);
                                }
                            }
                        }
                        else if (mod.type == ModType::UMM)
                        {
                            if (!loadUncachedArchive(arFilePath))
                            {
                                for (uint32_t i = 0; ; i++)
                                {
                                    s_tempPath = arFilePath;
                                    s_tempPath += fmt::format(".{:02}", i);
                                    if (!loadUncachedArchive(s_tempPath))
                                        break;
                                }
                            }
                        }
                    };

                if (mod.type == ModType::UMM)
                {
                    if (mod.merge)
                    {
                        if (arFilePath.empty())
                            arFilePath = arFilePathU8;

                        loadArchives(arFilePath);
                    }
                }
                else if (mod.type == ModType::HMM)
                {
                    if (appendArFilePath.empty())
                    {
                        if (arFilePath.empty())
                            arFilePath = arFilePathU8;

                        appendArFilePath = arFilePath.parent_path();
                        appendArFilePath /= "+";
                        appendArFilePath += arFilePath.filename();
                    }

                    loadArchives(appendArFilePath);
                }
            }
        }

        s_cache.emplace(hash, std::move(arFilePaths));
    }

    ctx.r3 = r3;
    ctx.r4 = r4;
    ctx.r5 = r5;
    ctx.r6 = r6;
    ctx.r7 = r7;
    ctx.r8 = r8;

    __imp__sub_82E0B500(ctx, base);
}

// CriAuObjLoc::AttachCueSheet
PPC_FUNC_IMPL(__imp__sub_8314A310);
PPC_FUNC(sub_8314A310)
{
    // allocator: 0x4
    // capacity: 0x24
    // count: 0x28
    // data: 0x2C
    uint32_t capacity = PPC_LOAD_U32(ctx.r3.u32 + 0x24);
    if (capacity == PPC_LOAD_U32(ctx.r3.u32 + 0x28))
    {
        auto r3 = ctx.r3;
        auto r4 = ctx.r4;
        auto r5 = ctx.r5;

        // Allocate
        ctx.r3.u32 = PPC_LOAD_U32(r3.u32 + 0x4);
        ctx.r4.u32 = (capacity * 2) * sizeof(uint32_t);
        ctx.r5.u32 = 0x82195248; // AuObjCueSheet
        ctx.r6.u32 = 0x4;
        sub_83167FD8(ctx, base);

        // Copy
        uint32_t oldData = PPC_LOAD_U32(r3.u32 + 0x2C);
        uint32_t newData = ctx.r3.u32;

        memcpy(base + newData, base + oldData, capacity * sizeof(uint32_t));
        memset(base + newData + (capacity * sizeof(uint32_t)), 0, capacity * sizeof(uint32_t));

        PPC_STORE_U32(r3.u32 + 0x24, capacity * 2);
        PPC_STORE_U32(r3.u32 + 0x2C, newData);

        // Deallocate
        ctx.r3.u32 = PPC_LOAD_U32(r3.u32 + 0x4);
        ctx.r4.u32 = oldData;
        sub_83168100(ctx, base);

        ctx.r3 = r3;
        ctx.r4 = r4;
        ctx.r5 = r5;
    }

    __imp__sub_8314A310(ctx, base);
}
