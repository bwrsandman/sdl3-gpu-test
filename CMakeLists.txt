cmake_minimum_required(VERSION 3.27)
project(SDL3GPUTest)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL3 REQUIRED)

find_program(slangc slangc REQUIRED)
find_program(xxd xxd REQUIRED)

function(add_shader)
  cmake_parse_arguments(ARG "" "SHADER_NAME;ENTRY_POINT;INPUT_FILE;OUTPUT_FILE;TARGET;PROFILE" "" ${ARGN})

  get_filename_component(output_base ${ARG_OUTPUT_FILE} NAME_WE)

  set(binary_output ${CMAKE_CURRENT_BINARY_DIR}/${output_base}.${ARG_TARGET})

  add_custom_command(
    OUTPUT ${binary_output}
    COMMAND ${slangc} ${ARG_INPUT_FILE} -entry ${ARG_ENTRY_POINT} -target ${ARG_TARGET} -profile ${ARG_PROFILE} -o ${binary_output}
    DEPENDS ${ARG_INPUT_FILE}
    VERBATIM)

  add_custom_command(
    OUTPUT ${ARG_OUTPUT_FILE}
    COMMAND ${xxd} -i -n ${ARG_SHADER_NAME} ${binary_output} ${ARG_OUTPUT_FILE}
    DEPENDS ${binary_output}
    VERBATIM)
endfunction()

if (WIN32)
  add_shader(
    SHADER_NAME triangle_vert_dxil
    ENTRY_POINT vertexMain
    INPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/triangle.slang
    OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/triangle_vert_dxil.h
    TARGET dxil
    PROFILE vs_6_6
  )
  list(APPEND SHADER_HEADERS ${CMAKE_CURRENT_BINARY_DIR}/triangle_vert_dxil.h)

  add_shader(
    SHADER_NAME triangle_frag_dxil
    ENTRY_POINT fragmentMain
    INPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/triangle.slang
    OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/triangle_frag_dxil.h
    TARGET dxil
    PROFILE ps_6_6
  )
  list(APPEND SHADER_HEADERS ${CMAKE_CURRENT_BINARY_DIR}/triangle_frag_dxil.h)

  add_shader(
    SHADER_NAME triangle_vert_dxbc
    ENTRY_POINT vertexMain
    INPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/triangle.slang
    OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/triangle_vert_dxbc.h
    TARGET dxbc
    PROFILE vs_6_6
  )
  list(APPEND SHADER_HEADERS ${CMAKE_CURRENT_BINARY_DIR}/triangle_vert_dxbc.h)

  add_shader(
    SHADER_NAME triangle_frag_dxbc
    ENTRY_POINT fragmentMain
    INPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/triangle.slang
    OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/triangle_frag_dxbc.h
    TARGET dxbc
    PROFILE ps_6_6
  )
  list(APPEND SHADER_HEADERS ${CMAKE_CURRENT_BINARY_DIR}/triangle_frag_dxil.h)
endif()

if(Darwin)
  add_shader(
    SHADER_NAME triangle_vert_metallib
    ENTRY_POINT vertexMain
    INPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/triangle.slang
    OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/triangle_vert_metallib.h
    TARGET metallib
    PROFILE vs_6_6
  )
  list(APPEND SHADER_HEADERS ${CMAKE_CURRENT_BINARY_DIR}/triangle_vert_metallib.h)

  add_shader(
    SHADER_NAME triangle_frag_metallib
    ENTRY_POINT fragmentMain
    INPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/triangle.slang
    OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/triangle_frag_metallib.h
    TARGET metallib
    PROFILE ps_6_6
  )
  list(APPEND SHADER_HEADERS ${CMAKE_CURRENT_BINARY_DIR}/triangle_frag_metallib.h)
else()
  add_shader(
    SHADER_NAME triangle_vert_spirv
    ENTRY_POINT vertexMain
    INPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/triangle.slang
    OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/triangle_vert_spirv.h
    TARGET spirv
    PROFILE vs_6_6
  )
  list(APPEND SHADER_HEADERS ${CMAKE_CURRENT_BINARY_DIR}/triangle_vert_spirv.h)

  add_shader(
    SHADER_NAME triangle_frag_spirv
    ENTRY_POINT fragmentMain
    INPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/triangle.slang
    OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/triangle_frag_spirv.h
    TARGET spirv
    PROFILE ps_6_6
  )
  list(APPEND SHADER_HEADERS ${CMAKE_CURRENT_BINARY_DIR}/triangle_frag_spirv.h)
endif()

add_subdirectory(lnd)

add_executable(sdl3gputest main.cpp ${CMAKE_CURRENT_BINARY_DIR}/triangle_vert_spirv.h ${SHADER_HEADERS})
target_link_libraries(sdl3gputest PRIVATE SDL3::SDL3 lnd)
target_include_directories(sdl3gputest PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
