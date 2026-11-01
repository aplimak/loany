cmake_minimum_required(VERSION 3.18)
project(Loany VERSION 1.0.0 LANGUAGES C)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(LOANY_BUILD_SQLITE "Build the sqlite3 locally" OFF)
option(LOANY_BUILD_TESTS "Build the test suite" ON)
option(LOANY_BUILD_CLI   "Build the command-line interface" ON)
option(ENABLE_CLANG_TIDY "Run clang-tidy during build (only debug builds)" OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerWarnings)
include(Sanitizers)
include(Hardening)

if (ENABLE_CLANG_TIDY AND CMAKE_BUILD_TYPE STREQUAL "Debug")
  find_program(CLANG_TIDY_BIN clang-tidy REQUIRED)
  set(CMAKE_C_CLANG_TIDY
        ${CLANG_TIDY_BIN}
        "--checks=-*,bugprone-*,clang-analyzer-*,performance-*,portability-*,readability-*"
        "--warnings-as-errors=*"
    )
endif()

if (NOT LOANY_BUILD_SQLITE)
  find_package(SQLite3 3.35 QUIET)
endif()

if(SQLite3_FOUND)
  message(STATUS "Using system SQLite3 ${SQLite3_VERSION}")
  if(NOT TARGET SQLite3::SQLite3)
    add_library(SQLite3::SQLite3 ALIAS SQLite::SQLite3)
  endif()
else()
  if (NOT LOANY_BUILD_SQLITE)
    message(STATUS "System SQLite3 missing or too old — building 3.54.1")
  else()
    message(STATUS "User requested sqlite3 build — building 3.54.1")
  endif()

  add_subdirectory(third_party/sqlite3)
endif()

include(CheckCCompilerFlag)
check_c_compiler_flag(-Wunterminated-string-initialization
    LOANY_HAVE_W_UNINIT_STR
  )

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
