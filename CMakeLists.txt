cmake_minimum_required(VERSION 3.20)
project(fracture)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(imgui)

add_subdirectory(src)