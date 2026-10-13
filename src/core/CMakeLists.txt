set(TARGET core)

file(GLOB SOURCES "*.cpp")
add_library(${TARGET} STATIC 
    ${SOURCES}
)

add_dependencies(${TARGET} shaders)

target_link_libraries(${TARGET} PRIVATE
	fractals
	backend
	render
	window
)

LINK_SRC(${TARGET})