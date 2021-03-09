# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.18)

project ("usamba")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  add_compile_options(-fprofile-instr-generate -fcoverage-mapping)
  add_link_options(-fprofile-instr-generate)

  #add_compile_options(-MD)
  #add_link_options(clang_rt.asan_dynamic-i386.lib)
  #add_compile_options(-fsanitize=address)
endif()

set(SOURCES
    "chipid.c" "chipid.h"
    "comm.c" "comm.h"
    "eefc.c" "eefc.h"
    "utils.h"
    "usamba.c" 
)

add_executable(${PROJECT_NAME} ${SOURCES})

set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD 99)
set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD_REQUIRED 99)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED 11)

#target_include_directories(${PROJECT_NAME} PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/network")

# get_cmake_property(_variableNames VARIABLES)
# foreach (_variableName ${_variableNames})
#   message(STATUS "${_variableName}=${${_variableName}}")
# endforeach()
#