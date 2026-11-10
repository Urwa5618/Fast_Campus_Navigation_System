cmake_minimum_required(VERSION 3.16)
project(FastNav LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)

# ---- Core library (plain C++, no Qt) ----
add_library(core
    src/core/Graph.cpp
    src/core/DataLoader.cpp
    src/core/Algorithms.cpp
)
target_include_directories(core PUBLIC src/core)

# ---- Console test for the core ----
add_executable(test_core src/test_core.cpp)
target_link_libraries(test_core PRIVATE core)

# ---- GUI application ----
find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(FastNav
    src/main.cpp
    src/gui/MainWindow.cpp
    src/gui/MapView.cpp
)
target_include_directories(FastNav PRIVATE src/gui)
target_link_libraries(FastNav PRIVATE core Qt6::Widgets)
