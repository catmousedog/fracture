set(TARGET fractals)

file(GLOB_RECURSE SOURCES "*.cpp")
add_library(${TARGET} STATIC 
    ${SOURCES}
)

target_link_libraries(${TARGET} PRIVATE
	util
)

target_compile_definitions(${TARGET} PRIVATE SHADER_DIR="${CMAKE_BINARY_DIR}/shaders/")

LINK_SRC(${TARGET})