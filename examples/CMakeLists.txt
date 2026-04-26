# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set(ALL_EXAMPLES basic_example complex_example)

message("Examples to be built: ${ALL_EXAMPLES}")

foreach(example ${ALL_EXAMPLES})
    add_executable(beman.range_searcher.examples.${example})
    target_sources(beman.range_searcher.examples.${example} PRIVATE ${example}.cpp)
    target_link_libraries(
        beman.range_searcher.examples.${example}
        PRIVATE beman::range_searcher
    )
endforeach()
