set(TARGET render)

file(GLOB_RECURSE SOURCES "*.cpp")
add_library(${TARGET} 
	${SOURCES}
)

target_link_libraries(${TARGET} PRIVATE
	vulkan
	imgui
	window
)

LINK_SRC(${TARGET})