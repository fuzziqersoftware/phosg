cmake_minimum_required(VERSION 3.10)



# Project setup

project(phosg)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
if (MSVC)
    # Disabled warnings:
    # 4458 = declaration of '%s' hides class member (I use this->x for members)
    add_compile_options(/W4 /WX /wd4458)
else()
    add_compile_options(-Wall -Wextra -Werror -Wno-strict-aliasing -Wno-unused-result -Wno-overflow)
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_BUILD_TYPE Debug)
set(CTEST_OUTPUT_ON_FAILURE ON)

include_directories("/usr/local/include")
link_directories("/usr/local/lib")



# Library and executable definitions

add_library(phosg src/Encoding.cc src/Filesystem.cc src/Hash.cc src/Image.cc src/JSON.cc src/Network.cc src/Process.cc src/Random.cc src/Strings.cc src/Time.cc src/Tools.cc src/UnitTest.cc)
target_link_libraries(phosg pthread)

# It seems that on some Linux variants (e.g. Raspbian) we also need -latomic,
# but this library does not exist on others (e.g. Ubuntu) nor on macOS
message(STATUS "Target architecture is ${CMAKE_HOST_SYSTEM_PROCESSOR}")
string(FIND ${CMAKE_HOST_SYSTEM_PROCESSOR} "armv" IS_LINUX_ARMV)
if(${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "aarch64" OR ${IS_LINUX_ARMV} GREATER_EQUAL 0)
  target_link_libraries(phosg atomic)
endif()

add_executable(jsonformat src/JSONFormat.cc)
target_link_libraries(jsonformat phosg)

add_executable(parse-data src/ParseData.cc)
target_link_libraries(parse-data phosg)



# Test definitions

enable_testing()

# TODO: Figure out why ToolsTest doesn't work in GitHub Actions and add it back.
# (It works locally on macOS and Ubuntu.)

add_executable(ToolsTest src/ToolsTest.cc)
target_link_libraries(ToolsTest phosg)

foreach(TestName IN ITEMS EncodingTest FilesystemTest HashTest ImageTest JSONTest KDTreeTest LRUSetTest LRUMapTest ProcessTest StringsTest TimeTest UnitTestTest)
  add_executable(${TestName} src/${TestName}.cc)
  target_link_libraries(${TestName} phosg)
  add_test(NAME ${TestName} COMMAND ${TestName})
endforeach()



# Installation configuration

file(GLOB Headers ${CMAKE_SOURCE_DIR}/src/*.hh)
install(TARGETS phosg EXPORT phosg DESTINATION lib)
install(TARGETS jsonformat DESTINATION bin)
install(TARGETS parse-data DESTINATION bin)
install(FILES ${Headers} DESTINATION include/phosg)
install(FILES phosg-config.cmake DESTINATION lib)
install(EXPORT phosg DESTINATION lib)
