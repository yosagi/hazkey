cmake_minimum_required(VERSION 3.21)
project(ibus-hazkey VERSION 0.2.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# shared key handling state machine and server connection. added here too
# so that this directory can also be built on its own.
if(NOT TARGET hazkey-frontend-core)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../hazkey-frontend-core
                     ${CMAKE_CURRENT_BINARY_DIR}/hazkey-frontend-core)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(IBUS REQUIRED IMPORTED_TARGET ibus-1.0)

include(GNUInstallDirs)

add_executable(ibus-engine-hazkey
    src/main.cpp
    src/hazkey_engine.cpp
)

target_link_libraries(ibus-engine-hazkey PRIVATE
    hazkey-frontend-core
    PkgConfig::IBUS
)

set(HAZKEY_IBUS_ENGINE_PATH ${CMAKE_INSTALL_FULL_LIBDIR}/hazkey/ibus-engine-hazkey)
set(HAZKEY_SETTINGS_PATH ${CMAKE_INSTALL_FULL_BINDIR}/hazkey-settings)
set(HAZKEY_ICON_PATH ${CMAKE_INSTALL_FULL_DATADIR}/icons/hicolor/scalable/apps/hazkey.svg)
configure_file(hazkey.xml.in hazkey.xml @ONLY)

# the engine registers the same component itself when started by hand
target_compile_definitions(ibus-engine-hazkey PRIVATE
    HAZKEY_IBUS_ENGINE_PATH="${HAZKEY_IBUS_ENGINE_PATH}"
    HAZKEY_SETTINGS_PATH="${HAZKEY_SETTINGS_PATH}"
    HAZKEY_ICON_PATH="${HAZKEY_ICON_PATH}"
    HAZKEY_VERSION="${PROJECT_VERSION}"
)

install(TARGETS ibus-engine-hazkey
    RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/hazkey)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/hazkey.xml
    DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/ibus/component)
