set(TARGET ${PROJECT_NAME})

file(GLOB_RECURSE SOURCES "*.cpp")
add_executable(${TARGET}
	${SOURCES}
)

target_link_libraries(${TARGET} PRIVATE
	backend	
	core
)

LINK_SRC(${TARGET})