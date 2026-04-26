// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <algorithm>
#include <functional>
#include <vector>

#include <gtest/gtest.h>

#include <beman/range_searcher/searcher.hpp>

namespace exe     = beman::range_searcher;
namespace branges = exe::ranges;

struct NonConstView : std::ranges::view_base {
    explicit NonConstView(int* b, int* e) : b_(b), e_(e) {}
    const int* begin() { return b_; } // deliberately non-const
    const int* end() { return e_; }   // deliberately non-const
    const int* b_;
    const int* e_;
};

struct MoveOnlyFunctor {
    MoveOnlyFunctor()                                      = default;
    MoveOnlyFunctor(const MoveOnlyFunctor&)                = delete;
    MoveOnlyFunctor& operator=(const MoveOnlyFunctor&)     = delete;
    MoveOnlyFunctor(MoveOnlyFunctor&&) noexcept            = default;
    MoveOnlyFunctor& operator=(MoveOnlyFunctor&&) noexcept = default;
    int              operator()(int a, const std::unique_ptr<int>& b) const { return a + *b; }
};

template <typename Iter, typename Sen, typename Searcher, typename Out>
void split(Iter first, Sen last, const Searcher& s, Out& out) {
    while (first != last) {
        auto found = s(first, last);
        out.emplace_back(first, std::ranges::begin(found));
        // if the pattern is found at the end of the input, output an empty chunk.
        if (std::ranges::end(found) == last && !std::ranges::empty(found))
            return;
        first = std::ranges::end(found); // start the next search here
    }
}

TEST(RangeSearcher, General) {
    std::vector vec  = {1, 2, 3, 4};
    std::vector vec2 = {2, 3};
    std::vector vec3 = {2, 4};
    ASSERT_TRUE(branges::contains_subrange(vec, branges::default_searcher{vec2}));
    ASSERT_EQ(branges::search(vec, branges::default_searcher{vec2}).begin() - vec.begin(), 1);
    ASSERT_EQ(branges::search(vec, branges::default_searcher{vec2}).end() - vec.begin(), 3);
    ASSERT_FALSE(branges::contains_subrange(vec, branges::default_searcher{vec3}));
    ASSERT_EQ(branges::search(vec, branges::default_searcher{vec3}).begin(), vec.end());
    ASSERT_EQ(branges::search(vec, branges::default_searcher{vec3}).end(), vec.end());
    ASSERT_TRUE(branges::contains_subrange(vec, branges::boyer_moore_searcher{vec2}));
    ASSERT_EQ(branges::search(vec, branges::boyer_moore_searcher{vec2}).begin() - vec.begin(), 1);
    ASSERT_EQ(branges::search(vec, branges::boyer_moore_searcher{vec2}).end() - vec.begin(), 3);
    ASSERT_FALSE(branges::contains_subrange(vec, branges::boyer_moore_searcher{vec3}));
    ASSERT_EQ(branges::search(vec, branges::boyer_moore_searcher{vec3}).begin(), vec.end());
    ASSERT_EQ(branges::search(vec, branges::boyer_moore_searcher{vec3}).end(), vec.end());
    ASSERT_TRUE(branges::contains_subrange(vec, branges::boyer_moore_horspool_searcher{vec2}));
    ASSERT_EQ(branges::search(vec, branges::boyer_moore_horspool_searcher{vec2}).begin() - vec.begin(), 1);
    ASSERT_EQ(branges::search(vec, branges::boyer_moore_horspool_searcher{vec2}).end() - vec.begin(), 3);
    ASSERT_FALSE(branges::contains_subrange(vec, branges::boyer_moore_horspool_searcher{vec3}));
    ASSERT_EQ(branges::search(vec, branges::boyer_moore_horspool_searcher{vec3}).begin(), vec.end());
    ASSERT_EQ(branges::search(vec, branges::boyer_moore_horspool_searcher{vec3}).end(), vec.end());

    std::string      haystack = "a quick brown fox jumps over the lazy dog";
    std::string_view needle   = "jump";
    ASSERT_TRUE(branges::contains_subrange(haystack, branges::default_searcher{needle}));
    ASSERT_EQ(branges::search(haystack, branges::default_searcher{needle}).begin() - haystack.begin(), 18);
    ASSERT_EQ(branges::search(haystack, branges::default_searcher{needle}).end() - haystack.begin(), 22);
    ASSERT_FALSE(branges::contains_subrange(haystack, branges::default_searcher{"Jump"}));
    ASSERT_EQ(branges::search(haystack, branges::default_searcher{"Jump"}).begin(), haystack.end());
    ASSERT_EQ(branges::search(haystack, branges::default_searcher{"Jump"}).end(), haystack.end());
    ASSERT_TRUE(branges::contains_subrange(haystack, branges::boyer_moore_searcher{needle}));
    ASSERT_EQ(branges::search(haystack, branges::boyer_moore_searcher{needle}).begin() - haystack.begin(), 18);
    ASSERT_EQ(branges::search(haystack, branges::boyer_moore_searcher{needle}).end() - haystack.begin(), 22);
    ASSERT_FALSE(branges::contains_subrange(haystack, branges::boyer_moore_searcher{"Jump"}));
    ASSERT_EQ(branges::search(haystack, branges::boyer_moore_searcher{"Jump"}).begin(), haystack.end());
    ASSERT_EQ(branges::search(haystack, branges::boyer_moore_searcher{"Jump"}).end(), haystack.end());
    ASSERT_TRUE(branges::contains_subrange(haystack, branges::boyer_moore_horspool_searcher{needle}));
    ASSERT_EQ(branges::search(haystack, branges::boyer_moore_horspool_searcher{needle}).begin() - haystack.begin(),
              18);
    ASSERT_EQ(branges::search(haystack, branges::boyer_moore_horspool_searcher{needle}).end() - haystack.begin(), 22);
    ASSERT_FALSE(branges::contains_subrange(haystack, branges::boyer_moore_horspool_searcher{"Jump"}));
    ASSERT_EQ(branges::search(haystack, branges::boyer_moore_horspool_searcher{"Jump"}).begin(), haystack.end());
    ASSERT_EQ(branges::search(haystack, branges::boyer_moore_horspool_searcher{"Jump"}).end(), haystack.end());
}

TEST(RangeSearcher, Constexpr) {
    static constexpr std::array vec  = {1, 2, 3, 4};
    static constexpr std::array vec2 = {2, 3};
    static constexpr std::array vec3 = {2, 4};
    static_assert(branges::contains_subrange(vec, branges::default_searcher{vec2}));
    static_assert(branges::search(vec, branges::default_searcher{vec2}).begin() == vec.begin() + 1);
    static_assert(branges::search(vec, branges::default_searcher{vec2}).end() == vec.begin() + 3);
    static_assert(!branges::contains_subrange(vec, branges::default_searcher{vec3}));
    static_assert(branges::search(vec, branges::default_searcher{vec3}).begin() == vec.end());
    static_assert(branges::search(vec, branges::default_searcher{vec3}).end() == vec.end());

    static constexpr std::string_view haystack = "a quick brown fox jumps over the lazy dog";
    static constexpr std::string_view needle   = "jump";
    static_assert(branges::contains_subrange(haystack, branges::default_searcher{needle}));
    static_assert(branges::search(haystack, branges::default_searcher{needle}).begin() == haystack.begin() + 18);
    static_assert(branges::search(haystack, branges::default_searcher{needle}).end() == haystack.begin() + 22);
    static_assert(!branges::contains_subrange(haystack, branges::default_searcher{"Jump"}));
    static_assert(branges::search(haystack, branges::default_searcher{"Jump"}).begin() == haystack.end());
    static_assert(branges::search(haystack, branges::default_searcher{"Jump"}).end() == haystack.end());
}

TEST(RangeSearcher, Views) {
    std::vector vec   = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    auto        view1 = NonConstView{vec.data(), vec.data() + vec.size()};
    std::vector vec2  = {2, 3};
    auto        view2 = vec2 | std::views::transform([](auto x) { return x + 3; });
    auto        view3 = std::views::single(10);
    ASSERT_TRUE(branges::contains_subrange(view1, branges::default_searcher{view2}));
    ASSERT_EQ(branges::search(view1, branges::default_searcher{view2}).begin() - view1.begin(), 4);
    ASSERT_EQ(branges::search(view1, branges::default_searcher{view2}).end() - view1.begin(), 6);
    ASSERT_FALSE(branges::contains_subrange(view1, branges::default_searcher{view3}));
    ASSERT_EQ(branges::search(view1, branges::default_searcher{view3}).begin(), view1.end());
    ASSERT_EQ(branges::search(view1, branges::default_searcher{view3}).end(), view1.end());
    ASSERT_TRUE(branges::contains_subrange(view1, branges::boyer_moore_searcher{view2}));
    ASSERT_EQ(branges::search(view1, branges::boyer_moore_searcher{view2}).begin() - view1.begin(), 4);
    ASSERT_EQ(branges::search(view1, branges::boyer_moore_searcher{view2}).end() - view1.begin(), 6);
    ASSERT_FALSE(branges::contains_subrange(view1, branges::boyer_moore_searcher{view3}));
    ASSERT_EQ(branges::search(view1, branges::boyer_moore_searcher{view3}).begin(), view1.end());
    ASSERT_EQ(branges::search(view1, branges::boyer_moore_searcher{view3}).end(), view1.end());
    ASSERT_TRUE(branges::contains_subrange(view1, branges::boyer_moore_horspool_searcher{view2}));
    ASSERT_EQ(branges::search(view1, branges::boyer_moore_horspool_searcher{view2}).begin() - view1.begin(), 4);
    ASSERT_EQ(branges::search(view1, branges::boyer_moore_horspool_searcher{view2}).end() - view1.begin(), 6);
    ASSERT_FALSE(branges::contains_subrange(view1, branges::boyer_moore_horspool_searcher{view3}));
    ASSERT_EQ(branges::search(view1, branges::boyer_moore_horspool_searcher{view3}).begin(), view1.end());
    ASSERT_EQ(branges::search(view1, branges::boyer_moore_horspool_searcher{view3}).end(), view1.end());

    std::string haystack = "a quick brown fox jumps abc over the lazy dog";
    ASSERT_TRUE(branges::contains_subrange(haystack, branges::default_searcher{std::views::iota('a', 'd')}));
    ASSERT_EQ(branges::search(haystack, branges::default_searcher{std::views::iota('a', 'd')}).begin() -
                  haystack.begin(),
              24);
    ASSERT_EQ(
        branges::search(haystack, branges::default_searcher{std::views::iota('a', 'd')}).end() - haystack.begin(), 27);
    ASSERT_TRUE(branges::contains_subrange(haystack, branges::boyer_moore_searcher{std::views::iota('a', 'd')}));
    ASSERT_EQ(branges::search(haystack, branges::boyer_moore_searcher{std::views::iota('a', 'd')}).begin() -
                  haystack.begin(),
              24);
    ASSERT_EQ(branges::search(haystack, branges::boyer_moore_searcher{std::views::iota('a', 'd')}).end() -
                  haystack.begin(),
              27);
    ASSERT_TRUE(
        branges::contains_subrange(haystack, branges::boyer_moore_horspool_searcher{std::views::iota('a', 'd')}));
    ASSERT_EQ(branges::search(haystack, branges::boyer_moore_horspool_searcher{std::views::iota('a', 'd')}).begin() -
                  haystack.begin(),
              24);
    ASSERT_EQ(branges::search(haystack, branges::boyer_moore_horspool_searcher{std::views::iota('a', 'd')}).end() -
                  haystack.begin(),
              27);
}

TEST(RangeSearcher, Empty) {
    std::vector      vec  = {1, 2, 3, 4};
    std::vector      vec2 = {2, 3};
    std::vector<int> vec3;
    ASSERT_TRUE(branges::contains_subrange(vec, branges::default_searcher{vec2}));
    ASSERT_EQ(branges::search(vec, branges::default_searcher{vec2}).begin() - vec.begin(), 1);
    ASSERT_EQ(branges::search(vec, branges::default_searcher{vec2}).end() - vec.begin(), 3);
    ASSERT_FALSE(branges::contains_subrange(vec, branges::default_searcher{vec3}));
    ASSERT_EQ(branges::search(vec, branges::default_searcher{vec3}).begin(), vec.begin());
    ASSERT_EQ(branges::search(vec, branges::default_searcher{vec3}).end(), vec.begin());
    ASSERT_TRUE(branges::contains_subrange(vec, branges::boyer_moore_searcher{vec2}));
    ASSERT_EQ(branges::search(vec, branges::boyer_moore_searcher{vec2}).begin() - vec.begin(), 1);
    ASSERT_EQ(branges::search(vec, branges::boyer_moore_searcher{vec2}).end() - vec.begin(), 3);
    ASSERT_FALSE(branges::contains_subrange(vec, branges::boyer_moore_searcher{vec3}));
    ASSERT_EQ(branges::search(vec, branges::boyer_moore_searcher{vec3}).begin(), vec.begin());
    ASSERT_EQ(branges::search(vec, branges::boyer_moore_searcher{vec3}).end(), vec.begin());
    ASSERT_TRUE(branges::contains_subrange(vec, branges::boyer_moore_horspool_searcher{vec2}));
    ASSERT_EQ(branges::search(vec, branges::boyer_moore_horspool_searcher{vec2}).begin() - vec.begin(), 1);
    ASSERT_EQ(branges::search(vec, branges::boyer_moore_horspool_searcher{vec2}).end() - vec.begin(), 3);
    ASSERT_FALSE(branges::contains_subrange(vec, branges::boyer_moore_horspool_searcher{vec3}));
    ASSERT_EQ(branges::search(vec, branges::boyer_moore_horspool_searcher{vec3}).begin(), vec.begin());
    ASSERT_EQ(branges::search(vec, branges::boyer_moore_horspool_searcher{vec3}).end(), vec.begin());
    ASSERT_FALSE(branges::contains_subrange(vec3, branges::default_searcher{vec2}));
    ASSERT_EQ(branges::search(vec3, branges::default_searcher{vec2}).begin(), vec3.end());
    ASSERT_EQ(branges::search(vec3, branges::default_searcher{vec2}).end(), vec3.end());
    ASSERT_FALSE(branges::contains_subrange(vec3, branges::boyer_moore_searcher{vec2}));
    ASSERT_EQ(branges::search(vec3, branges::boyer_moore_searcher{vec2}).begin(), vec3.end());
    ASSERT_EQ(branges::search(vec3, branges::boyer_moore_searcher{vec2}).end(), vec3.end());
    ASSERT_FALSE(branges::contains_subrange(vec3, branges::boyer_moore_horspool_searcher{vec2}));
    ASSERT_EQ(branges::search(vec3, branges::boyer_moore_horspool_searcher{vec2}).begin(), vec3.end());
    ASSERT_EQ(branges::search(vec3, branges::boyer_moore_horspool_searcher{vec2}).end(), vec3.end());

    std::string haystack = "a quick brown fox jumps over the lazy dog";
    auto        needle   = std::views::single('u');
    ASSERT_TRUE(branges::contains_subrange(haystack, branges::default_searcher{needle}));
    ASSERT_EQ(branges::search(haystack, branges::default_searcher{needle}).begin() - haystack.begin(), 3);
    ASSERT_EQ(branges::search(haystack, branges::default_searcher{needle}).end() - haystack.begin(), 4);
    ASSERT_FALSE(branges::contains_subrange(haystack, branges::default_searcher{""}));
    ASSERT_EQ(branges::search(haystack, branges::default_searcher{std::string{}}).begin(), haystack.begin());
    ASSERT_EQ(branges::search(haystack, branges::default_searcher{std::string{}}).end(), haystack.begin());
    ASSERT_TRUE(branges::contains_subrange(haystack, branges::boyer_moore_searcher{needle}));
    ASSERT_EQ(branges::search(haystack, branges::boyer_moore_searcher{needle}).begin() - haystack.begin(), 3);
    ASSERT_EQ(branges::search(haystack, branges::boyer_moore_searcher{needle}).end() - haystack.begin(), 4);
    ASSERT_FALSE(branges::contains_subrange(haystack, branges::boyer_moore_searcher{std::string{}}));
    ASSERT_EQ(branges::search(haystack, branges::boyer_moore_searcher{std::string{}}).begin(), haystack.begin());
    ASSERT_EQ(branges::search(haystack, branges::boyer_moore_searcher{std::string{}}).end(), haystack.begin());
    ASSERT_TRUE(branges::contains_subrange(haystack, branges::boyer_moore_horspool_searcher{needle}));
    ASSERT_EQ(branges::search(haystack, branges::boyer_moore_horspool_searcher{needle}).begin() - haystack.begin(), 3);
    ASSERT_EQ(branges::search(haystack, branges::boyer_moore_horspool_searcher{needle}).end() - haystack.begin(), 4);
    ASSERT_FALSE(branges::contains_subrange(haystack, branges::boyer_moore_horspool_searcher{std::string{}}));
    ASSERT_EQ(branges::search(haystack, branges::boyer_moore_horspool_searcher{std::string{}}).begin(),
              haystack.begin());
    ASSERT_EQ(branges::search(haystack, branges::boyer_moore_horspool_searcher{std::string{}}).end(),
              haystack.begin());
}

TEST(RangeSearcher, Split) {
    std::vector                   vec1 = {1, 2, 3, 4, 2, 5, 6, 2, 7, 8};
    std::vector                   vec2 = {2};
    std::vector<std::vector<int>> result;
    auto                          test = [&]<typename Searcher>() {
        result.clear();
        split(vec1.begin(), vec1.end(), Searcher{vec2}, result);
        ASSERT_EQ(result.size(), 4);
        ASSERT_EQ(result[0], std::vector{1});
        ASSERT_EQ(result[1], (std::vector{3, 4}));
        ASSERT_EQ(result[2], (std::vector{5, 6}));
        ASSERT_EQ(result[3], (std::vector{7, 8}));
    };
    test.operator()<branges::default_searcher<std::vector<int>>>();
    test.operator()<branges::boyer_moore_searcher<std::vector<int>>>();
    test.operator()<branges::boyer_moore_horspool_searcher<std::vector<int>>>();

    std::string              sentence = "A quick brown fox";
    std::vector<std::string> result2;
    auto                     test2 = [&]<typename Searcher>() {
        result2.clear();
        split(sentence.begin(), sentence.end(), Searcher{std::string_view{" "}}, result2);
        ASSERT_EQ(result2.size(), 4);
        ASSERT_EQ(result2[0], "A");
        ASSERT_EQ(result2[1], "quick");
        ASSERT_EQ(result2[2], "brown");
        ASSERT_EQ(result2[3], "fox");
    };
    test2.operator()<branges::default_searcher<std::string_view>>();
    test2.operator()<branges::boyer_moore_searcher<std::string_view>>();
    test2.operator()<branges::boyer_moore_horspool_searcher<std::string_view>>();
}
