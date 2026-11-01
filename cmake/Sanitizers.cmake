option(LOANY_SANITIZE "Enable ASan+UBSan (debug-only)" OFF)
add_library(loany_sanitizers INTERFACE)

if(NOT LOANY_SANITIZE)
  return()
endif()

if(NOT CMAKE_CONFIGURATION_TYPES
   AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug"
   AND NOT LOANY_SANITIZE_ALLOW_RELEASE)
  message(FATAL_ERROR "LOANY_SANITIZE is Debug-only ...")
endif()

if(NOT CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
  message(WARNING "LOANY_SANITIZE requested but compiler '${CMAKE_C_COMPILER_ID}' is not supported")
  return()
endif()

if(WIN32 OR ANDROID)
  message(FATAL_ERROR "LOANY_SANITIZE requested but platform '${CMAKE_SYSTEM_NAME}' is not supported")
  return()
endif()

target_compile_options(loany_sanitizers INTERFACE
  -fsanitize=address,undefined
  -fno-omit-frame-pointer
  -fno-sanitize-recover=all
  -O1
)
target_link_options(loany_sanitizers INTERFACE
  -fsanitize=address,undefined
  $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:-no-pie>
)
