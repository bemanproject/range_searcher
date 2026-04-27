# beman.range_searcher: Range-based searchers and range overloads of searcher-based `std::search`

<!--
SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
-->

<!-- markdownlint-disable-next-line line-length -->
![Library Status](https://raw.githubusercontent.com/bemanproject/beman/refs/heads/main/images/badges/beman_badge-beman_library_under_development.svg) ![Continuous Integration Tests](https://github.com/bemanproject/range_searcher/actions/workflows/ci_tests.yml/badge.svg) ![Lint Check (pre-commit)](https://github.com/bemanproject/range_searcher/actions/workflows/pre-commit-check.yml/badge.svg) [![Coverage](https://coveralls.io/repos/github/bemanproject/range_searcher/badge.svg?branch=main)](https://coveralls.io/github/bemanproject/range_searcher?branch=main) ![Standard Target](https://github.com/bemanproject/beman/blob/main/images/badges/cpp29.svg)

**Implements**: Range-based searchers and range overloads of `std::search` with search arguments,
proposed in [Range-Based Searchers (P4205R0)](https://wg21.link/P4205R0).

**Status**: [Under development and not yet ready for production use.](https://github.com/bemanproject/beman/blob/main/docs/beman_library_maturity_model.md#under-development-and-not-yet-ready-for-production-use)

## License

`beman.range_searcher` is licensed under the Apache License v2.0 with LLVM Exceptions.

Note: this project includes code from [the LLVM project](https://github.com/llvm/llvm-project).

## Usage

Implements the following proposed addition to the standard library:

- Searcher-based overloads for `std::ranges::search` and `std::ranges::contains_subrange`
- Concept `std::searchable` that captures the standard searcher requirements
- `std::ranges::default_searcher`, `std::ranges::boyer_moore_searcher`, `std::ranges::boyer_moore_horspool_searcher`
  - Range-ified versions of the standard searchers, with API revamped to accommodate C++20 range and iterator-sentinel model.

```cpp
#include <functional>
#include <print>
#include <vector>

#include <beman/range_searcher/searcher.hpp>

namespace exe = beman::range_searcher;
namespace branges = exe::ranges;

// Example given in the paper for range-based searchers. (Needs C++23)
int main() {
    std::string haystack = "a quick brown fox jumps over the lazy dog";
    std::string needle = "jump";

    branges::boyer_moore_searcher searcher(needle);
    auto                          result = branges::search(haystack, searcher);
    print(std::ranges::subrange{haystack.begin(), result.begin()});
    print("[");
    print(result);
    print("]");
    println(std::ranges::subrange{result.end(), haystack.end()});
    // Output: a quick brown fox [jump]s over the lazy dog

    if (branges::contains_subrange(haystack, branges::boyer_moore_horspool_searcher{needle}))
        println("jump found!");
    else println("jump not found!");
    if (branges::contains_subrange(haystack, branges::boyer_moore_horspool_searcher{"run"}))
        println("run found!");
    else println("run not found!");

    return 0;
}
```

Full runnable examples can be found in [`examples/`](examples/).

## Dependencies

### Build Environment

This project requires at least the following to build:

* A C++ compiler that conforms to the C++20 standard or greater
* CMake 3.30 or later
* (Test Only) GoogleTest

You can disable building tests by setting CMake option `BEMAN_RANGE_SEARCHER_BUILD_TESTS` to
`OFF` when configuring the project.

### Supported Platforms

| Compiler   | Version | C++ Standards | Standard Library  |
|------------|---------|---------------|-------------------|
| GCC        | 15-13   | C++26-C++20   | libstdc++         |
| GCC        | 12      | C++23, C++20  | libstdc++         |
| Clang      | 20-19   | C++26-C++20   | libstdc++, libc++ |
| Clang      | 18-17   | C++26-C++20   | libc++            |
| Clang      | 18-17   | C++20         | libstdc++         |
| AppleClang | latest  | C++26-C++20   | libc++            |
| MSVC       | latest  | C++23         | MSVC STL          |

## Development

See the [Contributing Guidelines](CONTRIBUTING.md).

## Integrate beman.range_searcher into your project

### Build

You can build range_searcher using a CMake workflow preset:

```bash
cmake --workflow --preset gcc-release
```

To list available workflow presets, you can invoke:

```bash
cmake --list-presets=workflow
```

For details on building beman.range_searcher without using a CMake preset, refer to the
[Contributing Guidelines](CONTRIBUTING.md).

### Installation

To install beman.range_searcher globally after building with the `gcc-release` preset, you can
run:

```bash
sudo cmake --install build/gcc-release
```

Alternatively, to install to a prefix, for example `/opt/beman`, you can run:

```bash
sudo cmake --install build/gcc-release --prefix /opt/beman
```

This will generate the following directory structure:

```txt
/opt/beman
├── include
│   └── beman
│       └── range_searcher
│           ├── searcher.hpp
│           └── ...
└── lib
    └── cmake
        └── beman.range_searcher
            ├── beman.range_searcher-config-version.cmake
            ├── beman.range_searcher-config.cmake
            └── beman.range_searcher-targets.cmake
```

### CMake Configuration

If you installed beman.range_searcher to a prefix, you can specify that prefix to your CMake
project using `CMAKE_PREFIX_PATH`; for example, `-DCMAKE_PREFIX_PATH=/opt/beman`.

You need to bring in the `beman.range_searcher` package to define the `beman::range_searcher` CMake
target:

```cmake
find_package(beman.range_searcher REQUIRED)
```

You will then need to add `beman::range_searcher` to the link libraries of any libraries or
executables that include `beman.range_searcher` headers.

```cmake
target_link_libraries(yourlib PUBLIC beman::range_searcher)
```

### Using beman.range_searcher

To use `beman.range_searcher` in your C++ project,
include an appropriate `beman.range_searcher` header from your source code.

```c++
#include <beman/range_searcher/searcher.hpp>
```

> [!NOTE]
>
> `beman.range_searcher` headers are to be included with the `beman/range_searcher/` prefix.
> Altering include search paths to spell the include target another way (e.g.
> `#include <searcher.hpp>`) is unsupported.
