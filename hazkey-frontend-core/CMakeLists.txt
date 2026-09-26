cmake_minimum_required(VERSION 3.21)

# Key handling state machine shared by the frontends (fcitx5, emacs).
# Also generates the protobuf code of the server protocol for them.
project(hazkey-frontend-core LANGUAGES CXX)

find_package(Protobuf REQUIRED)

set(PROTO_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/../protocol/base.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/../protocol/commands.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/../protocol/config.proto
)

add_library(hazkey-frontend-core STATIC
    src/utf8.cpp
)

set_target_properties(hazkey-frontend-core PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    # linked into the fcitx5 addon (shared library)
    POSITION_INDEPENDENT_CODE ON
)

if(Protobuf_VERSION VERSION_GREATER_EQUAL "3.15")
    # 3.15 ~：stable proto3 optional support
    message(STATUS "hazkey-frontend-core: Using standard protobuf_generate (protobuf ${Protobuf_VERSION})")
    protobuf_generate(
        TARGET hazkey-frontend-core
        LANGUAGE cpp
        PROTOS ${PROTO_FILES}
        IMPORT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../protocol
        PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
    )
elseif(Protobuf_VERSION VERSION_GREATER_EQUAL "3.12")
    # 3.12 ~ 3.14: build with flag. cannot use PROTOC_OPTIONS.
    # Ubuntu 22.04: v3.12.4 (2025/8/28)
    message(STATUS "hazkey-frontend-core: Using manual protoc execution for proto3 optional (protobuf ${Protobuf_VERSION})")

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

    target_sources(hazkey-frontend-core PRIVATE ${PROTO_SRCS} ${PROTO_HDRS})
else()
    # ~ 3.12：no proto3 optional support
    message(FATAL_ERROR "protobuf 3.12+ required for proto3 optional support. Current version: ${Protobuf_VERSION}")
endif()

target_include_directories(hazkey-frontend-core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Protobuf_INCLUDE_DIRS}
)

target_link_libraries(hazkey-frontend-core PUBLIC ${Protobuf_LITE_LIBRARIES})
