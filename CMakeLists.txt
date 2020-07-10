cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(dwmipcpp CXX)

option(BUILD_EXAMPLES "Build examples" ON)

add_library(${PROJECT_NAME} STATIC
    ${PROJECT_SOURCE_DIR}/include/connection.hpp
    ${PROJECT_SOURCE_DIR}/include/errors.hpp
    ${PROJECT_SOURCE_DIR}/include/packet.hpp
    ${PROJECT_SOURCE_DIR}/src/connection.cpp
    ${PROJECT_SOURCE_DIR}/src/errors.cpp
    ${PROJECT_SOURCE_DIR}/src/packet.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_compile_options(${PROJECT_NAME} PRIVATE -std=c++11 -Wall -Wextra)
target_compile_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>: -g3 -DDEBUG>)
target_compile_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Release>: -O2>)

find_package(PkgConfig)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

target_link_libraries(${PROJECT_NAME} PUBLIC ${JSONCPP_LIBRARIES})
target_include_directories(${PROJECT_NAME} PUBLIC ${JSONCPP_INCLUDEDIR})

get_directory_property(HAS_PARENT PARENT_DIRECTORY)
set(DWMIPCPP_LIBRARIES ${PROJECT_NAME})
set(DWMIPCPP_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include ${JSONCPP_INCLUDEDIR})

if (BUILD_EXAMPLES)
    add_subdirectory("${PROJECT_SOURCE_DIR}/examples")
endif()
