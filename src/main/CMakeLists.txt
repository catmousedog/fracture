set(TARGET ${PROJECT_NAME})

add_executable(${TARGET} 
	main.cpp
)

target_link_libraries(${TARGET} 
	vulkan
	core
)
target_include_directories(${TARGET} PRIVATE 
	../vulkan
	../core
)