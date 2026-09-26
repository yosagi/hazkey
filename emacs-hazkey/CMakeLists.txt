cmake_minimum_required(VERSION 3.21)
project(emacs-hazkey VERSION 0.2.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# shared key handling state machine. added here too so that this directory
# can also be built on its own.
if(NOT TARGET hazkey-frontend-core)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../hazkey-frontend-core
                     ${CMAKE_CURRENT_BINARY_DIR}/hazkey-frontend-core)
endif()

set(PROJECT_SOURCES
    src/main.cpp
    src/hazkey_emacs_connector.cpp
    src/mozc_output_builder.cpp
    src/key_translator.cpp
    mozc_lib/mozc_emacs_helper_lib.cc
)

add_executable(hazkey_emacs_helper ${PROJECT_SOURCES})

target_include_directories(hazkey_emacs_helper PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/mozc_lib/shims
    ${CMAKE_CURRENT_SOURCE_DIR}/mozc_lib
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(hazkey_emacs_helper PRIVATE
    hazkey-frontend-core
)

include(GNUInstallDirs)
install(TARGETS hazkey_emacs_helper RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/hazkey)
install(CODE "
    file(MAKE_DIRECTORY \$ENV{DESTDIR}${CMAKE_INSTALL_FULL_BINDIR})
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${CMAKE_INSTALL_FULL_LIBDIR}/hazkey/hazkey_emacs_helper
            \$ENV{DESTDIR}${CMAKE_INSTALL_FULL_BINDIR}/hazkey_emacs_helper
    )
")
