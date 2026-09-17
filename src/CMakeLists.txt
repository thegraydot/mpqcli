# Internal to this repository, so no include/ and no alias. Keeping the logic in
# a library rather than beside main() is what lets test binaries link it; a test
# that recompiles the sources instead is a second build that drifts from the
# shipped one.
add_library(mpqcli_core STATIC
    commands.cpp
    mpq.cpp
    helpers.cpp
    locales.cpp
    gamerules.cpp
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
