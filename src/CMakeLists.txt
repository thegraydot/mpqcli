# The warning bar, defined once and linked privately by every target the project
# owns. -Wsign-conversion is spelled out because clang folds it into -Wconversion
# and GCC does not; left implicit, the two compilers hold different bars for the
# same source. The vendored targets deliberately do not link this
add_library(mpqcli_warnings INTERFACE)
target_compile_options(mpqcli_warnings INTERFACE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wnon-virtual-dtor -Wold-style-cast>
)

if(MPQCLI_WERROR)
    target_compile_options(mpqcli_warnings INTERFACE
        $<$<CXX_COMPILER_ID:MSVC>:/WX>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Werror>
    )
endif()

add_library(mpqcli::warnings ALIAS mpqcli_warnings)

# Generated beside the core library so the core and the CLI report the same version
configure_file(
    "${PROJECT_SOURCE_DIR}/cmake/version.h.in"
    "${PROJECT_BINARY_DIR}/include/mpqcli/version.h"
    @ONLY
)

# Internal to this repository, so no include/ and no alias. Keeping the logic in
# a library rather than beside main() is what lets test binaries link it; a test
# that recompiles the sources instead is a second build that drifts from the
# shipped one.
add_library(mpqcli_core STATIC
    cli/add.cpp
    cli/compact.cpp
    cli/create.cpp
    cli/extract.cpp
    cli/info.cpp
    cli/list.cpp
    cli/read.cpp
    cli/remove.cpp
    cli/rename.cpp
    cli/verify.cpp
    mpq/add.cpp
    mpq/archive.cpp
    mpq/compact.cpp
    mpq/create.cpp
    mpq/extract.cpp
    mpq/flags.cpp
    mpq/info.cpp
    mpq/list.cpp
    mpq/query.cpp
    mpq/read.cpp
    mpq/remove.cpp
    mpq/rename.cpp
    mpq/verify.cpp
    gamerules/profile_data.cpp
    gamerules/profiles.cpp
    gamerules/rules.cpp
    util/capacity.cpp
    util/format.cpp
    util/hash.cpp
    util/locales.cpp
    util/path.cpp
    util/string.cpp
)

add_dependencies(mpqcli_core storm)

# PUBLIC so app/ and the test binaries can include these headers by name
target_include_directories(mpqcli_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PROJECT_BINARY_DIR}/include"
)
target_include_directories(mpqcli_core SYSTEM PUBLIC
    "${CMAKE_SOURCE_DIR}/extern/StormLib/src"
)

target_link_libraries(mpqcli_core PUBLIC storm hash-library)
target_link_libraries(mpqcli_core PRIVATE mpqcli::warnings)
