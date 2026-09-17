cmake_minimum_required(VERSION 3.25)

project(MPQCLI VERSION 0.10.2)

# An option() name is global to the CMake cache, including a consumer's, so these
# carry the project prefix; a bare BUILD_STATIC claims a name anyone might want.
option(MPQCLI_BUILD_APP "Build the mpqcli binary" ON)
option(MPQCLI_BUILD_STATIC "Build static binary" OFF)

# Set project defaults
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Windows specific settings
if(MSVC)
    add_compile_definitions(__STORMLIB_NO_STATIC_LINK__)
    set(CMAKE_GENERATOR_PLATFORM x64)
endif()

# Static linking configuration
if(MPQCLI_BUILD_STATIC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    set(BUILD_SHARED_LIBS OFF)
endif()

# Determine git commit hash
execute_process(
    COMMAND git rev-parse HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_STRIP_TRAILING_WHITESPACE
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    RESULT_VARIABLE GIT_COMMIT_RESULT
    ERROR_QUIET
)
if(NOT GIT_COMMIT_RESULT EQUAL 0 OR NOT GIT_COMMIT_HASH)
    set(GIT_COMMIT_HASH "unknown")
endif()

# Handle StormLib dependency
if (NOT EXISTS "${CMAKE_SOURCE_DIR}/extern/StormLib/CMakeLists.txt")
    message(FATAL_ERROR
"Missing dependency: StormLib
mpqcli requires the StormLib library.
It is provided as a submodule of this repository.
Did you forget to execute the following commands?
   git submodule init
   git submodule update")
endif()

# Use StormLib's bundled zlib/bzip2/libtommath sources instead of requiring
# system packages. This keeps mpqcli buildable everywhere (musl/glibc Docker
# images, MSVC, plain `cmake -B build`) without extra system dependencies.
set(STORM_USE_BUNDLED_LIBRARIES ON CACHE BOOL "" FORCE)

add_subdirectory(extern/StormLib)

# Handle CLI11 dependency. Only app/ parses arguments, so the core never sees it.
if(MPQCLI_BUILD_APP)
    if (NOT EXISTS "${CMAKE_SOURCE_DIR}/extern/CLI11/CMakeLists.txt")
        message(FATAL_ERROR
"Missing dependency: CLI11
mpqcli requires the CLI11 library.
It is provided as a submodule of this repository.
Did you forget to execute the following commands?
   git submodule init
   git submodule update")
    endif()

    add_subdirectory(extern/CLI11)
endif()

# Handle hash-library dependency (CRC32 + MD5 for the add --update checks)
if (NOT EXISTS "${CMAKE_SOURCE_DIR}/extern/hash-library/crc32.cpp")
    message(FATAL_ERROR
"Missing dependency: hash-library
mpqcli requires the hash-library library.
It is provided as a submodule of this repository.
Did you forget to execute the following commands?
   git submodule init
   git submodule update")
endif()

# hash-library ships no CMakeLists; compile only the two algorithms we use
add_library(hash-library STATIC
    extern/hash-library/crc32.cpp
    extern/hash-library/md5.cpp
)
target_include_directories(hash-library SYSTEM INTERFACE "${CMAKE_SOURCE_DIR}/extern")

# Unconditional: building the core without the app is what proves nothing in
# src/ has picked up a dependency on app/.
add_subdirectory(src)

if(MPQCLI_BUILD_APP)
    add_subdirectory(app)
endif()
