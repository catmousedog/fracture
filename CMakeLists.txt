cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED CMAKE_CXX_COMPILER AND NOT DEFINED ENV{CXX})
    find_program(CLANGXX clang++)
    if(CLANGXX)
        set(CMAKE_CXX_COMPILER "${CLANGXX}" CACHE FILEPATH "")
        set(CMAKE_C_COMPILER clang CACHE FILEPATH "")
    endif()
endif()

project(fracture)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(Vulkan REQUIRED)
find_package(slang REQUIRED)

add_subdirectory(imgui)

add_subdirectory(shaders)

add_subdirectory(src)