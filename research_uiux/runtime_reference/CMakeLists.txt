cmake_minimum_required(VERSION 3.20)
project(sward_ui_runtime_reference LANGUAGES C CXX)

add_library(sward_ui_runtime STATIC
    src/contract_loader.cpp
    src/runtime.cpp
    src/profiles.cpp
    src/sgfx_templates.cpp
    src/sonic_hud_reference.cpp
    src/frontend_screen_reference.cpp
    src/frontend_screen_controllers.cpp
)

target_include_directories(sward_ui_runtime
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(sward_ui_runtime PUBLIC cxx_std_20)

target_compile_definitions(sward_ui_runtime
    PUBLIC
        SWARD_UI_RUNTIME_CONTRACT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/contracts"
)

add_library(sward_ui_runtime_c_api STATIC
    src/runtime_c.cpp
)

target_include_directories(sward_ui_runtime_c_api
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(sward_ui_runtime_c_api
    PUBLIC
        sward_ui_runtime
)

target_compile_features(sward_ui_runtime_c_api PUBLIC cxx_std_20)

add_executable(sward_ui_runtime_example
    examples/pause_menu_example.cpp
)

target_link_libraries(sward_ui_runtime_example
    PRIVATE
        sward_ui_runtime
)

add_executable(sward_ui_runtime_title_menu_example
    examples/title_menu_example.cpp
)

target_link_libraries(sward_ui_runtime_title_menu_example
    PRIVATE
        sward_ui_runtime
)

add_executable(sward_ui_runtime_toast_example
    examples/toast_overlay_example.cpp
)

target_link_libraries(sward_ui_runtime_toast_example
    PRIVATE
        sward_ui_runtime
)

add_executable(sward_ui_runtime_c_example
    examples/c_pause_menu_example.c
)

target_link_libraries(sward_ui_runtime_c_example
    PRIVATE
        sward_ui_runtime_c_api
)

set_target_properties(sward_ui_runtime_c_example PROPERTIES LINKER_LANGUAGE CXX)

add_executable(sward_ui_runtime_debug_selector
    examples/ui_debug_selector.cpp
)

target_link_libraries(sward_ui_runtime_debug_selector
    PRIVATE
        sward_ui_runtime
)

add_executable(sward_ui_runtime_debug_workbench
    examples/ui_debug_workbench.cpp
)

target_link_libraries(sward_ui_runtime_debug_workbench
    PRIVATE
        sward_ui_runtime
)

add_executable(sward_sgfx_template_catalog
    examples/sgfx_template_catalog.cpp
)

target_link_libraries(sward_sgfx_template_catalog
    PRIVATE
        sward_ui_runtime
)

add_executable(sward_sonic_hud_reference_catalog
    examples/sonic_hud_reference_catalog.cpp
)

target_link_libraries(sward_sonic_hud_reference_catalog
    PRIVATE
        sward_ui_runtime
)

add_executable(sward_frontend_screen_reference_catalog
    examples/frontend_screen_reference_catalog.cpp
)

target_link_libraries(sward_frontend_screen_reference_catalog
    PRIVATE
        sward_ui_runtime
)

add_executable(sward_frontend_screen_controller_catalog
    examples/frontend_screen_controller_catalog.cpp
)

target_link_libraries(sward_frontend_screen_controller_catalog
    PRIVATE
        sward_ui_runtime
)

add_executable(sward_ui_runtime_debug_gui WIN32
    examples/ui_debug_workbench_gui.cpp
)

target_link_libraries(sward_ui_runtime_debug_gui
    PRIVATE
        sward_ui_runtime
        user32
        gdi32
        gdiplus
)

add_executable(sward_su_ui_asset_renderer WIN32
    examples/su_ui_asset_renderer.cpp
)

target_link_libraries(sward_su_ui_asset_renderer
    PRIVATE
        sward_ui_runtime
        user32
        gdi32
        gdiplus
)

# Phase 334: standalone sgfx_runtime.exe -- drives every screen
# state machine through a full retail playthrough scenario without
# UnleashedRecomp in the loop. Compiles audio dispatch + the
# embedded OGG cue blobs from UnleashedRecomp/res/sounds.
set(_SGFX_RUNTIME_RES_SOUNDS
    "${CMAKE_SOURCE_DIR}/local_build_env/ur103clean/UnleashedRecomp")
add_executable(sgfx_runtime
    src/sgfx_runtime_main.cpp
    src/sgfx_audio_dispatch.cpp
    "${_SGFX_RUNTIME_RES_SOUNDS}/res/sounds/sys_worldmap_cursor.ogg.c"
    "${_SGFX_RUNTIME_RES_SOUNDS}/res/sounds/sys_worldmap_finaldecide.ogg.c"
    "${_SGFX_RUNTIME_RES_SOUNDS}/res/sounds/sys_actstg_pausecansel.ogg.c"
    "${_SGFX_RUNTIME_RES_SOUNDS}/res/sounds/sys_actstg_pausecursor.ogg.c"
    "${_SGFX_RUNTIME_RES_SOUNDS}/res/sounds/sys_actstg_pausedecide.ogg.c"
    "${_SGFX_RUNTIME_RES_SOUNDS}/res/sounds/sys_actstg_pausewinclose.ogg.c"
    "${_SGFX_RUNTIME_RES_SOUNDS}/res/sounds/sys_actstg_pausewinopen.ogg.c"
)

target_include_directories(sgfx_runtime
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        "${_SGFX_RUNTIME_RES_SOUNDS}"
)

target_compile_features(sgfx_runtime PRIVATE cxx_std_20)

# Phase 334: state-machine smoke test target. Header-only state
# machines + free-standing test driver -- verifies retail-mined
# field offsets + behaviors are preserved across edits.
add_executable(sgfx_state_machines_smoke
    src/sgfx_state_machines_smoke_test.cpp
)

target_include_directories(sgfx_state_machines_smoke
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(sgfx_state_machines_smoke PRIVATE cxx_std_20)
