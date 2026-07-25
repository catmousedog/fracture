set(TARGET core)

file(GLOB SOURCES "*.cpp")
add_library(${TARGET} STATIC 
    ${SOURCES}
)

add_dependencies(${TARGET} shaders)

add_subdirectory(fractals)

target_link_libraries(${TARGET} PRIVATE
	fractals	
	render
	ui
	window
)

LINK_SRC(${TARGET})