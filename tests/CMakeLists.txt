add_executable(test_loany test_loany.c)
target_link_libraries(test_loany PRIVATE Loany::loany)
add_test(NAME LoanyUnitTests COMMAND test_loany)

add_executable(test_db_schema test_db_schema.c)
target_link_libraries(test_db_schema PRIVATE Loany::loany)
add_test(NAME LoanyDatabaseSchemaTests COMMAND test_db_schema)
