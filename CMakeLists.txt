# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.25)

project(
    beman.range_searcher # CMake Project Name, which is also the name of the top-level
    # targets (e.g., library, executable, etc.).
    DESCRIPTION
        "Range-based searchers and range overloads of searcher-based `std::search`"
    LANGUAGES CXX
    VERSION 0.0.1
)

# [CMAKE.SKIP_TESTS]
option(
    BEMAN_RANGE_SEARCHER_BUILD_TESTS
    "Enable building tests and test infrastructure. Default: ${PROJECT_IS_TOP_LEVEL}. Values: { ON, OFF }."
    ${PROJECT_IS_TOP_LEVEL}
)

# [CMAKE.SKIP_EXAMPLES]
option(
    BEMAN_RANGE_SEARCHER_BUILD_EXAMPLES
    "Enable building examples. Default: ${PROJECT_IS_TOP_LEVEL}. Values: { ON, OFF }."
    ${PROJECT_IS_TOP_LEVEL}
)

option(
    BEMAN_RANGE_SEARCHER_INSTALL_CONFIG_FILE_PACKAGE
    "Enable creating and installing a CMake config-file package. Default: ${PROJECT_IS_TOP_LEVEL}. Values: { ON, OFF }."
    ${PROJECT_IS_TOP_LEVEL}
)

include(GNUInstallDirs)

add_library(beman.range_searcher INTERFACE)
add_library(beman::range_searcher ALIAS beman.range_searcher)

target_sources(
    beman.range_searcher
    PUBLIC
        FILE_SET HEADERS
            BASE_DIRS ${CMAKE_CURRENT_BINARY_DIR}/include include
            FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/beman/range_searcher/searcher.hpp
)

set_target_properties(
    beman.range_searcher
    PROPERTIES VERIFY_INTERFACE_HEADER_SETS ON EXPORT_NAME range_searcher
)

install(
    TARGETS beman.range_searcher
    COMPONENT beman.range_searcher
    EXPORT beman.range_searcher
    DESTINATION ${CMAKE_INSTALL_LIBDIR}$<$<CONFIG:Debug>:/debug>
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}$<$<CONFIG:Debug>:/debug>
    FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if(BEMAN_range_searcher_INSTALL_CONFIG_FILE_PACKAGE)
    include(CMakePackageConfigHelpers)

    configure_package_config_file(
        "${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}-config.cmake.in"
        "${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}/${PROJECT_NAME}-config.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
        PATH_VARS PROJECT_NAME PROJECT_VERSION
    )

    write_basic_package_version_file(
        "${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}/${PROJECT_NAME}-version.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY ExactVersion
    )

    install(
        FILES
            "${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}/${PROJECT_NAME}-config.cmake"
            "${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}/${PROJECT_NAME}-version.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
        COMPONENT development
    )

    install(
        EXPORT beman.range_searcher
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
        NAMESPACE beman::
        FILE ${PROJECT_NAME}-targets.cmake
        COMPONENT development
    )
endif()

include(CTest)

if(BEMAN_RANGE_SEARCHER_BUILD_TESTS)
    add_subdirectory(tests/beman/range_searcher)
endif()

if(BEMAN_RANGE_SEARCHER_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
