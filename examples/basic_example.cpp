#include <functional>
#include <iostream>
#include <string>

#if __cpp_lib_print >= 202207L
    #include <print>
#endif

#include <beman/range_searcher/searcher.hpp>

namespace exe     = beman::range_searcher;
namespace branges = exe::ranges;

#if __cpp_lib_print >= 202207L && __cpp_lib_format_ranges >= 202207L
void print(auto&& rng) { std::print("{:s}", std::forward<decltype(rng)>(rng)); }

void println(auto&& rng) { std::println("{:s}", std::forward<decltype(rng)>(rng)); }
#else
void print(auto&& rng) {
    for (auto&& elem : rng)
        std::cout << elem;
}

void println(auto&& rng) {
    print(std::forward<decltype(rng)>(rng));
    std::cout << "\n";
}
#endif

// Example given in the paper for range-based searchers. (Needs C++23)
int main() {
    std::string haystack = "a quick brown fox jumps over the lazy dog";
    std::string needle   = "jump";

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
    else
        println("jump not found!");
    if (branges::contains_subrange(haystack, branges::boyer_moore_horspool_searcher{"run"}))
        println("run found!");
    else
        println("run not found!");

    return 0;
}
