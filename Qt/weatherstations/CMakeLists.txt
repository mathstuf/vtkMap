include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}
)

set(UI_FILES qtMapCoordinatesWidget.ui qtWeatherStations.ui)
set(QT_WRAP qtMapCoordinatesWidget.h qtWeatherStations.h)
QT4_WRAP_UI(UISrcs ${UI_FILES})
QT4_WRAP_CPP(MOCSrcs ${QT_WRAP})

add_executable(wsmap
  wsmap.cxx
  qtWeatherStations.cxx
  qtMapCoordinatesWidget.cxx
  ${UISrcs} ${MOCSrcs}
)
target_link_libraries(wsmap vtkMap ${CURL_LIBRARIES})

