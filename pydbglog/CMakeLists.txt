define_module(LIBRARY pydbglog
  DEPENDS
  dbglog Boost_THREAD Boost_SYSTEM Boost_PYTHON PYTHONLIBS)

if(PYDBGLOG_STATIC)
  set(pydbglog_SOURCES
    dbglogmodule.cpp
    )

  add_library(pydbglog STATIC ${pydbglog_SOURCES})
  buildsys_library(pydbglog)
  target_link_libraries(pydbglog ${MODULE_LIBRARIES})

else()
  # module
  buildsys_position_independent(${MODULE_LIBRARIES})

  set(dbglogmodule_SOURCES
    dbglogmodule.cpp
    )

  add_library(dbglogmodule MODULE ${dbglogmodule_SOURCES})

  target_link_libraries(dbglogmodule ${MODULE_LIBRARIES})

  # update name to suit python
  set_target_properties(dbglogmodule PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/module
    PREFIX "")

  if (PYDBGLOG_INSTALL)
    install(TARGETS dbglogmodule LIBRARY DESTINATION lib/python${PYTHON_VERSION}
      COMPONENT main)
  endif()
endif()
