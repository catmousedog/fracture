set(TARGET util)

file(GLOB_RECURSE SOURCES "*.cpp")

# add_library(${TARGET} STATIC 
#     ${SOURCES}
# )

add_library(${TARGET} INTERFACE) 

target_include_directories(${TARGET} INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)