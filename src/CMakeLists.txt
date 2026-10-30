set(SQL_FILES
    schema/people.sql
    schema/loans.sql
    schema/payments.sql
)

foreach(SQL_FILE IN LISTS SQL_FILES)
  string(REGEX REPLACE "\\.[^.]*$" "" SQL_STEM "${SQL_FILE}")
  string(REGEX REPLACE "[^A-Za-z0-9]" "_" SQL_IDENT "${SQL_STEM}")

  set(VAR_NAME "${SQL_IDENT}_sql")
  set(GENERATED_HEADER "${CMAKE_CURRENT_BINARY_DIR}/generated/${VAR_NAME}.h")

  add_custom_command(
        OUTPUT "${GENERATED_HEADER}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/generated"
        COMMAND xxd -C -t -i -n "${VAR_NAME}"
        "${CMAKE_CURRENT_SOURCE_DIR}/${SQL_FILE}"
        "${GENERATED_HEADER}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${SQL_FILE}"
        COMMENT "Embedding ${SQL_FILE} into ${GENERATED_HEADER}"
        VERBATIM
    )
  list(APPEND GENERATED_HEADERS "${GENERATED_HEADER}")
endforeach()

add_library(loany STATIC
    loany.c
    db.c
    error.c
)

add_library(Loany::loany ALIAS loany)

target_include_directories(loany PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<INSTALL_INTERFACE:include>
)

target_sources(loany PRIVATE ${GENERATED_HEADERS})
target_include_directories(loany PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")

target_link_libraries(loany PRIVATE $<BUILD_INTERFACE:SQLite3::SQLite3>)
