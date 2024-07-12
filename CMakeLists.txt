cmake_minimum_required(VERSION 3.5)

project(TIS-100-CXX LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(TIS-100-CXX main.cpp parser.hpp node.hpp)

install(TARGETS TIS-100-CXX
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
