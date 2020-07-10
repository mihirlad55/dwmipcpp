cmake_minimum_required(VERSION 3.0)
project(dwmipcpp-examples)

include_directories(
    ${DWMIPCPP_INCLUDE_DIRS}
)

link_directories(
    ${sDWMIPCPP_LIBRARY_DIRS}
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g3 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")

add_executable(dump-all dump_all.cpp)
target_link_libraries(dump-all ${DWMIPCPP_LIBRARIES})
