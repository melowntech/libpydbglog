if(PYDBGLOG_STATIC)
  # TODO: add Boost_PYTHON
  define_module(LIBRARY pydbglog DEPENDS Boost_THREAD Boost_SYSTEM
    Boost_PYTHON PYTHONLIBS)

  set(pydbglog_SOURCES
    dbglogmodule.cpp
    )

  add_library(pydbglog STATIC ${pydbglog_SOURCES})
  buildsys_library(pydbglog)
  target_link_libraries(pydbglog ${MODULE_LIBRARIES})
else()
  set(dbglogmodule_SOURCES
    dbglogmodule.cpp
    ${DBGLOG_ROOT}/dbglog.cpp
    ${DBGLOG_ROOT}/mask.cpp
    )

  add_library(dbglogmodule MODULE ${dbglogmodule_SOURCES})

  target_link_libraries(dbglogmodule ${Boost_LIBRARIES} ${PYTHONLIBS_LIBRARIES})

  # update name to suit python
  set_target_properties(dbglogmodule PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/module
    PREFIX "")
endif()
