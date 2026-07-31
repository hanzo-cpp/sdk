# The Hanzo C++ SDK — one static library over the whole /v1 surface.
#
# The client under include/ and src/ is generated (scripts/generate.sh); this
# file is not, and the generator's own CMakeLists.txt is dropped before the tree
# lands. The target name, the export set and the examples switch are this repo's
# contract with a consumer, so the repo owns them.
cmake_minimum_required(VERSION 3.20)

# Kept in step with hanzo.yaml's info.version by scripts/generate.sh, which
# writes the same number into the generated sources' packageVersion.
project(hanzo VERSION 8.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(HANZO_BUILD_EXAMPLES "Build the six canonical example flows" OFF)

find_package(cpprestsdk REQUIRED)
find_package(Boost REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# GLOB, deliberately: the file list IS the generated tree, and a hand-kept copy
# of ~2400 names would be a second source of truth that goes stale the first time
# an operation is added upstream. CONFIGURE_DEPENDS makes cmake re-check it on
# every build.
file(GLOB_RECURSE HANZO_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
if(NOT HANZO_SOURCES)
  message(FATAL_ERROR
    "no sources under src/. The client is generated: run ./scripts/generate.sh")
endif()

add_library(hanzo STATIC ${HANZO_SOURCES})
add_library(hanzo::hanzo ALIAS hanzo)

target_include_directories(hanzo PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

target_link_libraries(hanzo PUBLIC
  cpprestsdk::cpprest
  Boost::boost
  OpenSSL::SSL
  OpenSSL::Crypto
  Threads::Threads)

# The generated sources are what the document projects, not code anyone reviews
# line by line. Warnings on them are noise that would bury a real warning in the
# examples.
target_compile_options(hanzo PRIVATE -w)

if(HANZO_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS hanzo EXPORT hanzoTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT hanzoTargets
  FILE hanzoTargets.cmake
  NAMESPACE hanzo::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hanzo)

configure_package_config_file(
  cmake/hanzoConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/hanzoConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hanzo)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/hanzoConfigVersion.cmake"
  COMPATIBILITY SameMajorVersion)
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/hanzoConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/hanzoConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hanzo)
