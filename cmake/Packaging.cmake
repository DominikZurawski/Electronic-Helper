include(GNUInstallDirs)

set(CPACK_PACKAGE_NAME "PomocnikProjektantaElektronika")
set(CPACK_PACKAGE_VENDOR "Pomocnik Projektanta Elektronika")
set(CPACK_PACKAGE_CONTACT "maintainers@local")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

# Default generators (cross-platform safe)
set(CPACK_GENERATOR "ZIP;TGZ")

if (UNIX AND NOT APPLE)
  list(APPEND CPACK_GENERATOR "RPM;DEB")

  # Runtime dependencies for packaged GUI builds.
  set(CPACK_RPM_PACKAGE_REQUIRES
    "qt6-qtbase-gui, qt6-qtwebengine"
  )
  set(CPACK_DEBIAN_PACKAGE_DEPENDS
    "libqt6widgets6, libqt6webenginewidgets6"
  )
endif()

if (WIN32)
  list(APPEND CPACK_GENERATOR "NSIS")
  set(CPACK_NSIS_DISPLAY_NAME "Pomocnik Projektanta Elektronika")
  set(CPACK_NSIS_PACKAGE_NAME "Pomocnik Projektanta Elektronika")
  if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.ico")
    set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.ico")
    set(CPACK_NSIS_MUI_UNIICON "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.ico")
  endif()
  set(CPACK_NSIS_CREATE_ICONS_EXTRA
    "CreateShortCut '$DESKTOP\\\\Pomocnik Projektanta Elektronika.lnk' '$INSTDIR\\\\bin\\\\ppe_gui.exe'"
  )
  set(CPACK_NSIS_DELETE_ICONS_EXTRA
    "Delete '$DESKTOP\\\\Pomocnik Projektanta Elektronika.lnk'"
  )
endif()

if (APPLE)
  list(APPEND CPACK_GENERATOR "DragNDrop")
endif()

include(CPack)
