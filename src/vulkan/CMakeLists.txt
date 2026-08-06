set(TARGET vulkan)

file(GLOB_RECURSE SOURCES "*.cpp")
add_library(${TARGET} 
	${SOURCES}
)

target_link_libraries(${TARGET} PRIVATE
	Vulkan::Vulkan
	imgui
	window
	glm
)
target_include_directories(${TARGET} PRIVATE
    ${Vulkan_INCLUDE_DIRS}
)

target_compile_definitions(${TARGET} PRIVATE SHADER_DIR="${CMAKE_BINARY_DIR}/shaders/")

LINK_SRC(${TARGET})