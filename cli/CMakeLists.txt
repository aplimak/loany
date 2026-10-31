add_executable(loany_cli main.c)
target_link_libraries(loany_cli PRIVATE Loany::loany)

if(CMAKE_BUILD_TYPE MATCHES "Release|RelWithDebInfo|MinSizeRel" AND ipo_supported)
  set_target_properties(loany_cli PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION TRUE
    )
endif()
