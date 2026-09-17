add_executable(mpqcli
    main.cpp
    about.cpp
    completion.cpp
    validators.cpp
    version.cpp
)

target_link_libraries(mpqcli PRIVATE mpqcli_core CLI11::CLI11)
