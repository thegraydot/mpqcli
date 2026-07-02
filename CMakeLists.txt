cmake_minimum_required(VERSION 3.10)

project(MPQCLI VERSION 0.10.1)

# Options
option(BUILD_MPQCLI "Build the mpqcli CLI app" ON)
option(BUILD_STATIC "Build static binary" OFF)

# Set project defaults
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Static linking configuration
if(BUILD_STATIC)
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

add_subdirectory(extern/StormLib)

# Handle CLI11 dependency
if(BUILD_MPQCLI)
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

    # Add the main application
    add_subdirectory(src)
endif()
