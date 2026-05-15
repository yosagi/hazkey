cmake_minimum_required(VERSION 3.21)
project(emacs-hazkey VERSION 0.2.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Protobuf REQUIRED)

set(PROTO_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/../protocol/base.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/../protocol/commands.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/../protocol/config.proto
)

set(PROJECT_SOURCES
    src/main.cpp
    src/hazkey_emacs_state.cpp
    src/hazkey_emacs_connector.cpp
    src/mozc_output_builder.cpp
    src/key_translator.cpp
    mozc_lib/mozc_emacs_helper_lib.cc
)

add_executable(hazkey_emacs_helper ${PROJECT_SOURCES})

if(Protobuf_VERSION VERSION_GREATER_EQUAL "3.15")
    message(STATUS "emacs-hazkey: Using standard protobuf_generate (protobuf ${Protobuf_VERSION})")
    protobuf_generate(
        TARGET hazkey_emacs_helper
        LANGUAGE cpp
        PROTOS ${PROTO_FILES}
        IMPORT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../protocol
        PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
    )
elseif(Protobuf_VERSION VERSION_GREATER_EQUAL "3.12")
    message(STATUS "emacs-hazkey: Using manual protoc execution (protobuf ${Protobuf_VERSION})")

    set(PROTO_SRCS)
    set(PROTO_HDRS)

    foreach(PROTO_FILE ${PROTO_FILES})
        get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
        set(PROTO_SRC "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb.cc")
        set(PROTO_HDR "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb.h")

        list(APPEND PROTO_SRCS ${PROTO_SRC})
        list(APPEND PROTO_HDRS ${PROTO_HDR})

        add_custom_command(
            OUTPUT ${PROTO_SRC} ${PROTO_HDR}
            COMMAND ${Protobuf_PROTOC_EXECUTABLE}
            ARGS --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
                 --experimental_allow_proto3_optional
                 --proto_path=${CMAKE_CURRENT_SOURCE_DIR}/../protocol
                 ${CMAKE_CURRENT_SOURCE_DIR}/../protocol/${PROTO_NAME}.proto
            DEPENDS ${PROTO_FILE}
            COMMENT "Generating C++ protobuf files from ${PROTO_FILE}"
            VERBATIM
        )
    endforeach()

    target_sources(hazkey_emacs_helper PRIVATE ${PROTO_SRCS} ${PROTO_HDRS})
else()
    message(FATAL_ERROR "protobuf 3.12+ required. Current version: ${Protobuf_VERSION}")
endif()

target_include_directories(hazkey_emacs_helper PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/mozc_lib/shims
    ${CMAKE_CURRENT_SOURCE_DIR}/mozc_lib
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Protobuf_INCLUDE_DIRS}
)

target_link_libraries(hazkey_emacs_helper PRIVATE
    ${Protobuf_LITE_LIBRARIES}
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
