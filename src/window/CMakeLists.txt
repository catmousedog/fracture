set(TARGET window)

file(GLOB SOURCES "*.cpp")
add_library(${TARGET} 
	${SOURCES}
)

target_link_libraries(${TARGET} PRIVATE
	glfw
	imgui
)

LINK_SRC(${TARGET})