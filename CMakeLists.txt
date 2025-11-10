cmake_minimum_required(VERSION 3.18)

project(${SKBUILD_PROJECT_NAME} LANGUAGES C)

find_package(
  Python
  COMPONENTS Development.Module NumPy ${SKBUILD_SABI_COMPONENT}
  REQUIRED)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(IGRAPH_ENABLE_LTO AUTO)
set(IGRAPH_USE_INTERNAL_GMP ON)
set(IGRAPH_USE_INTERNAL_ARPACK ON)
set(IGRAPH_USE_INTERNAL_BLAS ON)
set(IGRAPH_USE_INTERNAL_GLPK ON)
set(IGRAPH_USE_INTERNAL_LAPACK ON)
set(IGRAPH_USE_INTERNAL_PLFIT ON)

add_subdirectory(vendor/speakeasy2)

if(NOT "${SKBUILD_SABI_VERSION}" STREQUAL "")
  python_add_library(_speakeasy2 MODULE WITH_SOABI USE_SABI
    ${SKBUILD_SABI_VERSION} speakeasy2/_speakeasy2.c)
else()
  python_add_library(_speakeasy2 MODULE WITH_SOABI speakeasy2/_speakeasy2.c)
endif()

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
