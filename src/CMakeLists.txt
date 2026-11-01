set(SQL_DIR      "${CMAKE_CURRENT_SOURCE_DIR}/sql")
set(SQL_SCRIPT   "${CMAKE_SOURCE_DIR}/cmake/compile_sql.sh")
set(GEN_INC_DIR  "${CMAKE_CURRENT_BINARY_DIR}/include")
set(GEN_DIR      "${GEN_INC_DIR}/generated")
set(GEN_HEADER   "${GEN_DIR}/sql_str.h")

file(GLOB_RECURSE SQL_SOURCES CONFIGURE_DEPENDS "${SQL_DIR}/*.sql")

add_custom_command(
    OUTPUT  "${GEN_HEADER}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${GEN_DIR}"
    COMMAND bash "${SQL_SCRIPT}" "${SQL_DIR}" "${GEN_HEADER}" loany_sql
    DEPENDS "${SQL_SCRIPT}" ${SQL_SOURCES}
    COMMENT "Embedding SQL from ${SQL_DIR} -> ${GEN_HEADER}"
    VERBATIM
)

add_custom_target(loany_db_sql_str DEPENDS "${GEN_HEADER}")

add_library(loany
    loany.c
    db.c
    error.c
)

add_library(Loany::loany ALIAS loany)

target_include_directories(loany PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<INSTALL_INTERFACE:include>
)

add_dependencies(loany loany_db_sql_str)
target_include_directories(loany PRIVATE "${GEN_INC_DIR}")

target_link_libraries(loany PUBLIC $<BUILD_INTERFACE:SQLite3::SQLite3>)

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(loany PRIVATE -Werror)
  elseif(MSVC)
    target_compile_options(loany PRIVATE /WX)
  endif()
  if(ipo_supported)
    set_target_properties(loany PROPERTIES
          INTERPROCEDURAL_OPTIMIZATION TRUE
      )
  endif()
endif()
