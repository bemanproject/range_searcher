// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef BEMAN_RANGE_SEARCHER_SEARCHER_HPP
#define BEMAN_RANGE_SEARCHER_SEARCHER_HPP

#include <beman/range_searcher/config.hpp>

#if BEMAN_RANGE_SEARCHER_USE_MODULES() && !defined(BEMAN_RANGE_SEARCHER_INCLUDED_FROM_INTERFACE_UNIT)

import beman.range_searcher;

#else

    #if !BEMAN_RANGE_SEARCHER_USE_MODULES()

        #include <algorithm>
        #include <functional>
        #include <iterator>
        #include <memory>
        #include <ranges>
        #include <type_traits>

    #endif // !BEMAN_RANGE_SEARCHER_USE_MODULES()

    #include "detail/expo_only.hpp"

namespace beman::range_searcher {

template <class Searcher, class I, class S, class P = std::identity>
concept searchable = std::movable<Searcher> && std::invocable<const Searcher&, I, S, P> &&
                     std::ranges::forward_range<std::invoke_result_t<const Searcher&, I, S, P> >;

namespace ranges {

template <std::ranges::forward_range                                           V,
          std::copy_constructible                                              Pred = std::ranges::equal_to,
          std::indirectly_regular_unary_invocable<std::ranges::iterator_t<V> > Proj = std::identity>
    requires std::ranges::view<V>
class default_searcher {
  public:
    constexpr explicit default_searcher(V base, Pred pred = {}, Proj proj = {})
        : base_(std::move(base)), pred_(std::move(pred)), proj_(std::move(proj)) {}

    template <std::forward_iterator I2, std::sentinel_for<I2> S2, class Proj2 = std::identity>
        requires std::indirectly_comparable<I2, std::ranges::iterator_t<V>, Pred, Proj2, Proj>
    constexpr std::ranges::subrange<I2> operator()(I2 first, S2 last, Proj2 proj2 = {}) const {
        return std::ranges::search(
            first, last, std::ranges::begin(base_), std::ranges::end(base_), pred_, std::move(proj2), proj_);
    }

  private:
    [[no_unique_address]] V    base_; // exposition only
    [[no_unique_address]] Pred pred_; // exposition only
    [[no_unique_address]] Proj proj_; // exposition only
};

template <std::ranges::forward_range                                           R,
          std::copy_constructible                                              Pred = std::ranges::equal_to,
          std::indirectly_regular_unary_invocable<std::ranges::iterator_t<R> > Proj = std::identity>
default_searcher(R&&, Pred = {}, Proj = {}) -> default_searcher<std::views::all_t<R>, Pred, Proj>;

template <std::ranges::random_access_range                                     V,
          std::copy_constructible                                              Pred = std::ranges::equal_to,
          std::indirectly_regular_unary_invocable<std::ranges::iterator_t<V> > Proj = std::identity,
          class Hash = std::hash<detail::projected_value_t<std::ranges::iterator_t<V>, Proj> > >
    requires std::ranges::view<V> && std::semiregular<std::ranges::range_value_t<V> >
class boyer_moore_searcher {
  private:
    using difference_type = std::ranges::range_difference_t<V>;
    using value_type      = detail::projected_value_t<std::ranges::iterator_t<V>, Proj>;
    using skip_table_type = detail::BMSkipTable<value_type,
                                                difference_type,
                                                detail::hash_wrapper<Hash, Proj>,
                                                Pred,
                                                std::is_integral_v<value_type> && sizeof(value_type) == 1 &&
                                                    std::is_same_v<Hash, std::hash<value_type> > &&
                                                    std::is_same_v<Pred, std::ranges::equal_to> >;

  public:
    explicit boyer_moore_searcher(V base, Pred pred = {}, Proj proj = {}, Hash hf = {})
        : base_(std::move(base)),
          pred_(std::move(pred)),
          proj_(std::move(proj)),
          pattern_length_(std::ranges::distance(base)),
          skip_table_(std::make_shared<skip_table_type>(
              pattern_length_, -1, detail::hash_wrapper<Hash, Proj>(hf, proj_), pred_)),
          suffix_(std::make_shared<difference_type[]>(pattern_length_ + 1)) {
        difference_type i         = 0;
        auto            pat_first = std::ranges::begin(base);
        auto            pat_last  = std::ranges::end(base);
        while (pat_first != pat_last) {
            skip_table_->insert(std::invoke(proj_, *pat_first), i);
            ++pat_first;
            ++i;
        }
        build_suffix_table(std::ranges::begin(base_), std::ranges::end(base_), pred_, proj_);
    }

    template <std::random_access_iterator I2, std::sentinel_for<I2> S2, class Proj2 = std::identity>
        requires std::indirectly_comparable<I2, std::ranges::iterator_t<V>, Pred, Proj2, Proj>
    std::ranges::subrange<I2> operator()(I2 first, S2 last, Proj2 proj2 = {}) const {
        static_assert(std::is_same_v<std::remove_cvref_t<value_type>,
                                     std::remove_cvref_t<detail::projected_value_t<I2, Proj2> > >,
                      "Corpus and Pattern iterators must point to the same type");
        if (first == last)
            return {first, last};
        if (std::ranges::empty(base_))
            return {first, first};

        if (pattern_length_ > (last - first))
            return {last, last};
        return search(first, last, proj2);
    }

  private:
    [[no_unique_address]] V            base_; // exposition only
    [[no_unique_address]] Pred         pred_; // exposition only
    [[no_unique_address]] Proj         proj_; // exposition only
    difference_type                    pattern_length_;
    std::shared_ptr<skip_table_type>   skip_table_;
    std::shared_ptr<difference_type[]> suffix_;

    template <typename I2, typename S2, typename Proj2>
    std::ranges::subrange<I2> search(I2 f, S2 l, Proj2 proj2) const {
        I2                     current    = f;
        const I2               last       = l - pattern_length_;
        const skip_table_type& skip_table = *skip_table_;

        while (current <= last) {
            difference_type j = pattern_length_;
            while (std::invoke(pred_, std::invoke(proj_, base_[j - 1]), std::invoke(proj2, current[j - 1]))) {
                --j;
                if (j == 0)
                    return {current, current + pattern_length_};
            }

            difference_type k = skip_table[std::invoke(proj2, current[j - 1])];
            difference_type m = j - k - 1;
            if (k < j && m > suffix_[j])
                current += m;
            else
                current += suffix_[j];
        }
        return {l, l};
    }

    template <typename I2, typename S2, typename Container>
    static void compute_bm_prefix(I2 first, S2 last, Pred pred, Proj proj, Container& prefix) {
        const std::size_t count = last - first;

        prefix[0]     = 0;
        std::size_t k = 0;

        for (std::size_t i = 1; i != count; ++i) {
            while (k > 0 && !std::invoke(pred, std::invoke(proj, first[k]), std::invoke(proj, first[i])))
                k = prefix[k - 1];

            if (std::invoke(pred, std::invoke(proj, first[k]), std::invoke(proj, first[i])))
                ++k;
            prefix[i] = k;
        }
    }

    void build_suffix_table(std::ranges::iterator_t<V> first, std::ranges::sentinel_t<V> last, Pred pred, Proj proj) {
        const std::size_t count = last - first;

        if (count == 0)
            return;

        auto scratch = std::make_unique<difference_type[]>(count);

        compute_bm_prefix(first, last, pred, proj, scratch);
        for (std::size_t i = 0; i <= count; ++i)
            suffix_[i] = count - scratch[count - 1];

        using ReverseIter = std::reverse_iterator<std::ranges::iterator_t<V> >;
        compute_bm_prefix(ReverseIter(last), ReverseIter(first), pred, proj, scratch);

        for (std::size_t i = 0; i != count; ++i) {
            const std::size_t     j = count - scratch[i];
            const difference_type k = i - scratch[i] + 1;

            if (suffix_[j] > k)
                suffix_[j] = k;
        }
    }
};

template <std::ranges::random_access_range                                     R,
          std::copy_constructible                                              Pred = std::ranges::equal_to,
          std::indirectly_regular_unary_invocable<std::ranges::iterator_t<R> > Proj = std::identity,
          class Hash = std::hash<detail::projected_value_t<std::ranges::iterator_t<R>, Proj> > >
    requires std::semiregular<std::ranges::range_value_t<R> >
boyer_moore_searcher(R&&, Pred = {}, Proj = {}, Hash = {})
    -> boyer_moore_searcher<std::views::all_t<R>, Pred, Proj, Hash>;

template <std::ranges::random_access_range                                     V,
          std::copy_constructible                                              Pred = std::ranges::equal_to,
          std::indirectly_regular_unary_invocable<std::ranges::iterator_t<V> > Proj = std::identity,
          class Hash = std::hash<detail::projected_value_t<std::ranges::iterator_t<V>, Proj> > >
    requires std::ranges::view<V> && std::semiregular<std::ranges::range_value_t<V> >
class boyer_moore_horspool_searcher {
  private:
    using difference_type = std::ranges::range_difference_t<V>;
    using value_type      = detail::projected_value_t<std::ranges::iterator_t<V>, Proj>;
    using skip_table_type = detail::BMSkipTable<value_type,
                                                difference_type,
                                                detail::hash_wrapper<Hash, Proj>,
                                                Pred,
                                                std::is_integral_v<value_type> && sizeof(value_type) == 1 &&
                                                    std::is_same_v<Hash, std::hash<value_type> > &&
                                                    std::is_same_v<Pred, std::ranges::equal_to> >;

  public:
    explicit boyer_moore_horspool_searcher(V base, Pred pred = {}, Proj proj = {}, Hash hf = {})
        : base_(std::move(base)),
          pred_(std::move(pred)),
          proj_(std::move(proj)),
          pattern_length_(std::ranges::distance(base_)),
          skip_table_(std::make_shared<skip_table_type>(
              pattern_length_, pattern_length_, detail::hash_wrapper<Hash, Proj>(hf, proj_), pred_)) {
        if (std::ranges::empty(base_))
            return;
        auto pat_first = std::ranges::begin(base_);
        auto pat_last  = std::ranges::end(base_);
        --pat_last;
        difference_type i = 0;
        while (pat_first != pat_last) {
            skip_table_->insert(std::invoke(proj_, *pat_first), pattern_length_ - 1 - i);
            ++pat_first;
            ++i;
        }
    }

    template <std::random_access_iterator I2, std::sentinel_for<I2> S2, class Proj2 = std::identity>
        requires std::indirectly_comparable<I2, std::ranges::iterator_t<V>, Pred, Proj2, Proj>
    std::ranges::subrange<I2> operator()(I2 first, S2 last, Proj2 proj2 = {}) const {
        static_assert(std::is_same_v<std::remove_cvref_t<value_type>,
                                     std::remove_cvref_t<detail::projected_value_t<I2, Proj2> > >,
                      "Corpus and Pattern iterators must point to the same type");
        if (first == last)
            return {first, last};
        if (std::ranges::empty(base_))
            return {first, first};

        if (pattern_length_ > (last - first))
            return {last, last};
        return search(first, last, proj2);
    }

  private:
    [[no_unique_address]] V          base_; // exposition only
    [[no_unique_address]] Pred       pred_; // exposition only
    [[no_unique_address]] Proj       proj_; // exposition only
    difference_type                  pattern_length_;
    std::shared_ptr<skip_table_type> skip_table_;

    template <typename I2, typename S2, typename Proj2>
    std::ranges::subrange<I2> search(I2 f, S2 l, Proj2 proj2) const {
        I2                     current    = f;
        const I2               last       = l - pattern_length_;
        const skip_table_type& skip_table = *skip_table_;

        while (current <= last) {
            difference_type j = pattern_length_;
            while (std::invoke(pred_, std::invoke(proj_, base_[j - 1]), std::invoke(proj2, current[j - 1]))) {
                --j;
                if (j == 0)
                    return {current, current + pattern_length_};
            }
            current += skip_table[std::invoke(proj2, current[pattern_length_ - 1])];
        }
        return {l, l};
    }
};

template <std::ranges::random_access_range                                     R,
          std::copy_constructible                                              Pred = std::ranges::equal_to,
          std::indirectly_regular_unary_invocable<std::ranges::iterator_t<R> > Proj = std::identity,
          class Hash = std::hash<detail::projected_value_t<std::ranges::iterator_t<R>, Proj> > >
    requires std::semiregular<std::ranges::range_value_t<R> >
boyer_moore_horspool_searcher(R&&, Pred = {}, Proj = {}, Hash = {})
    -> boyer_moore_horspool_searcher<std::views::all_t<R>, Pred, Proj, Hash>;

template <std::forward_iterator I, std::sentinel_for<I> S, class Searcher, class Proj = std::identity>
    requires searchable<Searcher, I, S, Proj>
constexpr auto search(I first, S last, const Searcher& searcher, Proj proj = {}) {
    return searcher(first, last, std::move(proj));
}
template <std::ranges::forward_range R, class Searcher, class Proj = std::identity>
    requires searchable<Searcher, std::ranges::iterator_t<R>, std::ranges::sentinel_t<R>, Proj>
constexpr auto search(R&& r, const Searcher& searcher, Proj proj = {}) {
    return searcher(std::ranges::begin(r), std::ranges::end(r), std::move(proj));
}

template <std::forward_iterator I, std::sentinel_for<I> S, class Searcher, class Proj = std::identity>
    requires searchable<Searcher, I, S, Proj>
constexpr bool contains_subrange(I first, S last, const Searcher& searcher, Proj proj = {}) {
    return !std::ranges::empty(search(first, last, searcher, std::move(proj)));
}
template <std::ranges::forward_range R, class Searcher, class Proj = std::identity>
    requires searchable<Searcher, std::ranges::iterator_t<R>, std::ranges::sentinel_t<R>, Proj>
constexpr bool contains_subrange(R&& r, const Searcher& searcher, Proj proj = {}) {
    return !std::ranges::empty(search(std::forward<R>(r), searcher, std::move(proj)));
}

} // namespace ranges

} // namespace beman::range_searcher

#endif // #if BEMAN_RANGE_SEARCHER_USE_MODULES() &&
       // !defined(BEMAN_RANGE_SEARCHER_INCLUDED_FROM_INTERFACE_UNIT)

#endif // BEMAN_RANGE_SEARCHER_SEARCHER_HPP
