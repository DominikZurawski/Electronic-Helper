cmake_minimum_required(VERSION 3.20)

project(PomocnikProjektantaElektronika
  VERSION 0.1.0
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PPE_ENABLE_WARNINGS "Enable warnings" ON)
option(PPE_ENABLE_TESTS "Enable tests" ON)
option(PPE_ENABLE_GUI "Enable Qt GUI" ON)

if (PPE_ENABLE_WARNINGS)
  if (MSVC)
    add_compile_options(/W4 /permissive-)
  else()
    add_compile_options(-Wall -Wextra -Wpedantic)
  endif()
endif()

add_library(ppe_core
  src/core/app.cpp
  src/core/module_registry.cpp
  src/ltspice/ltspice_import.cpp
  src/ltspice/ltspice_export.cpp
  src/ltspice/ltspice_template.cpp
)

target_include_directories(ppe_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(ppe
  src/main.cpp
  src/cli/command_router.cpp
  src/modules/psu_basic/module.cpp
  src/modules/psu_basic/model/model.cpp
  src/modules/psu_symmetric/module.cpp
  src/modules/psu_symmetric/model/model.cpp
  src/modules/antenna_basic/module.cpp
  src/modules/antenna_basic/model/model.cpp
)

target_link_libraries(ppe PRIVATE ppe_core)

if (PPE_ENABLE_GUI)
  find_package(Qt6 COMPONENTS Widgets QUIET)
  if (Qt6_FOUND)
    set(CMAKE_AUTORCC ON)
    add_executable(ppe_gui
      src/gui/main.cpp
      src/modules/project_design/ui/widget.cpp
      src/modules/project_design/model/model.cpp
      src/modules/project_design/graph/graph.cpp
      src/modules/project_design/export/export.cpp
      src/modules/psu_basic/ui/waveform_widget.cpp
      src/modules/psu_basic/model/model.cpp
      src/modules/psu_basic/export/export.cpp
      src/modules/psu_basic/export/template_export.cpp
      src/modules/psu_symmetric/model/model.cpp
      src/modules/antenna_basic/ui/widget.cpp
      src/modules/antenna_basic/model/model.cpp
      assets/ppe.qrc
    )
    target_link_libraries(ppe_gui PRIVATE ppe_core Qt6::Widgets)
    target_include_directories(ppe_gui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    set_target_properties(ppe_gui PROPERTIES OUTPUT_NAME "ppe_gui")
    if (APPLE)
      set(PPE_ICNS ${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.icns)
      if (EXISTS ${PPE_ICNS})
        set_source_files_properties(${PPE_ICNS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
        target_sources(ppe_gui PRIVATE ${PPE_ICNS})
        set_target_properties(ppe_gui PROPERTIES MACOSX_BUNDLE TRUE MACOSX_BUNDLE_ICON_FILE "icon.icns")
      else()
        message(STATUS "macOS icon not found. Run: scripts/generate_icons_macos.sh")
      endif()
    endif()
  else()
    message(STATUS "Qt6 not found. GUI target 'ppe_gui' will not be built.")
    message(STATUS "Install on Fedora: sudo dnf install qt6-qtbase-devel qt6-qtbase-gui")
    message(STATUS "Install on Ubuntu/Debian: sudo apt install qt6-base-dev")
    message(STATUS "Install on macOS: brew install qt")
  endif()
endif()

install(TARGETS ppe RUNTIME DESTINATION bin)
if (TARGET ppe_gui)
  install(TARGETS ppe_gui RUNTIME DESTINATION bin BUNDLE DESTINATION bin)
endif()

if (UNIX AND NOT APPLE AND TARGET ppe_gui)
  set(PPE_DESKTOP_EXEC "ppe_gui")
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/assets/ppe.desktop.in
    ${CMAKE_CURRENT_BINARY_DIR}/ppe.desktop
    @ONLY
  )
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ppe.desktop DESTINATION share/applications)
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.svg DESTINATION share/icons/hicolor/scalable/apps RENAME ppe.svg)
endif()

if (PPE_ENABLE_TESTS)
  enable_testing()
  add_executable(ppe_tests tests/test_registry.cpp)
  target_link_libraries(ppe_tests PRIVATE ppe_core)
  add_test(NAME registry COMMAND ppe_tests)
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Packaging.cmake)
