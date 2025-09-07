cmake_minimum_required(VERSION 3.22)



# Project setup

if(APPLE AND NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES arm64;x86_64)
endif()

project(phosg)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
if (MSVC)
    # Disabled warnings:
    # 4458 = declaration of '%s' hides class member (I use this->x for members)
    add_compile_options(/W4 /WX /wd4458)
else()
    add_compile_options(-Wall -Wextra -Werror -Wno-strict-aliasing -Wno-unused-result -Wno-overflow)
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CTEST_OUTPUT_ON_FAILURE ON)



# Library and executable definitions

add_library(
  phosg
  src/Arguments.cc
  src/Encoding.cc
  src/Filesystem.cc
  src/Hash.cc
  src/JSON.cc
  src/Network.cc
  src/Process.cc
  src/Random.cc
  src/Strings.cc
  src/Time.cc
  src/Tools.cc
  src/UnitTest.cc
)
if (NOT WIN32)
  target_sources(phosg PRIVATE src/Filesystem-Unix.cc)
endif()
target_link_libraries(phosg pthread z)
target_include_directories(
  phosg PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
if("${PHOSG_SKIP_PROCESS_TEST}" EQUAL 1)
  target_compile_definitions(phosg PUBLIC -DPHOSG_SKIP_PROCESS_TEST)
endif()

# It seems that on some Linux variants (e.g. Raspbian) we also need -latomic,
# but this library does not exist on others (e.g. Ubuntu) nor on macOS
message(STATUS "Target architecture is ${CMAKE_HOST_SYSTEM_PROCESSOR}")
string(FIND ${CMAKE_HOST_SYSTEM_PROCESSOR} "armv" IS_LINUX_ARMV)
if("${CMAKE_HOST_SYSTEM_PROCESSOR}" STREQUAL "aarch64" OR "${IS_LINUX_ARMV}" GREATER_EQUAL 0
  OR "${CMAKE_OSX_ARCHITECTURES}" STREQUAL "ppc")
  target_link_libraries(phosg atomic)
endif()

add_executable(bindiff src/BinDiff.cc)
add_executable(jsonformat src/JSONFormat.cc)
add_executable(parse-data src/ParseData.cc)
add_executable(phosg-png-conv src/PhosgPNGConv.cc)

target_link_libraries(bindiff phosg)
target_link_libraries(jsonformat phosg)
target_link_libraries(parse-data phosg)
target_link_libraries(phosg-png-conv phosg)

if (WIN32)
  target_link_libraries(bindiff -static -static-libgcc -static-libstdc++)
  target_link_libraries(jsonformat -static -static-libgcc -static-libstdc++)
  target_link_libraries(parse-data -static -static-libgcc -static-libstdc++)
  target_link_libraries(phosg-png-conv -static -static-libgcc -static-libstdc++)
endif()



# Test definitions

enable_testing()

# TODO: Figure out why ToolsTest doesn't work in GitHub Actions and add it back.
# (It works locally on macOS and Ubuntu.)

add_executable(ToolsTest src/ToolsTest.cc)
target_link_libraries(ToolsTest phosg)
if (WIN32)
  target_link_libraries(ToolsTest -static -static-libgcc -static-libstdc++)
endif()

foreach(TestName IN ITEMS ArgumentsTest EncodingTest FilesystemTest HashTest ImageTest JSONTest KDTreeTest LRUMapTest LRUSetTest MathTest ProcessTest StringsTest TimeTest UnitTestTest)
  add_executable(${TestName} src/${TestName}.cc)
  target_link_libraries(${TestName} phosg)
  if (WIN32)
    target_link_libraries(${TestName} -static -static-libgcc -static-libstdc++)
  endif()
  add_test(NAME ${TestName} COMMAND ${TestName})
endforeach()



# Installation configuration

# Package setup
install(
  TARGETS phosg
  EXPORT phosg
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Header files
file(GLOB Headers ${CMAKE_SOURCE_DIR}/src/*.hh)
install(FILES ${Headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/phosg)

# Export definition
install(
  EXPORT phosg
  FILE phosg.cmake
  NAMESPACE phosg::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/phosg
)

# CMake config files
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/phosgConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/phosgConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/phosg
)
set(version 0.1)
set_property(TARGET phosg PROPERTY VERSION ${version})
set_property(TARGET phosg PROPERTY SOVERSION 0)
set_property(TARGET phosg PROPERTY INTERFACE_phosg_MAJOR_VERSION 0)
set_property(TARGET phosg APPEND PROPERTY COMPATIBLE_INTERFACE_STRING phosg_MAJOR_VERSION)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/phosgConfigVersion.cmake"
  VERSION "${version}"
  COMPATIBILITY AnyNewerVersion
)
install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/phosgConfig.cmake" "${CMAKE_CURRENT_BINARY_DIR}/phosgConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/phosg
)

# Executables (separate from package definition)
install(TARGETS bindiff DESTINATION bin)
install(TARGETS jsonformat DESTINATION bin)
install(TARGETS parse-data DESTINATION bin)
install(TARGETS phosg-png-conv DESTINATION bin)
