cmake_minimum_required(3.20)
project(mc_linker)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)
set(CMAKE_CXX_STANDARD 2C CACHE STRING "C++ 
standard to conform to")
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")

set(NO_RTTI "-fno-rtti")
set(W_LIFETIME "-Wlifetime")
add_definitions(${NO_RTTI})
add_definitions(${W_LIFETIME})