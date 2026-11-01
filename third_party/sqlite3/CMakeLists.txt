find_package(Threads REQUIRED)

add_library(sqlite3 STATIC sqlite3.c)

target_include_directories(sqlite3
    SYSTEM PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(sqlite3
    PUBLIC
        Threads::Threads
        ${CMAKE_DL_LIBS}
)
if(NOT WIN32)
    target_link_libraries(sqlite3 PRIVATE m)
endif()

add_library(SQLite3::SQLite3 ALIAS sqlite3)

target_compile_definitions(sqlite3 PRIVATE
    SQLITE_THREADSAFE=1
    SQLITE_ENABLE_FTS5
    SQLITE_ENABLE_RTREE
    SQLITE_ENABLE_JSON1
    SQLITE_ENABLE_STAT4
    SQLITE_DQS=0                       # forbid double-quoted string literals
    SQLITE_DEFAULT_MEMSTATUS=0         # removes an atomic counter per malloc
    SQLITE_OMIT_DEPRECATED
    SQLITE_OMIT_SHARED_CACHE
    SQLITE_OMIT_LOAD_EXTENSION
    SQLITE_DEFAULT_CACHE_SIZE=-16000
    SQLITE_DEFAULT_PAGE_SIZE=4096
    SQLITE_MAX_EXPR_DEPTH=10000
)
