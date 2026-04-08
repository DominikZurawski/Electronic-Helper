cmake_minimum_required(VERSION 3.20)

project(PomocnikProjektantaElektronika
  VERSION 0.1.0
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(PPE_ENABLE_WARNINGS "Enable warnings" ON)
option(PPE_ENABLE_TESTS "Enable tests" ON)
option(PPE_ENABLE_GUI "Enable Qt GUI" ON)

file(GLOB_RECURSE PPE_CLANG_FORMAT_FILES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.h
)

file(GLOB_RECURSE PPE_CLANG_TIDY_FILES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cxx
)

find_program(PPE_CLANG_FORMAT_EXE NAMES clang-format)
if (PPE_CLANG_FORMAT_EXE)
  add_custom_target(format
    COMMAND ${PPE_CLANG_FORMAT_EXE} -i --style=file ${PPE_CLANG_FORMAT_FILES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Formatting C++ sources with clang-format"
    VERBATIM
    COMMAND_EXPAND_LISTS
  )

  add_custom_target(format-check
    COMMAND ${PPE_CLANG_FORMAT_EXE} --dry-run --Werror --style=file ${PPE_CLANG_FORMAT_FILES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Checking C++ formatting with clang-format"
    VERBATIM
    COMMAND_EXPAND_LISTS
  )
else()
  message(STATUS "clang-format not found. Targets 'format' and 'format-check' will not be available.")
endif()

find_program(PPE_CLANG_TIDY_EXE NAMES clang-tidy)
if (PPE_CLANG_TIDY_EXE)
  add_custom_target(lint
    COMMAND ${PPE_CLANG_TIDY_EXE} --quiet -p ${CMAKE_BINARY_DIR} ${PPE_CLANG_TIDY_FILES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running clang-tidy"
    VERBATIM
    COMMAND_EXPAND_LISTS
  )

  add_custom_target(lint-fix
    COMMAND ${PPE_CLANG_TIDY_EXE} --quiet -fix -format-style=file -p ${CMAKE_BINARY_DIR} ${PPE_CLANG_TIDY_FILES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running clang-tidy with automatic fixes"
    VERBATIM
    COMMAND_EXPAND_LISTS
  )
else()
  message(STATUS "clang-tidy not found. Targets 'lint' and 'lint-fix' will not be available.")
endif()

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
  src/ltspice/ltspice_export_render.cpp
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
      src/modules/project_design/ui/enum_ui.cpp
      src/modules/project_design/ui/canvas_layout.cpp
      src/modules/project_design/ui/canvas_scene.cpp
      src/modules/project_design/ui/connection_ui.cpp
      src/modules/project_design/ui/active_block_view.cpp
      src/modules/project_design/ui/widget_actions.cpp
      src/modules/project_design/ui/widget_bindings.cpp
      src/modules/project_design/ui/export_dialog.cpp
      src/modules/project_design/ui/export_actions.cpp
      src/modules/project_design/ui/form_sync.cpp
      src/modules/project_design/model/model.cpp
      src/modules/project_design/model/connection_ops.cpp
      src/modules/project_design/graph/graph.cpp
      src/modules/project_design/export/export.cpp
      src/modules/project_design/export/export_component_order.cpp
      src/modules/project_design/export/export_render.cpp
      src/modules/project_design/export/export_topology.cpp
      src/modules/project_design/export/export_value_builders.cpp
      src/modules/project_design/export/asc_ops.cpp
      src/modules/project_design/export/template_loader.cpp
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

  add_executable(ppe_ltspice_export_tests tests/test_ltspice_export.cpp)
  target_link_libraries(ppe_ltspice_export_tests PRIVATE ppe_core)
  target_include_directories(ppe_ltspice_export_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
  )
  add_test(NAME ltspice_export COMMAND ppe_ltspice_export_tests)

  add_executable(ppe_psu_basic_model_tests
    tests/test_psu_basic_model.cpp
    src/modules/psu_basic/model/model.cpp
  )
  target_include_directories(ppe_psu_basic_model_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
  )
  add_test(NAME psu_basic_model COMMAND ppe_psu_basic_model_tests)

  if (TARGET Qt6::Widgets)
    add_executable(ppe_project_design_tests
      tests/test_project_design_connection_ops.cpp
      src/modules/project_design/model/model.cpp
      src/modules/project_design/model/connection_ops.cpp
    )
    target_include_directories(ppe_project_design_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
      ${CMAKE_CURRENT_SOURCE_DIR}
    )
    target_link_libraries(ppe_project_design_tests PRIVATE Qt6::Widgets)
    add_test(NAME project_design_connection_ops COMMAND ppe_project_design_tests)

    add_executable(ppe_project_design_export_tests
      tests/test_project_design_export.cpp
      src/modules/project_design/export/export.cpp
      src/modules/project_design/export/export_component_order.cpp
      src/modules/project_design/export/export_render.cpp
      src/modules/project_design/export/export_topology.cpp
      src/modules/project_design/export/export_value_builders.cpp
      src/modules/project_design/export/asc_ops.cpp
      src/modules/project_design/export/template_loader.cpp
      src/modules/project_design/model/model.cpp
      src/modules/psu_basic/export/template_export.cpp
    )
    target_include_directories(ppe_project_design_export_tests PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/include
      ${CMAKE_CURRENT_SOURCE_DIR}
    )
    target_link_libraries(ppe_project_design_export_tests PRIVATE ppe_core Qt6::Widgets)
    add_test(NAME project_design_export COMMAND ppe_project_design_export_tests)
  endif()
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Packaging.cmake)
