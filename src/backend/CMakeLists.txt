set(TARGET backend)

file(GLOB SOURCES "*.cpp")
add_library(${TARGET} STATIC 
    ${SOURCES}
)

add_dependencies(${TARGET} shaders)

target_link_libraries(${TARGET} PRIVATE
	vulkan
	fractals
	util
)

LINK_SRC(${TARGET})