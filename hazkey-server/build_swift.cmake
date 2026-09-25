set(SWIFT_COMMAND
    "${SWIFT_EXECUTABLE}"
    "build" "-c" "${SWIFT_BUILD_TYPE}"
    "--scratch-path=${CMAKE_CURRENT_BINARY_DIR}/swift-build"
)

if(HAZKEY_SERVER_SWIFT_SDK)
    list(APPEND SWIFT_COMMAND "--swift-sdk" "${HAZKEY_SERVER_SWIFT_SDK}")
endif()

if(HAZKEY_SERVER_ZENZAI_TRAIT)
    list(APPEND SWIFT_COMMAND "--traits" "ZenzaiSupport")
    list(APPEND SWIFT_COMMAND "-Xlinker" "-L${LIBLLAMA_DIR}")
endif()

if(HAZKEY_SERVER_SWIFT_LTO_MODE)
    list(APPEND SWIFT_COMMAND "--experimental-lto-mode" "${HAZKEY_SERVER_SWIFT_LTO_MODE}")
endif()

# SwiftPM 6.4 switched the default build system to "swiftbuild", which
# partially links C targets with `clang -r`. With LTO enabled those objects are
# LLVM bitcode and the partial link fails ("file format not recognized", seen
# with swift-numerics' _NumericsShims). Stay on the native build system, which
# older toolchains (>= 6.0) also accept as their default.
list(APPEND SWIFT_COMMAND "--build-system" "native")

if(SWIFT_STATIC_STDLIB)
    list(APPEND SWIFT_COMMAND "-Xswiftc" "-static-stdlib")

    if(SWIFT_DYNAMIC_LIB_PATH)
        list(APPEND SWIFT_COMMAND "-Xlinker" "-L${SWIFT_DYNAMIC_LIB_PATH}")
    endif()
endif()

if(SWIFT_DISABLE_DEPENDENCY_CACHE)
    list(APPEND SWIFT_COMMAND "--disable-dependency-cache")
endif()

if(SWIFT_LINK_PATH)
    list(APPEND SWIFT_COMMAND "-Xlinker" "-L${SWIFT_LINK_PATH}")
endif()

# AzooKeyKanaKanjiConverter (pinned) enables the MemberImportVisibility
# upcoming feature in its Package.swift, but DictionaryBuilder.swift lacks an
# explicit `import OrderedCollections`, breaking clean builds on any Swift
# toolchain that honors the feature (>= 6.1, the project minimum). Disable the
# feature until the pin is updated.
list(APPEND SWIFT_COMMAND
    "-Xswiftc" "-disable-upcoming-feature"
    "-Xswiftc" "MemberImportVisibility")

execute_process(
    COMMAND ${SWIFT_COMMAND}
    WORKING_DIRECTORY "${SWIFT_WORK_DIR}"
    RESULT_VARIABLE result
)

if(NOT result EQUAL 0)
    message(FATAL_ERROR "Swift build failed with error: ${result}")
endif()
