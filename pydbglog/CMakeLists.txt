set(dbglogmodule_SOURCES
  dbglogmodule.cpp
  ${DBGLOG_ROOT}/dbglog.cpp
  ${DBGLOG_ROOT}/mask.cpp
  )

add_library(dbglogmodule MODULE ${dbglogmodule_SOURCES})

# link in some libs
target_link_libraries(dbglogmodule ${Boost_LIBRARIES})
target_link_libraries(dbglogmodule ${PYTHON_LIBRARIES})

# update name to suit python
set_target_properties(dbglogmodule PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/module
  PREFIX "")
