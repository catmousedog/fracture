set(TARGET render)

file(GLOB SOURCES "*.cpp")
add_library(${TARGET} 
	${SOURCES}
)

target_link_libraries(${TARGET} PRIVATE
	vulkan
	imgui
	window
	backend
	fractals
)

LINK_SRC(${TARGET})