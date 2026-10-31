cmake_minimum_required(VERSION 3.18)
project(Loany VERSION 1.0.0 LANGUAGES C)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

option(LOANY_BUILD_SQLITE "Build the sqlite3 statically" OFF)
option(LOANY_BUILD_TESTS "Build the test suite" ON)
option(LOANY_BUILD_CLI   "Build the command-line interface" ON)
option(ENABLE_CLANG_TIDY "Run clang-tidy during build" OFF)
option(ENABLE_SANITIZERS "Build with ASan+UBSan" ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_BIN clang-tidy REQUIRED)
    set(CMAKE_C_CLANG_TIDY
        ${CLANG_TIDY_BIN}
        "--checks=-*,bugprone-*,clang-analyzer-*,performance-*,portability-*,readability-*"
        "--warnings-as-errors=*"
    )
  endif()

  if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang" AND ENABLE_SANITIZERS)
    add_compile_options(
          -fsanitize=address,undefined
          -fno-omit-frame-pointer
          -fno-sanitize-recover=all
          -O1
          -no-pie            # <-- add this
      )
    add_link_options(-fsanitize=address,undefined -no-pie)  # <-- and here
    set(CMAKE_POSITION_INDEPENDENT_CODE OFF)
  endif()
else()
  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    add_compile_options(
              -fno-semantic-interposition
              -fno-plt
              -ffunction-sections
              -fdata-sections
          )
    add_link_options(
              -Wl,--gc-sections
              -static-libgcc
              -Wl,-s
          )
  endif()
endif()

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
  add_compile_options(
        -fstack-protector-strong
        -fstack-clash-protection
        -fcf-protection=full
        -Wformat
        -Wformat-security
    )
  add_link_options(
        -Wl,-z,relro,-z,now
        -Wl,-z,noexecstack
    )
endif()

# _FORTIFY_SOURCE needs optimisation; apply in Release (and RelWithDebInfo)
if(CMAKE_BUILD_TYPE MATCHES "Release|RelWithDebInfo|MinSizeRel")
  add_compile_options(-D_FORTIFY_SOURCE=3)
endif()

if (NOT LOANY_BUILD_SQLITE)
  find_package(SQLite3 3.35 QUIET)
endif()

if(SQLite3_FOUND)
  message(STATUS "Using system SQLite3 ${SQLite3_VERSION}")
else()
  if (NOT LOANY_BUILD_SQLITE)
    message(STATUS "System SQLite3 missing or too old — building 3.54.1")
  else()
    message(STATUS "User requested sqlite3 build — building 3.54.1")
  endif()

  add_library(sqlite3 STATIC third_party/sqlite3/sqlite3.c)
  target_include_directories(sqlite3 PUBLIC third_party/sqlite3)

  find_package(Threads REQUIRED)
  target_link_libraries(sqlite3 PUBLIC Threads::Threads ${CMAKE_DL_LIBS})

  add_library(SQLite3::SQLite3 ALIAS sqlite3)
endif()

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
  add_compile_options(
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
        -Wunterminated-string-initialization

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
elseif(MSVC)
  add_compile_options(/W4 /permissive-)
endif()

add_subdirectory(src)

if(LOANY_BUILD_CLI)
  add_subdirectory(cli)
endif()

if(LOANY_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS loany
    EXPORT LoanyTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT LoanyTargets
    FILE LoanyTargets.cmake
    NAMESPACE Loany::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Loany
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/loanyConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/LoanyConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Loany
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/LoanyConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/LoanyConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/LoanyConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Loany
)
