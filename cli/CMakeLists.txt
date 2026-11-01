add_executable(loany_cli main.c)
target_link_libraries(loany_cli PRIVATE
    Loany::loany
    loany_hardening
    loany_sanitizers
  )
loany_set_warnings(loany_cli)

if(ipo_supported)
  set_target_properties(loany_cli PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION $<NOT:$<CONFIG:Debug>>)
endif()
