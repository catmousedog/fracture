set(TARGET src)

# allow includes relative to src/
add_library(${TARGET} INTERFACE)
target_include_directories(${TARGET} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

function(LINK_SRC TARGET)
    target_link_libraries(${TARGET} PRIVATE src)
endfunction()

add_subdirectory(window)
add_subdirectory(vulkan)
add_subdirectory(render)
add_subdirectory(ui)
add_subdirectory(core)
add_subdirectory(main)
