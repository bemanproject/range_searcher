# Examples for beman.range_searcher

<!--
SPDX-License-Identifier: 2.0 license with LLVM exceptions
-->

List of usage examples for `beman.range_searcher`.

## Samples

Check basic `beman.range_searcher` library usages:

* local [./basic_example.cpp](./basic_example.cpp) or [basic_example@Compiler Explorer](https://godbolt.org/z/hG7vGW9MT)
* local [./complex_example.cpp](./complex_example.cpp) or [complex_example@Compiler Explorer](https://godbolt.org/z/c4Yq35vET)

### Local Build and Run

```bash
# building
$ cmake --workflow --preset llvm-release

# run sample.cpp
$ ./build/llvm-release/examples/beman.range_searcher.examples.basic_example
a quick brown fox [jump]s over the lazy dog
jump found!
run not found!
```
