# Re-declare a dependency's interface includes as SYSTEM includes, so clang-tidy
# and the compiler ignore them. Needed for any submodule that exports its own
# target: linking it otherwise pulls its headers in as ordinary -I paths.
function(mark_system target)
    get_target_property(_incs ${target} INTERFACE_INCLUDE_DIRECTORIES)
    if(_incs)
        set_target_properties(${target} PROPERTIES
            INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_incs}")
    endif()
endfunction()
