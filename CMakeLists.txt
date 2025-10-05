cmake_minimum_required(VERSION 3.18)

project(${SKBUILD_PROJECT_NAME} LANGUAGES C)

find_package(
  Python
  COMPONENTS Development.Module NumPy
  REQUIRED)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(vendor/speakeasy2)

python_add_library(_speakeasy2 MODULE speakeasy2/_speakeasy2.c WITH_SOABI)
target_link_libraries(_speakeasy2 PUBLIC SpeakEasy2)
target_include_directories(_speakeasy2 PRIVATE vendor/python-igraph/src/_igraph
  ${Python_NumPy_INCLUDE_DIR})

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set_property(TARGET ${name} PROPERTY SUFFIX ".${SKBUILD_SOABI}.pyd")
else()
  set_property(TARGET ${name}
    PROPERTY SUFFIX ".${SKBUILD_SOABI}${CMAKE_SHARED_MODULE_SUFFIX}")
endif()

install(TARGETS _speakeasy2 DESTINATION speakeasy2)
