cmake_minimum_required(VERSION 3.10)



# Project setup

project(phosg)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
if (MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

include_directories("/usr/local/include")
link_directories("/usr/local/lib")



# Library and executable definitions

add_library(phosg src/Concurrency.cc src/ConsistentHashRing.cc src/Encoding.cc src/FileCache.cc src/Filesystem.cc src/Hash.cc src/Image.cc src/JSON.cc src/Network.cc src/Process.cc src/Random.cc src/Strings.cc src/Time.cc src/UnitTest.cc)

add_executable(jsonformat src/JSONFormat.cc)
target_link_libraries(jsonformat phosg)



# Test definitions

enable_testing()

foreach(TestName IN ITEMS ConsistentHashRingTest EncodingTest FileCacheTest FilesystemTest HashTest ImageTest JSONTest KDTreeTest LRUSetTest ProcessTest StringsTest TimeTest UnitTestTest)
  add_executable(${TestName} src/${TestName}.cc)
  target_link_libraries(${TestName} phosg)
  add_test(NAME ${TestName} COMMAND ${TestName})
endforeach()



# Installation configuration

file(GLOB Headers ${CMAKE_SOURCE_DIR}/src/*.hh)
install(TARGETS phosg DESTINATION lib)
install(TARGETS jsonformat DESTINATION bin)
install(FILES ${Headers} DESTINATION include/phosg)