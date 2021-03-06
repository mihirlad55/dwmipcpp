cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(dwmipcpp CXX)

option(BUILD_EXAMPLES "Build examples" OFF)
option(BUILD_JSONCPP_STATIC "Build and link jsoncpp as a static library" OFF)

add_library(${PROJECT_NAME} STATIC
    ${PROJECT_SOURCE_DIR}/include/dwmipcpp/connection.hpp
    ${PROJECT_SOURCE_DIR}/include/dwmipcpp/errors.hpp
    ${PROJECT_SOURCE_DIR}/include/dwmipcpp/packet.hpp
    ${PROJECT_SOURCE_DIR}/include/dwmipcpp/types.hpp
    ${PROJECT_SOURCE_DIR}/include/dwmipcpp/util.hpp
    ${PROJECT_SOURCE_DIR}/src/connection.cpp
    ${PROJECT_SOURCE_DIR}/src/errors.cpp
    ${PROJECT_SOURCE_DIR}/src/packet.cpp
    ${PROJECT_SOURCE_DIR}/src/types.cpp
    ${PROJECT_SOURCE_DIR}/src/util.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_compile_options(${PROJECT_NAME} PRIVATE -std=c++11 -Wall -Wextra)
target_compile_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>: -g3 -DDEBUG>)
target_compile_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Release>: -O2>)

set(DWMIPCPP_LIBRARIES ${PROJECT_NAME})

# Build and link jsoncpp as a static library. This is useful for testing older
# versions of jsoncpp such as 1.7.2 which is currently latest for Ubuntu
if (BUILD_JSONCPP_STATIC)
    add_subdirectory("lib/jsoncpp")
    target_link_libraries(${PROJECT_NAME} PUBLIC "jsoncpp_lib_static")
    target_include_directories(${PROJECT_NAME} PUBLIC "lib/jsoncpp/include")

    set(DWMIPCPP_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include "lib/jsoncpp")
else()
    find_package(PkgConfig)
    pkg_check_modules(JSONCPP REQUIRED jsoncpp)

    target_link_libraries(${PROJECT_NAME} PUBLIC ${JSONCPP_LIBRARIES})
    target_include_directories(${PROJECT_NAME} PUBLIC ${JSONCPP_INCLUDEDIR})

    set(DWMIPCPP_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include "${JSONCPP_LIBRARIES}")
endif()

get_directory_property(HAS_PARENT PARENT_DIRECTORY)
if(HAS_PARENT)
    set(DWMIPCPP_LIBRARIES ${DWMIPCPP_LIBRARIES} PARENT_SCOPE)
    set(DWMIPCPP_INCLUDE_DIRS ${DWMIPCPP_INCLUDE_DIRS} PARENT_SCOPE)
endif()

if (BUILD_EXAMPLES)
    add_subdirectory("${PROJECT_SOURCE_DIR}/examples")
endif()
