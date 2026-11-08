set(TARGET fractals)

file(GLOB SOURCES "*.cpp")
add_library(${TARGET} INTERFACE
    ${SOURCES}
)

# LINK_SRC(${TARGET})