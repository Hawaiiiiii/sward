cmake_minimum_required(VERSION 3.20)
project(sgfx_ui CXX C)   # C enabled for thirdparty/stb/stb_vorbis.c (OGG decode)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# RECOMP_ROOT points at a recomp source tree. We reuse:
#   - thirdparty: SDL2 (built static), nlohmann/json (header), stb (header),
#                 D3D12MemoryAllocator (compiled), the plume RHI (compiled),
#                 and the precompiled CSD DXIL shaders.
# Pass -DRECOMP_ROOT=C:/swardbuild when configuring.
if(NOT DEFINED RECOMP_ROOT)
    message(FATAL_ERROR "set -DRECOMP_ROOT=<recomp source dir>")
endif()

# vcpkg-installed DirectX-Headers + Agility SDK that the recomp built against.
if(NOT DEFINED SGFX_VCPKG)
    set(SGFX_VCPKG "${RECOMP_ROOT}/thirdparty/vcpkg/installed/x64-windows-static")
endif()
set(VCPKG_INC "${SGFX_VCPKG}/include")
set(VCPKG_LIB "${SGFX_VCPKG}/lib")
set(AGILITY_DLL "${SGFX_VCPKG}/bin/D3D12Core.dll")

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON  CACHE BOOL "" FORCE)
set(SDL_TEST   OFF CACHE BOOL "" FORCE)
add_subdirectory("${RECOMP_ROOT}/thirdparty/SDL" sdl_build EXCLUDE_FROM_ALL)

add_executable(sgfx_ui
    src/main.cpp
    src/gfx_d3d12.cpp
    "${RECOMP_ROOT}/thirdparty/plume/plume_d3d12.cpp"
    "${RECOMP_ROOT}/thirdparty/plume/contrib/D3D12MemoryAllocator/src/D3D12MemAlloc.cpp")

target_compile_definitions(sgfx_ui PRIVATE
    NOMINMAX WIN32_LEAN_AND_MEAN
    UNLEASHED_RECOMP_D3D12
    D3D12MA_USING_DIRECTX_HEADERS
    D3D12MA_OPTIONS16_SUPPORTED)

target_include_directories(sgfx_ui PRIVATE
    "${VCPKG_INC}"                                    # directx/d3d12.h, dxguids/, agility
    "${RECOMP_ROOT}/thirdparty/plume"                 # plume_*.h
    "${RECOMP_ROOT}/UnleashedRecomp/gpu/shader"       # csd_vs.hlsl.dxil.h, csd_filter_ps...
    "${RECOMP_ROOT}/thirdparty/plume/contrib/D3D12MemoryAllocator/include"
    "${RECOMP_ROOT}/thirdparty/SDL/include"
    "${RECOMP_ROOT}/thirdparty/json/include"
    "${RECOMP_ROOT}/thirdparty/stb")

target_link_libraries(sgfx_ui PRIVATE
    SDL2-static
    "${VCPKG_LIB}/DirectX-Guids.lib"
    d3d12 dxgi dxguid)

# Stage data + assets + the Agility runtime (loaded from .\D3D12\ per plume's exports).
add_custom_command(TARGET sgfx_ui POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/data" "$<TARGET_FILE_DIR:sgfx_ui>/data"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${SGFX_ASSETS}" "$<TARGET_FILE_DIR:sgfx_ui>/assets"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:sgfx_ui>/D3D12"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${AGILITY_DLL}" "$<TARGET_FILE_DIR:sgfx_ui>/D3D12/D3D12Core.dll")

# -----------------------------------------------------------------------------
# sgfx_screens â€” the clean, hand-written screen reconstruction (UnleashedRecomp
# ui/options_menu methodology): each screen is a C++ module drawn through the
# sgfxui helper layer onto the same CSD GPU path. Shares the plume/SDL/stb setup.
# -----------------------------------------------------------------------------
add_executable(sgfx_screens
    src/main_screens.cpp
    src/sgfxui.cpp
    src/globe3d.cpp
    src/gltf_loader.cpp
    src/settings.cpp
    src/audio.cpp
    src/csd_player.cpp
    src/sgfx_data.cpp
    "${RECOMP_ROOT}/thirdparty/stb/stb_vorbis.c"
    src/screen_registry.cpp
    src/green_chrome.cpp
    src/viewer3d.cpp
    src/al_coverage.cpp
    src/fleet.cpp
    src/changelog.cpp
    src/variants.cpp
    src/screen_carview.cpp
    src/screen_ambient.cpp
    src/screen_fleet.cpp
    src/screen_qatests.cpp
    src/screen_changelogs.cpp
    src/screen_testdetail.cpp
    src/screen_delivery.cpp
    src/screen_variants.cpp
    src/screen_pause.cpp
    src/screen_result.cpp
    src/screen_title.cpp
    src/screen_world_map.cpp
    src/screen_status.cpp
    src/screen_sonic_hud.cpp
    src/screen_options.cpp
    src/screen_town.cpp
    src/screen_gate.cpp
    src/screen_result_ex.cpp
    src/screen_mediaroom.cpp
    src/screen_loading.cpp
    src/screen_start.cpp
    src/screen_balloon.cpp
    src/screen_world_map_help.cpp
    src/screen_installer.cpp
    src/screen_boot_loading.cpp
    src/screen_boot_title.cpp
    src/screen_boot_logos.cpp
    src/gfx_d3d12.cpp
    "${RECOMP_ROOT}/thirdparty/plume/plume_d3d12.cpp"
    "${RECOMP_ROOT}/thirdparty/plume/contrib/D3D12MemoryAllocator/src/D3D12MemAlloc.cpp")

target_compile_definitions(sgfx_screens PRIVATE
    NOMINMAX WIN32_LEAN_AND_MEAN
    UNLEASHED_RECOMP_D3D12
    D3D12MA_USING_DIRECTX_HEADERS
    D3D12MA_OPTIONS16_SUPPORTED)

target_include_directories(sgfx_screens PRIVATE
    "${VCPKG_INC}"
    "${RECOMP_ROOT}/thirdparty/plume"
    "${RECOMP_ROOT}/UnleashedRecomp/gpu/shader"
    "${RECOMP_ROOT}/thirdparty/plume/contrib/D3D12MemoryAllocator/include"
    "${RECOMP_ROOT}/thirdparty/SDL/include"
    "${RECOMP_ROOT}/thirdparty/json/include"
    "${RECOMP_ROOT}/thirdparty/stb")

target_link_libraries(sgfx_screens PRIVATE
    SDL2-static
    "${VCPKG_LIB}/DirectX-Guids.lib"
    d3d12 dxgi dxguid)

# Stage the Agility runtime AND the full data/ + assets/ trees: the --csd CSD
# renderer reads data/<id>.json and assets/<id>/<tex>.png relative to the exe's
# cwd, so the whole set must be present in the build dir (a missing assets/<id>
# silently renders that screen blank).
add_custom_command(TARGET sgfx_screens POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:sgfx_screens>/D3D12"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${AGILITY_DLL}" "$<TARGET_FILE_DIR:sgfx_screens>/D3D12/D3D12Core.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/data" "$<TARGET_FILE_DIR:sgfx_screens>/data"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${SGFX_ASSETS}" "$<TARGET_FILE_DIR:sgfx_screens>/assets")
