set(TARGET ui)

find_package(glfw3 REQUIRED)

file(GLOB_RECURSE SOURCES "*.cpp")

add_library(${TARGET} 
	${SOURCES} 
	${IMGUI_SOURCES}
)

target_link_libraries(${TARGET} PRIVATE
    util
    vulkan
    glfw
)

target_include_directories(${TARGET} PRIVATE
    ${imgui_SOURCE_DIR}
    ../util
    ../vulkan
)