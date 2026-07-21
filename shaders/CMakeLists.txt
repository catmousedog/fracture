find_program(SLANGC_EXECUTABLE slangc REQUIRED)

file(GLOB SHADER_SOURCES *.slang)

set(SHADER_SOURCES
    shader.slang
)

foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
    set(OUT ${CMAKE_BINARY_DIR}/shaders/${SHADER_NAME}.spv)

    add_custom_command(
        OUTPUT ${OUT}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/shaders
        COMMAND ${SLANGC_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER}
                -target spirv -profile spirv_1_4 -emit-spirv-directly
                -fvk-use-entrypoint-name
                -entry vertMain -entry fragMain # hard coded for now
                -o ${OUT}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER}
        COMMENT "Compiling ${SHADER}"
        VERBATIM
    )
    list(APPEND SHADER_OUTPUTS ${OUT})
endforeach()

add_custom_target(shaders ALL DEPENDS ${SHADER_OUTPUTS})