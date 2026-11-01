add_executable(test_loany test_loany.c)
target_link_libraries(test_loany PRIVATE
    Loany::loany
    loany_hardening
    loany_sanitizers
  )
add_test(NAME LoanyUnitTests COMMAND test_loany)

add_executable(test_db_schema test_db_schema.c)
target_link_libraries(test_db_schema PRIVATE
    Loany::loany
    SQLite3::SQLite3
    loany_hardening
    loany_sanitizers
  )
add_test(NAME LoanyDatabaseSchemaTests COMMAND test_db_schema)
