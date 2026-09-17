add_executable(mpqcli
    main.cpp
)

target_link_libraries(mpqcli PRIVATE mpqcli_core CLI11::CLI11)
