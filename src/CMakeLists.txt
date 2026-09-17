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
    gamerules.cpp
    util/capacity.cpp
    util/format.cpp
    util/hash.cpp
    util/locales.cpp
    util/path.cpp
)

add_dependencies(mpqcli_core storm)

# PUBLIC so app/ and the test binaries can include these headers by name
target_include_directories(mpqcli_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_include_directories(mpqcli_core SYSTEM PUBLIC
    "${CMAKE_SOURCE_DIR}/extern/StormLib/src"
)

target_link_libraries(mpqcli_core PUBLIC storm hash-library)
