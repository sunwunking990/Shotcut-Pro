# Additional security configurations
function(configure_security target)
    # Runtime security checks
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_definitions(${target} PRIVATE
            -D_GLIBCXX_ASSERTIONS=1
            -D_LIBCPP_DEBUG=0  # Disable libcpp debug in release
        )
    endif()
    
    # Disable insecure functions
    if(WIN32)
        target_compile_definitions(${target} PRIVATE
            -D_CRT_SECURE_NO_WARNINGS
            -D_CRT_SECURE_NO_DEPRECATE
        )
    endif()
    
    # Stack smashing protection
    if(UNIX AND NOT APPLE)
        target_link_options(${target} PRIVATE -Wl,-z,noexecstack)
    endif()
    
    # RELRO (Read-Only Relocation)
    if(UNIX)
        target_link_options(${target} PRIVATE -Wl,-z,relro -Wl,-z,now)
    endif()
    
    # Additional compiler warnings for security
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wsign-conversion
            -Wcast-align
            -Wstrict-overflow=5
            -Wstrict-prototypes
            -Wmissing-prototypes
            -Wredundant-decls
            -Wunreachable-code
        )
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(${target} PRIVATE
            /W4
            /Wall
        )
    endif()
endfunction()

# Security validation
function(validate_security_configuration target)
    message(STATUS "Validating security configuration for ${target}")
    
    # Check if security flags are applied
    get_target_property(compile_options ${target} COMPILE_OPTIONS)
    get_target_property(link_options ${target} LINK_OPTIONS)
    
    if(NOT compile_options)
        message(WARNING "No compile options found for ${target}")
    endif()
    
    if(NOT link_options)
        message(WARNING "No link options found for ${target}")
    endif()
    
    # Validate essential security features
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        foreach(flag -fstack-protector-strong -D_FORTIFY_SOURCE=3)
            if(NOT flag IN_LIST compile_options)
                message(WARNING "Security flag ${flag} not found for ${target}")
            endif()
        endforeach()
    endif()
endfunction()
