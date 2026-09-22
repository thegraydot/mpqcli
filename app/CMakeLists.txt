# Re-run configure when a script is edited, so the embedded copy cannot go stale
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/completion/mpqcli.bash"
    "${PROJECT_SOURCE_DIR}/completion/mpqcli.zsh"
    "${PROJECT_SOURCE_DIR}/completion/mpqcli.ps1"
    "${PROJECT_SOURCE_DIR}/completion/mpqcli.fish"
)
file(READ "${PROJECT_SOURCE_DIR}/completion/mpqcli.bash" BASH_COMPLETION_SCRIPT)
file(READ "${PROJECT_SOURCE_DIR}/completion/mpqcli.zsh" ZSH_COMPLETION_SCRIPT)
file(READ "${PROJECT_SOURCE_DIR}/completion/mpqcli.ps1" PS_COMPLETION_SCRIPT)
file(READ "${PROJECT_SOURCE_DIR}/completion/mpqcli.fish" FISH_COMPLETION_SCRIPT)
configure_file(
    "${PROJECT_SOURCE_DIR}/cmake/completion_data.h.in"
    "${PROJECT_BINARY_DIR}/include/mpqcli/completion_data.h"
    @ONLY
)

add_executable(mpqcli
    main.cpp
    about.cpp
    completion.cpp
    validators.cpp
    version.cpp
)

# An include tree rather than PROJECT_BINARY_DIR itself, which would expose
# every generated file in the build tree to #include
target_include_directories(mpqcli PRIVATE "${PROJECT_BINARY_DIR}/include")

target_link_libraries(mpqcli PRIVATE mpqcli_core CLI11::CLI11)
