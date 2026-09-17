add_executable(mpqcli
    main.cpp
    validators.cpp
)

target_link_libraries(mpqcli PRIVATE mpqcli_core CLI11::CLI11)
