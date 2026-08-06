set(TARGET window)

file(GLOB_RECURSE SOURCES "*.cpp")
add_library(${TARGET} 
	${SOURCES}
)

target_link_libraries(${TARGET} PRIVATE
	glfw
	imgui
)

LINK_SRC(${TARGET})