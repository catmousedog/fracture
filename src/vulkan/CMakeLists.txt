set(TARGET vulkan)

find_package(Vulkan REQUIRED)

file(GLOB_RECURSE SOURCES "*.cpp")

add_library(${TARGET} 
	${SOURCES}
	${IMGUI_SOURCES}
)

# PRIVATE
target_link_libraries(${TARGET} PRIVATE
    Vulkan::Vulkan
	glfw
	glm
)
target_include_directories(${TARGET} PRIVATE
    ${Vulkan_INCLUDE_DIRS}
	${imgui_SOURCE_DIR}
)

# PUBLIC
target_link_libraries(${TARGET} PUBLIC
    util
)
target_include_directories(${TARGET} PUBLIC
    ../util
)


add_dependencies(${TARGET} shaders)

target_compile_definitions(${TARGET} PRIVATE SHADER_DIR="${CMAKE_BINARY_DIR}/shaders/")