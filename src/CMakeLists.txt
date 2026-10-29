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

target_link_libraries(loany PRIVATE sqlite3)
