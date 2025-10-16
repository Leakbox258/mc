cmake_minimum_required(VERSION 3.20)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_STANDARD 26)

project(mc)

add_compile_options("-fno-rtti")
add_compile_options("-Wall")
add_compile_options("-Wextra")

option(ENABLE_ASAN "Enable AddressSanitizer" ON)

if(${CMAKE_BUILD_TYPE} MATCHES Debug)
        add_compile_options("-g" "-fstandalone-debug" "-fsanitize=address" "-fno-omit-frame-pointer")
        add_link_options("-rdynamic" "-fsanitize=address")
endif()

if(ENABLE_ASAN)
	message(STATUS "AddressSantitizer enable")
	add_compile_options("-fsanitize=address" "-fno-omit-frame-pointer")
	add_link_options(-fsanitize=address)
endif()

include_directories(include)

aux_source_directory(lib/mc MC)
aux_source_directory(lib/parser PARSER)

set(ASSEMBLER ${MC} ${PARSER})

add_executable(mc lib/main.cpp ${ASSEMBLER})
