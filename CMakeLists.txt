cmake_minimum_required(VERSION 3.18)
project(Loany VERSION 1.0.0 LANGUAGES C)

option(LOANY_BUILD_SQLITE "Build the sqlite3 statically" OFF)
option(LOANY_BUILD_TESTS "Build the test suite" ON)
option(LOANY_BUILD_CLI   "Build the command-line interface" ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

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

  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
    cmake_policy(SET CMP0135 NEW)
  endif()

  include(FetchContent)
  FetchContent_Declare(
        sqlite3_amalgamation
        URL      https://www.sqlite.org/2026/sqlite-amalgamation-3540100.zip
        URL_HASH SHA3_256=92281c86e2957f9b9305890f9ac04204caa7b677cc868dc326d942689df28e30
    )
  FetchContent_MakeAvailable(sqlite3_amalgamation)

  add_library(sqlite3 STATIC
        ${sqlite3_amalgamation_SOURCE_DIR}/sqlite3.c
    )
  target_include_directories(sqlite3 PUBLIC
        ${sqlite3_amalgamation_SOURCE_DIR}
    )

  find_package(Threads REQUIRED)
  target_link_libraries(sqlite3 PUBLIC Threads::Threads ${CMAKE_DL_LIBS})

  add_library(SQLite3::SQLite3 ALIAS sqlite3)
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
