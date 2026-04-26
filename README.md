# beman.range_searcher: Range-based searchers and range overloads of searcher-based `std::search`

<!--
SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
-->

![Library Status](https://raw.githubusercontent.com/bemanproject/beman/refs/heads/main/images/badges/beman_badge-beman_library_under_development.svg)
![Continuous Integration Tests](https://github.com/bemanproject/range_searcher/actions/workflows/ci_tests.yml/badge.svg)
![Lint Check (pre-commit)](https://github.com/bemanproject/range_searcher/actions/workflows/pre-commit.yml/badge.svg)
[![Coverage](https://coveralls.io/repos/github/bemanproject/range_searcher/badge.svg?branch=main)](https://coveralls.io/github/bemanproject/range_searcher?branch=main)
![Standard Target](https://github.com/bemanproject/beman/blob/main/images/badges/cpp29.svg)

**Implements**: Range-based searchers and range overloads of `std::search` with search arguments,
proposed in [Range-Based Searchers (P4205R0)](https://wg21.link/P4205R0).

**Status**: [Under development and not yet ready for production use.](https://github.com/bemanproject/beman/blob/main/docs/beman_library_maturity_model.md#under-development-and-not-yet-ready-for-production-use)

## License

beman.range_searcher is licensed under the Apache License v2.0 with LLVM Exceptions.

Note: this project includes code from the LLVM project.

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

- C++20
- CMake 3.25
- (Test Only) GoogleTest

You can disable building tests by setting cmake option
[`BEMAN_RANGE_SEARCHER_BUILD_TESTS`](#beman_range_searcher_build_tests) to `OFF`
when configuring the project.

> [!TIP]
>
> In the logs you will be able to see if there are any examples that aren't enabled
> due to compiler capabilities or the configured C++ version.
>
> Below is an example:
>
> ```txt
> -- Looking for __cpp_lib_ranges
> -- Looking for __cpp_lib_ranges - not found
> CMake Warning at examples/CMakeLists.txt:12 (message):
>   Missing range support! Skip: identity_as_default_projection
>
>
> Examples to be built: identity_direct_usage
> ```

### Supported Platforms

| Compiler   | Version | C++ Standards | Standard Library  |
|------------|---------|---------------|-------------------|
| GCC        | 15-13   | C++26-C++20   | libstdc++         |
| GCC        | 12-11   | C++23, C++20  | libstdc++         |
| Clang      | 20-19   | C++26-C++20   | libstdc++, libc++ |
| Clang      | 18-17   | C++26-C++20   | libc++            |
| Clang      | 18-17   | C++20         | libstdc++         |
| AppleClang | latest  | C++26-C++20   | libc++            |
| MSVC       | latest  | C++23         | MSVC STL          |

## Development

### Develop using GitHub Codespace

This project supports [GitHub Codespace](https://github.com/features/codespaces)
via [Development Containers](https://containers.dev/),
which allows rapid development and instant hacking in your browser.
We recommend you using GitHub codespace to explore this project as this
requires minimal setup.

You can create a codespace for this project by clicking this badge:

[![Open in GitHub Codespaces](https://github.com/codespaces/badge.svg)](https://codespaces.new/bemanproject/range_searcher)

For more detailed documentation regarding creating and developing inside of
GitHub codespaces, please reference [this doc](https://docs.github.com/en/codespaces/).

> [!NOTE]
>
> The codespace container may take up to 5 minutes to build and spin-up,
> this is normal as we need to build a custom docker container to setup
> an environment appropriate for beman projects.

### Develop locally on your machines

<details>
<summary> For Linux based systems </summary>

Beman libraries require [recent versions of CMake](#build-environment),
we advise you to download CMake directly from [CMake's website](https://cmake.org/download/)
or install it via the [Kitware apt library](https://apt.kitware.com/).

A [supported compiler](#supported-platforms) should be available from your package manager.
Alternatively you could use an install script from official compiler vendors.

Here is an example of how to install the latest stable version of clang
as per [the official LLVM install guide](https://apt.llvm.org/).

```bash
bash -c "$(wget -O - https://apt.llvm.org/llvm.sh)"
```

If the included test suite is being built and run, a GoogleTest library will be
required. Here is an example of installing GoogleTest on a Debian-based Linux
environment:

```bash
apt install libgtest-dev
```

The precise command and package name will vary depending on the Linux OS you are
using. Be sure to consult documentation and the package repository for the system
you are using.

</details>

<details>
<summary> For MacOS based systems </summary>

Beman libraries require [recent versions of CMake](#build-environment).
You can use [`Homebrew`](https://brew.sh/) to install the latest major version of CMake.

```bash
brew install cmake
```

A [supported compiler](#supported-platforms) is also available from brew.

For example, you can install the latest major release of Clang as:

```bash
brew install llvm
```

</details>

<details>
<summary> For Windows </summary>

To build Beman libraries, you will need the MSVC compiler. MSVC can be obtained
by installing Visual Studio; the free Visual Studio 2022 Community Edition can
be downloaded from
[Microsoft](https://visualstudio.microsoft.com/vs/community/).

After Visual Studio has been installed, you can launch "Developer PowerShell for
VS 2022" by typing it into Windows search bar. This shell environment will
provide CMake, Ninja, and MSVC, allowing you to build the library and run the
tests.

Note that you will need to use FetchContent to build GoogleTest. To do so,
please see the instructions in the "Build GoogleTest dependency from github.com"
dropdown in the [Project specific configure
arguments](#project-specific-configure-arguments) section.

</details>

### Configure and Build the Project Using CMake Presets

This project recommends using [CMake Presets](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html)
to configure, build and test the project.
Appropriate presets for major compilers have been included by default.
You can use `cmake --list-presets` to see all available presets.

Here is an example to invoke the `gcc-debug` preset.

```shell
cmake --workflow --preset gcc-debug
```

Generally, there are two kinds of presets, `debug` and `release`.

The `debug` presets are designed to aid development, so it has debugging
instrumentation enabled and as many sanitizers turned on as possible.

> [!NOTE]
>
> The set of sanitizer supports are different across compilers.
> You can checkout the exact set of compiler arguments by looking at the toolchain
> files under the [`cmake`](cmake/) directory.

The `release` presets are designed for use in production environments,
thus they have the highest optimization turned on (e.g. `O3`).

### Configure and Build Manually

While [CMake Presets](#configure-and-build-the-project-using-cmake-presets) are
convenient, you might want to set different configuration or compiler arguments
than any provided preset supports.

To configure, build and test the project with extra arguments,
you can run this set of commands.

```bash
cmake -B build -S . -DCMAKE_CXX_STANDARD=20 # Your extra arguments here.
cmake --build build
ctest --test-dir build
```

> [!IMPORTANT]
>
> Beman projects are
> [passive projects](https://github.com/bemanproject/beman/blob/main/docs/beman_standard.md#cmake),
> therefore,
> you will need to specify the C++ version via `CMAKE_CXX_STANDARD`
> when manually configuring the project.

### Finding and Fetching GTest from GitHub

If you do not have GoogleTest installed on your development system, you may
optionally configure this project to download a known-compatible release of
GoogleTest from source and build it as well.

Example commands:

```shell
cmake -B build -S . \
    -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=./infra/cmake/use-fetch-content.cmake \
    -DCMAKE_CXX_STANDARD=20
cmake --build build --target all
cmake --build build --target test
```

The precise version of GoogleTest that will be used is maintained in
`./lockfile.json`.

### Project specific configure arguments

When configuring the project manually,
you can pass an array of project specific CMake configs to customize your build.

Project specific options are prefixed with `BEMAN_RANGE_SEARCHER`.
You can see the list of available options with:

```bash
cmake -LH | grep "BEMAN_RANGE_SEARCHER" -C 2
```

<details>

<summary> Details of CMake arguments. </summary>

#### `BEMAN_RANGE_SEARCHER_BUILD_TESTS`

Enable building tests and test infrastructure. Default: ON.
Values: { ON, OFF }.

You can configure the project to have this option turned off via:

```bash
cmake -B build -S . -DCMAKE_CXX_STANDARD=20 -DBEMAN_RANGE_SEARCHER_BUILD_TESTS=OFF
```

> [!TIP]
> Because this project requires Google Tests as part of its development
> dependency,
> disable building tests avoids the project from pulling Google Tests from
> GitHub.

#### `BEMAN_RANGE_SEARCHER_BUILD_EXAMPLES`

Enable building examples. Default: ON. Values: { ON, OFF }.

</details>

## Integrate beman.range_searcher into your project

To use `beman.range_searcher` in your C++ project,
include an appropriate `beman.range_searcher` header from your source code.

```c++
#include <beman/range_searcher/searcher.hpp>
```

> [!NOTE]
>
> `beman.range_searcher` headers are to be included with the `beman/range_searcher/` directories prefixed.
> It is not supported to alter include search paths to spell the include target another way. For instance,
> `#include <searcher.hpp>` is not a supported interface.

How you will link your project against `beman.range_searcher` will depend on your build system.
CMake instructions are provided in following sections.

### Linking your project to beman.range_searcher with CMake

For CMake based projects,
you will need to use the `beman.range_searcher` CMake module
to define the `beman::range_searcher` CMake target:

```cmake
find_package(beman.range_searcher REQUIRED)
```

You will also need to add `beman::range_searcher` to the link libraries of
any libraries or executables that include beman.range_searcher's header file.

```cmake
target_link_libraries(yourlib PUBLIC beman::range_searcher)
```

### Produce beman.range_searcher static library locally

You can include range_searcher's headers locally
by producing a static `libbeman.range_searcher.a` library.

```bash
cmake --workflow --preset gcc-release
cmake --install build/gcc-release --prefix /opt/beman.range_searcher
```

This will generate such directory structure at `/opt/beman.range_searcher`.

```txt
/opt/beman.range_searcher
├── include
│   └── beman
│       └── range_searcher
│           └── searcher.hpp
└── lib
    └── libbeman.range_searcher.a
```
