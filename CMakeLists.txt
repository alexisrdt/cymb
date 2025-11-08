cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

project(
	Cymb
	LANGUAGES C ASM
	VERSION 0.1.0
)

set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CYMB_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<1:${CYMB_OUTPUT_DIRECTORY}>)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY $<1:${CYMB_OUTPUT_DIRECTORY}>)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY $<1:${CYMB_OUTPUT_DIRECTORY}>)

# Cymb library.
configure_file(include/cymb/version.h.in include/cymb/version.h @ONLY)

add_library(
	cymb_lib
	STATIC
	source/cymb/assembly.c
	source/cymb/cymb.c
	source/cymb/diagnostic.c
	source/cymb/lex.c
	source/cymb/memory.c
	source/cymb/options.c
	source/cymb/reader.c
	source/cymb/tree.c
	source/cymb/version.c
)

if(MSVC)
	target_compile_options(cymb_lib PUBLIC /W4 /utf-8)
	target_compile_definitions(cymb_lib PUBLIC _CRT_SECURE_NO_WARNINGS)
else()
	target_compile_options(cymb_lib PUBLIC -Wall -Wextra -pedantic)
endif()

if(WIN32)
	target_compile_definitions(cymb_lib PUBLIC UNICODE _UNICODE)
endif()

target_include_directories(cymb_lib PUBLIC include ${CMAKE_BINARY_DIR}/include)

# Cymb executable.
add_executable(cymb source/main.c)
target_link_libraries(cymb PRIVATE cymb_lib)

# Cymb libc.
add_library(
	cymb_libc
	SHARED
	source/libc/fcntl.s
	source/libc/stdio.c
	source/libc/string.s
	source/libc/unistd.s
)

if(MSVC)
	target_compile_options(cymb_libc PRIVATE /W4 /utf-8)
	target_compile_definitions(cymb_libc PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
	target_compile_options(cymb_libc PRIVATE -Wall -Wextra -pedantic -nostdinc)
	target_link_options(cymb_libc PRIVATE -nostdlib)
endif()

if(WIN32)
	target_compile_definitions(cymb_libc PRIVATE UNICODE _UNICODE)
endif()

target_include_directories(cymb_libc PRIVATE include/libc)

# Cymb crt0.

add_library(
	cymb_crt0
	OBJECT
	source/libc/crt0.s
)

add_custom_target(
	cymb_crt0_copy
	ALL
	DEPENDS cymb_crt0
	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_OBJECTS:cymb_crt0> ${CYMB_OUTPUT_DIRECTORY}/crt0.o
	COMMAND_EXPAND_LISTS
)

# Cymb test.
enable_testing()

add_executable(
	cymb_test
	test/test.c
	test/test_assembly.c
	test/test_lex.c
	test/test_tree.c
)

target_include_directories(cymb_test PRIVATE test)
target_link_libraries(cymb_test PRIVATE cymb_lib)

add_test(NAME cymb_test COMMAND cymb_test WORKING_DIRECTORY ${CYMB_OUTPUT_DIRECTORY})
