// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef BEMAN_RANGE_SEARCHER_SEARCHER_HPP
#define BEMAN_RANGE_SEARCHER_SEARCHER_HPP

#include <algorithm>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <ranges>
#include <type_traits>
#include <unordered_map>

namespace beman::range_searcher {

namespace detail {

template <std::indirectly_readable I, std::indirectly_regular_unary_invocable<I> Proj>
using projected_value_t = std::remove_cvref_t<std::invoke_result_t<Proj&, std::iter_value_t<I>&>>;

// The following is directly copied from libc++ <include/functional/boyer_moore_searcher.h>
// With modification to support projection
template <class Key, class Value, class Hash, class BinaryPredicate, bool /*useArray*/>
class BMSkipTable;

// General case for BM data searching; use a map
template <class Key, class Value, class Hash, class BinaryPredicate>
class BMSkipTable<Key, Value, Hash, BinaryPredicate, false> {
  private:
    using value_type = Value;
    using key_type   = Key;

    const value_type                                      default_value_;
    std::unordered_map<Key, Value, Hash, BinaryPredicate> table_;

  public:
    explicit BMSkipTable(size_t sz, value_type default_value, Hash hash, BinaryPredicate pred)
        : default_value_(default_value), table_(sz, hash, pred) {}

    void insert(const key_type& key, value_type val) { table_[key] = val; }

    value_type operator[](const key_type& key) const {
        auto it = table_.find(key);
        return it == table_.end() ? default_value_ : it->second;
    }
};

// Special case small numeric values; use an array
template <class Key, class Value, class Hash, class BinaryPredicate>
class BMSkipTable<Key, Value, Hash, BinaryPredicate, true> {
  private:
    using value_type = Value;
    using key_type   = Key;

    using unsigned_key_type = std::make_unsigned_t<key_type>;
    std::array<value_type, 256> table_;
    static_assert(std::numeric_limits<unsigned_key_type>::max() < 256);

  public:
    explicit BMSkipTable(size_t, value_type default_value, Hash, BinaryPredicate) {
        std::fill_n(table_.data(), table_.size(), default_value);
    }

    void insert(key_type key, value_type val) { table_[static_cast<unsigned_key_type>(key)] = val; }

    value_type operator[](key_type key) const { return table_[static_cast<unsigned_key_type>(key)]; }
};

} // namespace detail

template <class Searcher, class I, class S, class P = std::identity>
concept searchable = std::copyable<Searcher> && std::invocable<Searcher, I, S, P> &&
                     std::ranges::forward_range<std::invoke_result_t<Searcher, I, S, P>>;

namespace ranges {

template <std::ranges::forward_range                                          R,
          std::copy_constructible                                             Pred = std::ranges::equal_to,
          std::indirectly_regular_unary_invocable<std::ranges::iterator_t<R>> Proj = std::identity>
class default_searcher {
  public:
    constexpr default_searcher(std::ranges::iterator_t<R> pat_first,
                               std::ranges::sentinel_t<R> pat_last,
                               Pred                       pred = {},
                               Proj                       proj = {})
        : pat_first_(pat_first), pat_last_(pat_last), pred_(std::move(pred)), proj_(std::move(proj)) {}
    template <typename R2>
        requires std::same_as<std::decay_t<R2>, R>
    constexpr explicit default_searcher(R2&& r) : default_searcher(std::ranges::begin(r), std::ranges::end(r)) {}
    template <typename R2>
        requires std::same_as<std::decay_t<R2>, R>
    constexpr default_searcher(R2&& r, Pred pred, Proj proj = {})
        : default_searcher(std::ranges::begin(r), std::ranges::end(r), std::move(pred), std::move(proj)) {}

    template <std::forward_iterator I2, std::sentinel_for<I2> S2, class Proj2 = std::identity>
        requires std::indirectly_comparable<I2, std::ranges::iterator_t<R>, Pred, Proj2, Proj>
    constexpr std::ranges::subrange<I2> operator()(I2 first, S2 last, Proj2 proj2 = {}) const {
        return std::ranges::search(first, last, pat_first_, pat_last_, pred_, proj2, proj_);
    }
    template <std::ranges::forward_range R2, class Proj2 = std::identity>
        requires std::indirectly_comparable<std::ranges::iterator_t<R2>, std::ranges::iterator_t<R>, Pred, Proj2, Proj>
    constexpr std::ranges::borrowed_subrange_t<R2> operator()(R2&& r2, Proj2 proj2 = {}) const {
        return std::ranges::search(std::forward<R2>(r2), pat_first_, pat_last_, pred_, proj2, proj_);
    }

  private:
    [[no_unique_address]] std::ranges::iterator_t<R> pat_first_; // exposition only
    [[no_unique_address]] std::ranges::sentinel_t<R> pat_last_;  // exposition only
    [[no_unique_address]] Pred                       pred_;      // exposition only
    [[no_unique_address]] Proj                       proj_;      // exposition only
};

template <std::ranges::forward_range                                          R,
          std::copy_constructible                                             Pred = std::ranges::equal_to,
          std::indirectly_regular_unary_invocable<std::ranges::iterator_t<R>> Proj = std::identity>
default_searcher(R&&, Pred = {}, Proj = {}) -> default_searcher<std::decay_t<R>, Pred, Proj>;

template <std::ranges::random_access_range                                    R,
          std::copy_constructible                                             Pred = std::ranges::equal_to,
          std::indirectly_regular_unary_invocable<std::ranges::iterator_t<R>> Proj = std::identity,
          class Hash = std::hash<detail::projected_value_t<std::ranges::iterator_t<R>, Proj>>>
class boyer_moore_searcher {
  private:
    using difference_type = std::ranges::range_difference_t<R>;
    using value_type      = detail::projected_value_t<std::ranges::iterator_t<R>, Proj>;
    using skip_table_type = detail::BMSkipTable<value_type,
                                                difference_type,
                                                Hash,
                                                Pred,
                                                std::is_integral_v<value_type> && sizeof(value_type) == 1 &&
                                                    std::is_same_v<Hash, std::hash<value_type>> &&
                                                    std::is_same_v<Pred, std::ranges::equal_to>>;

  public:
    boyer_moore_searcher(std::ranges::iterator_t<R> pat_first,
                         std::ranges::sentinel_t<R> pat_last,
                         Hash                       hash = {},
                         Pred                       pred = {},
                         Proj                       proj = {})
        : pat_first_(pat_first),
          pat_last_(pat_last),
          pred_(std::move(pred)),
          proj_(std::move(proj)),
          pattern_length_(pat_last - pat_first),
          skip_table_(std::make_shared<skip_table_type>(pattern_length_, -1, hash, pred_)),
          suffix_(std::__allocate_shared_unbounded_array<difference_type[]>(std::allocator<difference_type>(),
                                                                            pattern_length_ + 1)) {
        difference_type i = 0;
        while (pat_first != pat_last) {
            skip_table_->insert(*pat_first, i);
            ++pat_first;
            ++i;
        }
        build_suffix_table(pat_first_, pat_last_, pred_, proj_);
    }
    template <typename R2>
        requires std::same_as<std::decay_t<R2>, R>
    explicit boyer_moore_searcher(R2&& r) : boyer_moore_searcher(std::ranges::begin(r), std::ranges::end(r)) {}
    template <typename R2>
        requires std::same_as<std::decay_t<R2>, R>
    boyer_moore_searcher(R2& r, Hash hash, Pred pred = {}, Proj proj = {})
        : boyer_moore_searcher(
              std::ranges::begin(r), std::ranges::end(r), std::move(hash), std::move(pred), std::move(proj)) {}

    template <std::random_access_iterator I2, std::sentinel_for<I2> S2, class Proj2 = std::identity>
        requires std::indirectly_comparable<I2, std::ranges::iterator_t<R>, Pred, Proj2, Proj>
    std::ranges::subrange<I2> operator()(I2 first, S2 last, Proj2 proj2 = {}) const {
        static_assert(
            std::is_same_v<std::remove_cvref_t<value_type>, std::remove_cvref_t<detail::projected_value_t<I2, Proj2>>>,
            "Corpus and Pattern iterators must point to the same type");
        if (first == last)
            return {first, last};
        if (pat_first_ == pat_last_)
            return {first, first};

        if (pattern_length_ > (last - first))
            return {last, last};
        return search(first, last, proj2);
    }
    template <std::ranges::random_access_range R2, class Proj2 = std::identity>
        requires std::indirectly_comparable<std::ranges::iterator_t<R2>, std::ranges::iterator_t<R>, Pred, Proj2, Proj>
    std::ranges::borrowed_subrange_t<R2> operator()(R2&& r2, Proj2 proj2 = {}) const {
        return operator()(std::ranges::begin(r2), std::ranges::end(r2), proj2);
    }

  private:
    [[no_unique_address]] std::ranges::iterator_t<R> pat_first_; // exposition only
    [[no_unique_address]] std::ranges::sentinel_t<R> pat_last_;  // exposition only
    [[no_unique_address]] Pred                       pred_;      // exposition only
    [[no_unique_address]] Proj                       proj_;      // exposition only
    difference_type                                  pattern_length_;
    std::shared_ptr<skip_table_type>                 skip_table_;
    std::shared_ptr<difference_type[]>               suffix_;

    template <typename I2, typename S2, typename Proj2>
    std::ranges::subrange<I2> search(I2 f, S2 l, Proj2 proj2) const {
        I2                     current    = f;
        const I2               last       = l - pattern_length_;
        const skip_table_type& skip_table = *skip_table_;

        while (current <= last) {
            difference_type j = pattern_length_;
            while (pred_(std::invoke(proj_, pat_first_[j - 1]), std::invoke(proj2, current[j - 1]))) {
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
        const size_t count = last - first;

        prefix[0] = 0;
        size_t k  = 0;

        for (size_t i = 1; i != count; ++i) {
            while (k > 0 && !pred(std::invoke(proj, first[k]), std::invoke(proj, first[i])))
                k = prefix[k - 1];

            if (pred(std::invoke(proj, first[k]), std::invoke(proj, first[i])))
                ++k;
            prefix[i] = k;
        }
    }

    void build_suffix_table(std::ranges::iterator_t<R> first, std::ranges::sentinel_t<R> last, Pred pred, Proj proj) {
        const size_t count = last - first;

        if (count == 0)
            return;

        auto scratch = std::make_unique<difference_type[]>(count);

        compute_bm_prefix(first, last, pred, proj, scratch);
        for (size_t i = 0; i <= count; ++i)
            suffix_[i] = count - scratch[count - 1];

        using ReverseIter = std::reverse_iterator<std::ranges::iterator_t<R>>;
        compute_bm_prefix(ReverseIter(last), ReverseIter(first), pred, proj, scratch);

        for (size_t i = 0; i != count; ++i) {
            const size_t          j = count - scratch[i];
            const difference_type k = i - scratch[i] + 1;

            if (suffix_[j] > k)
                suffix_[j] = k;
        }
    }
};

template <std::ranges::random_access_range                                    R,
          std::copy_constructible                                             Pred = std::ranges::equal_to,
          std::indirectly_regular_unary_invocable<std::ranges::iterator_t<R>> Proj = std::identity,
          class Hash = std::hash<detail::projected_value_t<std::ranges::iterator_t<R>, Proj>>>
boyer_moore_searcher(R&&, Hash = {}, Pred = {}, Proj = {}) -> boyer_moore_searcher<std::decay_t<R>, Pred, Proj, Hash>;

template <std::forward_iterator I, std::sentinel_for<I> S, class Searcher, class Proj = std::identity>
    requires searchable<Searcher, I, S, Proj>
constexpr auto search(I first, S last, const Searcher& searcher, Proj proj = {}) {
    return searcher(first, last, proj);
}
template <std::ranges::forward_range R, class Searcher, class Proj = std::identity>
    requires searchable<Searcher, std::ranges::iterator_t<R>, std::ranges::sentinel_t<R>, Proj>
constexpr auto search(R&& r, const Searcher& searcher, Proj proj = {}) {
    return searcher(std::ranges::begin(r), std::ranges::end(r), proj);
}

template <std::forward_iterator I, std::sentinel_for<I> S, class Searcher, class Proj = std::identity>
    requires searchable<Searcher, I, S, Proj>
constexpr bool contains_subrange(I first, S last, const Searcher& searcher, Proj proj = {}) {
    return !std::ranges::empty(search(first, last, searcher, proj));
}
template <std::ranges::forward_range R, class Searcher, class Proj = std::identity>
    requires searchable<Searcher, std::ranges::iterator_t<R>, std::ranges::sentinel_t<R>, Proj>
constexpr bool contains_subrange(R&& r, const Searcher& searcher, Proj proj = {}) {
    return !std::ranges::empty(search(std::forward<R>(r), searcher, proj));
}

} // namespace ranges

} // namespace beman::range_searcher

#endif // BEMAN_RANGE_SEARCHER_SEARCHER_HPP
