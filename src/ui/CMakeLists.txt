set(TARGET ui)

file(GLOB_RECURSE SOURCES "*.cpp")
add_library(${TARGET} 
	${SOURCES}
	${IMGUI_SOURCES}
)

target_link_libraries(${TARGET} PRIVATE
	window
)

target_include_directories(${TARGET} PRIVATE
	${IMGUI_INCLUDE}
)

LINK_SRC(${TARGET})