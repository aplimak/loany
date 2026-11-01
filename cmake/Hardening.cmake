add_library(loany_hardening INTERFACE)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
  # Common, safe hardening
  target_compile_options(loany_hardening INTERFACE
    -fstack-protector-strong
    -Wformat -Wformat-security
  )

  # Size: function/data sections let the linker drop unreferenced code.
  # Harmless with shared libs, but the payoff is on executables.
  target_compile_options(loany_hardening INTERFACE
    $<$<NOT:$<CONFIG:Debug>>:-ffunction-sections>
    $<$<NOT:$<CONFIG:Debug>>:-fdata-sections>
  )
  target_link_options(loany_hardening INTERFACE
    $<$<NOT:$<CONFIG:Debug>>:-Wl,--gc-sections>
  )

  # ELF-only hardening — explicitly exclude Android
  if(NOT WIN32 AND NOT ANDROID AND NOT CMAKE_SYSTEM_NAME STREQUAL "Android")
    target_compile_options(loany_hardening INTERFACE
      -fstack-clash-protection
      -fcf-protection=full
    )
    target_link_options(loany_hardening INTERFACE
      -Wl,-z,relro,-z,now
      -Wl,-z,noexecstack
    )

    # Performance: skip PLT and interposition for non-default-visible symbols.
    # Only meaningful on ELF; Android's Bionic toolchain doesn't gain here
    # and -fno-plt interacts poorly with some NDK linker versions.
    target_compile_options(loany_hardening INTERFACE
        $<$<NOT:$<CONFIG:Debug>>:-fno-semantic-interposition>
        $<$<NOT:$<CONFIG:Debug>>:-fno-plt>
      )
  endif()

  # _FORTIFY_SOURCE needs optimisation and must not coexist with sanitizers
  if(NOT LOANY_SANITIZE)
    target_compile_definitions(loany_hardening INTERFACE
      $<$<NOT:$<CONFIG:Debug>>:_FORTIFY_SOURCE=3>
    )
  endif()
endif()
