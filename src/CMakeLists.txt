# Configure version header
configure_file(mpqcli.h.in "${CMAKE_BINARY_DIR}/mpqcli.h")

# Embed completion scripts into generated headers.
# Declare the script files as configure-time dependencies so CMake re-generates
# completion_data.h whenever the bash or zsh source scripts are edited.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/completion/mpqcli.bash"
    "${CMAKE_CURRENT_SOURCE_DIR}/completion/mpqcli.zsh"
    "${CMAKE_CURRENT_SOURCE_DIR}/completion/mpqcli.ps1"
    "${CMAKE_CURRENT_SOURCE_DIR}/completion/mpqcli.fish"
)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/completion/mpqcli.bash" BASH_COMPLETION_SCRIPT)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/completion/mpqcli.zsh" ZSH_COMPLETION_SCRIPT)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/completion/mpqcli.ps1" PS_COMPLETION_SCRIPT)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/completion/mpqcli.fish" FISH_COMPLETION_SCRIPT)
configure_file(completion_data.h.in "${CMAKE_BINARY_DIR}/completion_data.h")

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
    "${CMAKE_BINARY_DIR}"
)
target_include_directories(mpqcli_core SYSTEM PUBLIC
    "${CMAKE_SOURCE_DIR}/extern/StormLib/src"
)

target_link_libraries(mpqcli_core PUBLIC storm hash-library)
