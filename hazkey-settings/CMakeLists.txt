cmake_minimum_required(VERSION 3.21)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR
        "In-source builds are not allowed. "
        "Please create a separate build directory and run cmake from there.\n"
        "To clean up, remove CMakeCache.txt and CMakeFiles/ directory.")
endif()

project(hazkey-settings VERSION 0.2.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets LinguistTools Network)

set(TS_FILES hazkey-settings_ja_JP.ts)

configure_file(constants.h.in constants.h @ONLY)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    controllers/tab_context.h
    controllers/warning_widget_factory.cpp
    controllers/warning_widget_factory.h
    controllers/user_interface_tab_controller.cpp
    controllers/user_interface_tab_controller.h
    controllers/conversion_tab_controller.cpp
    controllers/conversion_tab_controller.h
    controllers/input_style_tab_controller.cpp
    controllers/input_style_tab_controller.h
    controllers/dictionary_tab_controller.cpp
    controllers/dictionary_tab_controller.h
    controllers/ai_tab_controller.cpp
    controllers/ai_tab_controller.h
    controllers/about_tab_controller.cpp
    controllers/about_tab_controller.h
    controllers/dual_list_controller.cpp
    controllers/dual_list_controller.h
    serverconnector.cpp
    serverconnector.h
    config_definitions.h
    config_macros.h
    ${TS_FILES}
)

qt_add_executable(hazkey-settings ${PROJECT_SOURCES}
    hazkey-icon.qrc)
qt_create_translation(QM_FILES ${PROJECT_SOURCES} ${TS_FILES})

foreach(_qm ${QM_FILES})
    get_filename_component(_qm_name ${_qm} NAME)
    set_source_files_properties(${_qm} PROPERTIES QT_RESOURCE_ALIAS ${_qm_name})
endforeach()

qt_add_resources(hazkey-settings "translations"
    PREFIX "/i18n"
    FILES ${QM_FILES}
)

# hazkey-server connection (and the protobuf code of the server protocol)
# shared with the other clients.
if(NOT TARGET hazkey-frontend-core)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../hazkey-frontend-core
                     ${CMAKE_CURRENT_BINARY_DIR}/hazkey-frontend-core)
endif()

target_include_directories(hazkey-settings PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(hazkey-settings PRIVATE
    Qt6::Widgets
    Qt6::Network
    hazkey-frontend-core
)

include(GNUInstallDirs)
install(TARGETS hazkey-settings
    RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/hazkey
)

# install desktop entry
if(HAZKEY_FLATPAK)
    set(HAZKEY_ICON_NAME "org.hazkey.Fcitx5.Addon.Hazkey")
    set(HAZKEY_SETTINGS_DESKTOP_FILE_NAME org.hazkey.Fcitx5.Addon.Hazkey.hazkey-settings)
else()
    set(HAZKEY_ICON_NAME "hazkey")
    set(HAZKEY_SETTINGS_DESKTOP_FILE_NAME hazkey-settings)
endif()

configure_file(hazkey-settings.desktop.in ${HAZKEY_SETTINGS_DESKTOP_FILE_NAME}.desktop @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${HAZKEY_SETTINGS_DESKTOP_FILE_NAME}.desktop"
    DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/applications)

# create a symlink in the binary directory
install(CODE "
    file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_BINDIR}\")
    file(CREATE_LINK
    \"${CMAKE_INSTALL_FULL_LIBDIR}/hazkey/hazkey-settings\"
    \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_BINDIR}/hazkey-settings\"
    SYMBOLIC)
")
