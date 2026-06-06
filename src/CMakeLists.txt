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

# Set output directory for the executable
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Windows specific settings
if (MSVC)
    add_compile_definitions(__STORMLIB_NO_STATIC_LINK__)
    set(CMAKE_GENERATOR_PLATFORM x64)
endif()

# Create the main executable
add_executable(mpqcli
    main.cpp
    commands.cpp
    completion.cpp
    mpq.cpp
    helpers.cpp
    locales.cpp
    gamerules.cpp
)

# Add dependencies
add_dependencies(mpqcli storm)

# Include directories
target_include_directories(mpqcli PRIVATE 
    "${CMAKE_SOURCE_DIR}/extern/StormLib/src"
    "${CMAKE_BINARY_DIR}"
)

# Link libraries
target_link_libraries(mpqcli PRIVATE storm CLI11::CLI11)
