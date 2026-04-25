cmake_minimum_required(VERSION 3.20)
project(sward_ui_runtime_reference LANGUAGES C CXX)

add_library(sward_ui_runtime STATIC
    src/contract_loader.cpp
    src/runtime.cpp
    src/profiles.cpp
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
