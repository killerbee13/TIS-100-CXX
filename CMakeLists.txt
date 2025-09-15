cmake_minimum_required(VERSION 3.18)

if (CMAKE_GENERATOR MATCHES "Ninja")
file(
    WRITE "${CMAKE_BINARY_DIR}/GNUMakeRulesOverwrite.cmake"
        "STRING(REPLACE \"-MD\" \"-MMD\" CMAKE_DEPFILE_FLAGS_C \"\${CMAKE_DEPFILE_FLAGS_C}\")\n"
        "STRING(REPLACE \"-MD\" \"-MMD\" CMAKE_DEPFILE_FLAGS_CXX \"\${CMAKE_DEPFILE_FLAGS_CXX}\")\n"
)
set(CMAKE_USER_MAKE_RULES_OVERRIDE "${CMAKE_BINARY_DIR}/GNUMakeRulesOverwrite.cmake" CACHE INTERNAL "")
endif()

project(TIS-100-CXX LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	message("Clang detected")

	add_compile_options(-Wall -Wextra -pedantic -pedantic-errors
		-Wconversion -Wno-deprecated-declarations -Wuninitialized
		-Wold-style-cast -Wshadow -Wzero-as-null-pointer-constant
		-Wno-mismatched-tags -Wimplicit-fallthrough -Wno-pragmas
		-Wno-sign-conversion -Werror=implicit-fallthrough)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	message("GCC detected")

	add_compile_options(-fcoroutines)
	add_compile_options(-Wall -Wextra -pedantic -pedantic-errors
		-Wconversion -Wno-deprecated-declarations -Wuninitialized
		-Wold-style-cast -Wshadow=compatible-local -Wzero-as-null-pointer-constant
		-Wno-mismatched-tags -Wimplicit-fallthrough -Wno-pragmas
		-Wsign-compare -Werror=implicit-fallthrough -Wsuggest-override
		-fconcepts-diagnostics-depth=10)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
	message("ICC detected")

	add_compile_options(-fcoroutines)
	add_compile_options(-Wall -Wextra -pedantic -pedantic-errors
		-Wconversion -Wno-deprecated-declarations -Wuninitialized
		-Wold-style-cast -Wshadow -Wzero-as-null-pointer-constant
		-Wno-mismatched-tags -Wimplicit-fallthrough -Wno-pragmas)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	message("MSVC detected")

	add_compile_options(/W3)
endif()

if(CMAKE_BUILD_TYPE MATCHES "Debug")
	add_compile_options(-fsanitize=undefined,address)
	add_link_options(-fsanitize=undefined,address)
	add_compile_options(-Og)
endif()

include(CheckIPOSupported)
check_ipo_supported()

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)

# common stuff
add_library(common OBJECT
	builtin_specs.hpp field.cpp field.hpp game.hpp
	image.hpp instr.hpp io.hpp levels_builtin_tests.cpp levels.cpp levels.hpp
	logger.cpp logger.hpp node.hpp parser.cpp parser.hpp sim.cpp sim.hpp
	T21.hpp T30.hpp tests.hpp tis100.h tis_random.hpp utils.hpp
)
set_target_properties(common PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Build shared library libTIS100.so
add_library(libTIS100 SHARED tis100.cpp $<TARGET_OBJECTS:common>)
set_target_properties(libTIS100 PROPERTIES
	LINK_FLAGS "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/libTIS100_exports.map"
	PREFIX ""
)

# Build executable
add_executable(TIS-100-CXX main.cpp $<TARGET_OBJECTS:common>)

# used to add non-installed libraries
include_directories(SYSTEM PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/kblib"
	"${CMAKE_CURRENT_SOURCE_DIR}/tclap/include" ${EXTRA_INCLUDES}
	INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
	PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

option(TIS_ENABLE_LUA "Enable Lua support to run custom puzzles" ON)
option(TIS_ENABLE_DEBUG "Enable Debug log support for low level testing" ON)

if(NOT CMAKE_BUILD_TYPE MATCHES "Debug")
	# Used to generate the standalone build for the GitHub release
	# Static linking interferes with sanitizers
	target_link_options(libTIS100 PRIVATE -static-libgcc -static-libstdc++)
	target_link_options(TIS-100-CXX PRIVATE -static)
endif()

set(LUAJIT_INCLUDE_DIR "/usr/include/luajit-2.1" CACHE PATH "Path to the luajit include directory")
set(LUAJIT_LIB "luajit-5.1" CACHE FILEPATH "Path to the luajit library to link with")

if(TIS_ENABLE_LUA)
	add_compile_definitions(PUBLIC TIS_ENABLE_LUA)
	include_directories(SYSTEM PRIVATE
		"${LUAJIT_INCLUDE_DIR}"
		"${CMAKE_CURRENT_SOURCE_DIR}/sol2/include")
	target_link_libraries(libTIS100 PRIVATE "${LUAJIT_LIB}")
	target_link_libraries(TIS-100-CXX PRIVATE "${LUAJIT_LIB}")
endif()
if(TIS_ENABLE_DEBUG)
	add_compile_definitions(PUBLIC TIS_ENABLE_DEBUG)
endif()

install(TARGETS libTIS100 TIS-100-CXX)
