set(TARGET vulkan)

find_package(Vulkan REQUIRED)

file(GLOB_RECURSE SOURCES "*.cpp")

add_library(${TARGET} 
	${SOURCES}
)

target_link_libraries(${TARGET} PRIVATE
    Vulkan::Vulkan
)

target_include_directories(${TARGET} PRIVATE
    ${Vulkan_INCLUDE_DIRS}
)