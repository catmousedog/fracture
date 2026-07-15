set(TARGET core)

file(GLOB_RECURSE SOURCES "*.cpp")

add_library(${TARGET} STATIC 
    ${SOURCES}
)

target_link_libraries(${TARGET} 
    vulkan
)
target_include_directories(${TARGET} PRIVATE 
    ../vulkan
)