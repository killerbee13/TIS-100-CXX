cmake_minimum_required(VERSION 3.5)

project(TIS-100-CXX LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	message("Clang detected")

	add_compile_options(-Wall -Wextra -pedantic -pedantic-errors
		-Wconversion -Wno-deprecated-declarations -Wuninitialized
		-Wold-style-cast -Wshadow -Wzero-as-null-pointer-constant
		-Wno-mismatched-tags -Wimplicit-fallthrough -Wno-pragmas)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	message("GCC detected")

	add_compile_options(-fcoroutines)
	add_compile_options(-Wall -Wextra -pedantic -pedantic-errors
		-Wconversion -Wno-deprecated-declarations -Wuninitialized
		-Wold-style-cast -Wshadow -Wzero-as-null-pointer-constant
		-Wno-mismatched-tags -Wimplicit-fallthrough -Wno-pragmas
		-Wsign-conversion -Wsign-compare
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

add_compile_options(-fsanitize=undefined,address)
add_link_options(-fsanitize=undefined,address)

add_executable(TIS-100-CXX main.cpp parser.hpp node.hpp T21.hpp T30.hpp io.hpp
	parser.cpp builtin_levels.hpp image.cpp image.hpp T21.cpp logger.hpp
	logger.cpp random_levels.hpp random_levels.cpp tis_random.hpp
	README.md LICENSE)

# used to add non-installed libraries
target_include_directories(TIS-100-CXX
	SYSTEM PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/kblib"
	"${CMAKE_CURRENT_SOURCE_DIR}/tclap/include" "${EXTRA_INCLUDES}"
	INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
	PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

install(TARGETS TIS-100-CXX
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
