add_executable(test_loany test_loany.c)
target_link_libraries(test_loany PRIVATE Loany::loany)
add_test(NAME LoanyUnitTests COMMAND test_loany)
