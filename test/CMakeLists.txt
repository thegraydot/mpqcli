# Each test layer is a binary of its own, because each links something different.
# unit/ mirrors src/, one file per source file; functional/ has one file per
# subcommand. Fixtures shared by every layer live in fixtures/, one header each.
#
# Every target registers with a LABELS value, which is what lets ctest -L select a
# layer exactly, and with SKIP_RETURN_CODE 4, which is what makes Catch2's SKIP()
# a skip rather than a failure.

# Unit tests link the core and call it directly, never recompiling its sources.
# The core's PUBLIC include directory gives them every header in src/, private
# helpers included.
add_executable(mpqcli_unit_tests
    unit/commands/test_add.cpp
    unit/commands/test_compact.cpp
    unit/commands/test_create.cpp
    unit/commands/test_extract.cpp
    unit/commands/test_info.cpp
    unit/commands/test_list.cpp
    unit/commands/test_read.cpp
    unit/commands/test_remove.cpp
    unit/commands/test_rename.cpp
    unit/commands/test_verify.cpp
    unit/gamerules/test_profiles.cpp
    unit/gamerules/test_rules.cpp
    unit/mpq/test_extract.cpp
    unit/mpq/test_flags.cpp
    unit/mpq/test_query.cpp
    unit/test_errors.cpp
    unit/util/test_capacity.cpp
    unit/util/test_format.cpp
    unit/util/test_hash.cpp
    unit/util/test_locales.cpp
    unit/util/test_path.cpp
    unit/util/test_string.cpp
)

target_include_directories(mpqcli_unit_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
)

target_link_libraries(mpqcli_unit_tests PRIVATE
    mpqcli::core
    mpqcli::warnings
    Catch2::Catch2WithMain
)

catch_discover_tests(mpqcli_unit_tests
    PROPERTIES LABELS "unit" SKIP_RETURN_CODE 4)

# The functional layer spawns the binary, so without the app there is nothing
# for it to test
if(NOT MPQCLI_BUILD_APP)
    return()
endif()

add_executable(mpqcli_functional_tests
    archive_helper.cpp
    subprocess_helper.cpp
    functional/test_about.cpp
    functional/test_add.cpp
    functional/test_compact.cpp
    functional/test_completion.cpp
    functional/test_create.cpp
    functional/test_extract.cpp
    functional/test_info.cpp
    functional/test_list.cpp
    functional/test_read.cpp
    functional/test_remove.cpp
    functional/test_rename.cpp
    functional/test_subprocess_helper.cpp
    functional/test_verify.cpp
    functional/test_version.cpp
)

# Project-owned test headers use PRIVATE without SYSTEM: the helper beside this
# file, and the fixtures every layer shares
target_include_directories(mpqcli_functional_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
)

# extern/subprocess.h is the submodule directory (the repository is literally
# named "subprocess.h"). Marked SYSTEM in a call of its own, never combined with
# the project-owned paths above.
target_include_directories(mpqcli_functional_tests SYSTEM PRIVATE
    "${PROJECT_SOURCE_DIR}/extern/subprocess.h"
)

# Links neither the core nor the binary: the layer drives mpqcli through its CLI
# as a user would, and linking the core would let a test quietly call a function
# instead of running the command. hash-library is vendored, not the core: the
# verify tests compare a printed signature by its MD5.
target_link_libraries(mpqcli_functional_tests PRIVATE
    mpqcli::warnings
    mpqcli::hash_library
    Catch2::Catch2WithMain
)

# A build order, not a link: building the test target on its own must still
# produce a fresh binary for it to spawn
add_dependencies(mpqcli_functional_tests mpqcli)

if(WIN32)
    set(MPQCLI_BINARY_PATH "${PROJECT_BINARY_DIR}/bin/mpqcli.exe")
else()
    set(MPQCLI_BINARY_PATH "${PROJECT_BINARY_DIR}/bin/mpqcli")
endif()

# MPQCLI_BINARY_PATH_OVERRIDE points the functional tests at a binary other than
# the one just built: a downloaded release asset, or an installed copy, to check
# a published artefact behaves. Unset, which is the normal case including in CI,
# the build-tree path above is used.
if(MPQCLI_BINARY_PATH_OVERRIDE)
    set(MPQCLI_BINARY_PATH "${MPQCLI_BINARY_PATH_OVERRIDE}")
endif()

target_compile_definitions(mpqcli_functional_tests PRIVATE
    MPQCLI_BINARY_PATH="${MPQCLI_BINARY_PATH}"
    MPQCLI_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

# Cleared, the tests fall back to the MPQCLI_INTEGRATION_DATA environment
# variable at runtime, and skip when neither names a directory that exists
if(MPQCLI_INTEGRATION_DATA)
    target_compile_definitions(mpqcli_functional_tests PRIVATE
        MPQCLI_INTEGRATION_DATA="${MPQCLI_INTEGRATION_DATA}")
endif()

catch_discover_tests(mpqcli_functional_tests
    PROPERTIES LABELS "functional" SKIP_RETURN_CODE 4)
