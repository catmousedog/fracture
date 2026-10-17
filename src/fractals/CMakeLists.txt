set(TARGET fractals)

file(GLOB SOURCES "*.cpp")
add_library(${TARGET} INTERFACE
    ${SOURCES}
)

# target_link_libraries(${TARGET} PRIVATE
# 	util
# )

# LINK_SRC(${TARGET})