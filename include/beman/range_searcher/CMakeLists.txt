# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

if(BEMAN_RANGE_SEARCHER_USE_MODULES)
    target_sources(
        beman.range_searcher
        PUBLIC
            FILE_SET CXX_MODULES FILES range_searcher.cppm
            FILE_SET HEADERS
                FILES
                    config.hpp
                    searcher.hpp
                    detail/expo_only.hpp
                    "${PROJECT_BINARY_DIR}/include/beman/range_searcher/config_generated.hpp"
    )
else()
    target_sources(
        beman.range_searcher
        PUBLIC
            FILE_SET HEADERS
                FILES
                    config.hpp
                    searcher.hpp
                    detail/expo_only.hpp
                    "${PROJECT_BINARY_DIR}/include/beman/range_searcher/config_generated.hpp"
    )
endif()
