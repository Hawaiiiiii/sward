cmake_minimum_required(VERSION 3.20)
project(sward_ui_runtime_reference LANGUAGES CXX)

add_library(sward_ui_runtime STATIC
    src/runtime.cpp
)

target_include_directories(sward_ui_runtime
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(sward_ui_runtime PUBLIC cxx_std_20)

add_executable(sward_ui_runtime_example
    examples/pause_menu_example.cpp
)

target_link_libraries(sward_ui_runtime_example
    PRIVATE
        sward_ui_runtime
)
