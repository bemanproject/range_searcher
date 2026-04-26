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
