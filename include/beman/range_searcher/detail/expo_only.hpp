// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef BEMAN_RANGE_SEARCHER_DETAIL_EXPO_ONLY_HPP
#define BEMAN_RANGE_SEARCHER_DETAIL_EXPO_ONLY_HPP

#include <beman/range_searcher/config.hpp>

#if !BEMAN_RANGE_SEARCHER_USE_MODULES()

    #include <algorithm>
    #include <array>
    #include <functional>
    #include <iterator>
    #include <limits>
    #include <type_traits>
    #include <unordered_map>

#endif // !BEMAN_RANGE_SEARCHER_USE_MODULES()

namespace beman::range_searcher::detail {

template <std::indirectly_readable I, std::indirectly_regular_unary_invocable<I> Proj>
using projected_value_t = std::remove_cvref_t<std::invoke_result_t<Proj&, std::iter_value_t<I>&> >;

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
    explicit BMSkipTable(std::size_t sz, value_type default_value, Hash hash, BinaryPredicate pred)
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
    explicit BMSkipTable(std::size_t, value_type default_value, Hash, BinaryPredicate) {
        std::fill_n(table_.data(), table_.size(), default_value);
    }

    void insert(key_type key, value_type val) { table_[static_cast<unsigned_key_type>(key)] = val; }

    value_type operator[](key_type key) const { return table_[static_cast<unsigned_key_type>(key)]; }
};

// Wrapper for hash that respects projection
template <typename Hash, typename Proj>
class hash_wrapper {
  private:
    Hash hash_;
    Proj proj_;

  public:
    hash_wrapper(const Hash& hash, const Proj& proj) : hash_(hash), proj_(proj) {}
    std::size_t operator()(const auto& key) const { return hash_(std::invoke(proj_, key)); }
};

} // namespace beman::range_searcher::detail

#endif // BEMAN_RANGE_SEARCHER_DETAIL_EXPO_ONLY_HPP
