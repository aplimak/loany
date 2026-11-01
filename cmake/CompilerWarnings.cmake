function(loany_set_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /permissive-)
  elseif(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(${target} PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wundef
    -Wformat=2
    -Wcast-align
    -Wcast-qual
    -Wwrite-strings
    -Wmissing-prototypes
    -Wstrict-prototypes
    -Wold-style-definition

    -Wpointer-arith
    -Wswitch-enum
    -Wredundant-decls
    -Wnested-externs
    -Wdouble-promotion

    -Wconversion
    -Wsign-conversion
    -Wfloat-equal
    -Wpadded
    -Wuseless-cast

    -Waggregate-return
    -Wmissing-declarations
    -Wunsuffixed-float-constants
    -Wswitch-default
    -Wformat-nonliteral
    -Wdisabled-optimization
    )
  endif()
  if(LOANY_HAVE_W_UNINIT_STR)
    target_compile_options(${target} PRIVATE
        -Wunterminated-string-initialization
      )
  endif()
endfunction()
