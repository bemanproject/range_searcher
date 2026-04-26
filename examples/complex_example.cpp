#include <functional>
#include <iostream>
#include <string>
#include <string_view>

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
    std::string needle   = "Jump";

    if (branges::contains_subrange(haystack, branges::default_searcher{needle}))
        println("Jump found!");
    else
        println("Jump not found!");
    if (branges::contains_subrange(haystack, branges::boyer_moore_searcher{needle, [](auto a, auto b) {
                                                                               return std::tolower(a) ==
                                                                                      std::tolower(b);
                                                                           }}))
        println("Jump (case-insensitive) found!");
    else
        println("Jump (case-insensitive) not found!");
    if (branges::contains_subrange(haystack, branges::boyer_moore_horspool_searcher{needle, {}, [](auto c) -> char {
                                                                                        return std::tolower(c);
                                                                                    }}))
        println("Jump (case-insensitive) found!");
    else
        println("Jump (case-insensitive) not found!");

    auto result = branges::search(
        haystack, branges::boyer_moore_horspool_searcher{"Jumps-Over"}, [](char ch) { return ch == ' ' ? '-' : ch; });
    println("Offset: " + std::to_string(result.begin() - haystack.begin()));
    auto result2 = branges::search(
        haystack,
        branges::default_searcher{std::string_view{"Jumps-Over"}, {}, [](auto c) -> char { return std::tolower(c); }},
        [](char ch) { return ch == ' ' ? '-' : ch; });
    println("Offset: " + std::to_string(result2.begin() - haystack.begin()));

    return 0;
}
