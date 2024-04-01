cmake_minimum_required(VERSION 3.18)

project(
  SpeakEasy2
  DESCRIPTION "Python bindings for the SpeakEasy 2 C lib."
  LANGUAGES C CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(BUILD_SHARED_LIBS OFF)

if ("${CMAKE_PACKAGE_VERSION}" STREQUAL "")
  if (NOT GIT_FOUND)
    find_package(Git QUIET)
  endif()

  execute_process(COMMAND ${GIT_EXECUTABLE} describe --tags
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    OUTPUT_VARIABLE CMAKE_PACKAGE_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
endif()

if ("${CMAKE_PACKAGE_VERSION}" STREQUAL "")
  set(CMAKE_PACKAGE_VERSION "NOTFOUND")
endif()

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/vendor/speakeasy2/include/se2_version.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/include/se2_version.h
)

message(STATUS "${CMAKE_PROJECT_NAME} version: ${CMAKE_PACKAGE_VERSION}")

# NOTE: speakeasy2 builds the igraph C lib.
add_subdirectory(vendor/speakeasy2)

find_package(Python3 COMPONENTS Interpreter Development)

if(NOT Python3_FOUND)
  message(WARNING "Python not found not building package.")
endif()

if(Python3_VERSION_MINOR LESS 10)
  message(WARNING
    "Python version must be at least 3.10 found \
${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR} not building package.")
endif()

Python3_add_library(_speakeasy2 MODULE
  ${CMAKE_CURRENT_SOURCE_DIR}/speakeasy2/_speakeasy2.c)

target_link_libraries(_speakeasy2
  PUBLIC SpeakEasy2
)

target_include_directories(_speakeasy2
  PRIVATE
  ${PROJECT_SOURCE_DIR}/vendor/python-igraph/src/_igraph
  ${PROJECT_BINARY_DIR}/include
)
