# Security hardening flags based on compiler
function(apply_compiler_hardening target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        # Stack protection
        target_compile_options(${target} PRIVATE
            -fstack-protector-strong
            -fstack-clash-protection
        )
        
        # Control Flow Integrity
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            target_compile_options(${target} PRIVATE -fcf-protection=full)
        endif()
        
        # Position Independent Code
        target_compile_options(${target} PRIVATE -fPIE -fPIC)
        
        # Fortify Source
        target_compile_definitions(${target} PRIVATE
            -D_FORTIFY_SOURCE=3
        )
        
        # Format security
        target_compile_options(${target} PRIVATE
            -Wformat -Wformat-security
            -Werror=format-security
        )
        
        # Linker hardening
        target_link_options(${target} PRIVATE
            -Wl,-z,relro
            -Wl,-z,now
            -pie
        )
        
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        # Microsoft security features - minimal approach to avoid conflicts
        target_compile_options(${target} PRIVATE
            /GS                    # Buffer security check
            /guard:cf              # Control flow guard
            /sdl                   # Security development lifecycle
            /DYNAMICBASE            # Address space layout randomization
            /wd4820                 # Disable padding warnings
            /wd4625 /wd4626        # Disable deleted copy constructor warnings
            /wd4514                  # Disable unreferenced inline function warnings
            /wd4100                  # Disable unreferenced parameter warnings
            /wd4191                  # Disable unsafe conversion warnings
            /wd4365                  # Disable signed/unsigned mismatch warnings
            /W3                     # Use moderate warning level
        )
    endif()
    
    # Sanitizers for debug builds
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(${target} PRIVATE
                -fsanitize=address
                -fsanitize=undefined
                -fsanitize=leak
            )
            target_link_options(${target} PRIVATE
                -fsanitize=address
                -fsanitize=undefined
                -fsanitize=leak
            )
        elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
            target_compile_options(${target} PRIVATE /fsanitize=address)
            target_link_options(${target} PRIVATE /fsanitize=address)
        endif()
    endif()
endfunction()

# LTO optimization for release builds (temporarily disabled due to linker issues)
function(enable_lto target)
    # Temporarily disable LTO to fix linker issues
    message(STATUS "LTO temporarily disabled for target: ${target}")
    # if(CMAKE_BUILD_TYPE STREQUAL "Release")
    #     include(CheckIPOSupported)
    #     check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
    #     if(ipo_supported)
    #         set_property(TARGET ${target} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    #         message(STATUS "LTO enabled for target: ${target}")
    #     else()
    #         message(WARNING "LTO not supported for target ${target}: ${ipo_error}")
    #     endif()
    # endif()
endfunction()

# Apply hardening to all targets by default
function(apply_default_hardening target)
    apply_compiler_hardening(${target})
    enable_lto(${target})
endfunction()
