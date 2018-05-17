# bump version here
set(pydbglog_VERSION 1.0)

define_module(LIBRARY pydbglog=${pydbglog_VERSION}
  DEPENDS
  dbglog Boost_THREAD Boost_SYSTEM Boost_PYTHON PYTHONLIBS)

if(PYDBGLOG_STATIC)
  set(pydbglog_SOURCES
    dbglogmodule.cpp
    )

  add_library(pydbglog STATIC ${pydbglog_SOURCES})
  buildsys_library(pydbglog)
  target_link_libraries(pydbglog ${MODULE_LIBRARIES})
  target_compile_definitions(pydbglog PRIVATE PYDBGLOG_STATIC=1)

else()
  # module
  buildsys_position_independent(${MODULE_LIBRARIES})

  set(dbglogmodule_SOURCES
    dbglogmodule.cpp
    )

  add_library(dbglogmodule MODULE ${dbglogmodule_SOURCES})

  target_link_libraries(dbglogmodule ${MODULE_LIBRARIES})
  target_compile_definitions(dbglogmodule PRIVATE PYDBGLOG_STATIC=0)

  # update name to suit python
  set_target_properties(dbglogmodule PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/module
    PREFIX "")

  if(NOT PYTHONLIBS_VERSION_STRING VERSION_LESS 3.0.0)
    set_target_properties(dbglogmodule PROPERTIES
      LIBRARY_OUTPUT_NAME dbglog)
  endif()

  if (PYDBGLOG_INSTALL)
    install(TARGETS dbglogmodule LIBRARY DESTINATION ${PYTHON_MODULE_INSTALL_PATH}
      COMPONENT main)
  endif()
endif()
