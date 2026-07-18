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
    #include <optional>
    #include <type_traits>
    #include <unordered_map>

#endif // !BEMAN_RANGE_SEARCHER_USE_MODULES()

namespace beman::range_searcher::detail {

// until C++23, `__movable_box` was named `__copyable_box` and required the stored type to be copy-constructible, not
// just move-constructible; we preserve the old behavior in pre-C++23 modes.
template <class Tp>
concept movable_box_object =
#if __cpp_lib_ranges >= 202207L
    std::move_constructible<Tp>
#else
    std::copy_constructible<Tp>
#endif
    && std::is_object_v<Tp>;

// Primary template - uses std::optional and introduces an empty state in case assignment fails.
template <movable_box_object Tp>
class movable_box {
    [[no_unique_address]] std::optional<Tp> val_;

  public:
    template <class... Args>
        requires std::is_constructible_v<Tp, Args...>
    constexpr explicit movable_box(std::in_place_t,
                                   Args&&... args) noexcept(std::is_nothrow_constructible_v<Tp, Args...>)
        : val_(std::in_place, std::forward<Args>(args)...) {}

    constexpr movable_box() noexcept(std::is_nothrow_default_constructible_v<Tp>)
        requires std::default_initializable<Tp>
        : val_(std::in_place) {}

    movable_box(const movable_box&) = default;
    movable_box(movable_box&&)      = default;

    constexpr movable_box& operator=(const movable_box& other) noexcept(std::is_nothrow_copy_constructible_v<Tp>)
#if __cpp_lib_ranges >= 202207L
        requires std::copy_constructible<Tp>
#endif
    {
        if (this != std::addressof(other)) {
            if (other.has_value())
                val_.emplace(*other);
            else
                val_.reset();
        }
        return *this;
    }

    movable_box& operator=(movable_box&&)
        requires std::movable<Tp>
    = default;

    constexpr movable_box& operator=(movable_box&& other) noexcept(std::is_nothrow_move_constructible_v<Tp>) {
        if (this != std::addressof(other)) {
            if (other.has_value())
                val_.emplace(std::move(*other));
            else
                val_.reset();
        }
        return *this;
    }

    constexpr const Tp& operator*() const noexcept { return *val_; }
    constexpr Tp&       operator*() noexcept { return *val_; }

    constexpr const Tp* operator->() const noexcept { return val_.operator->(); }
    constexpr Tp*       operator->() noexcept { return val_.operator->(); }

    [[nodiscard]] constexpr bool has_value() const noexcept { return val_.has_value(); }
};

// Workaround for MSVC bug:
// https://developercommunity.visualstudio.com/t/std%3A%3Aoptional-with-empty-class-does-not/10655185
template <movable_box_object Tp>
    requires std::is_empty_v<Tp>
class movable_box<Tp> {
    [[no_unique_address]] Tp val_;
    bool                     has_value_;

  public:
    template <class... Args>
        requires std::is_constructible_v<Tp, Args...>
    constexpr explicit movable_box(std::in_place_t,
                                   Args&&... args) noexcept(std::is_nothrow_constructible_v<Tp, Args...>)
        : val_(std::forward<Args>(args)...), has_value_(true) {}

    constexpr movable_box() noexcept(std::is_nothrow_default_constructible_v<Tp>)
        requires std::default_initializable<Tp>
        : val_(), has_value_(false) {}

    movable_box(const movable_box&) = default;
    movable_box(movable_box&&)      = default;

    constexpr movable_box& operator=(const movable_box& other) = default;
    constexpr movable_box& operator=(movable_box&& other)      = default;

    constexpr const Tp& operator*() const noexcept { return val_; }
    constexpr Tp&       operator*() noexcept { return val_; }

    constexpr const Tp* operator->() const noexcept { return &val_; }
    constexpr Tp*       operator->() noexcept { return &val_; }

    [[nodiscard]] constexpr bool has_value() const noexcept { return has_value_; }
};

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
    Hash              hash_;
    movable_box<Proj> proj_;

  public:
    hash_wrapper(Hash hash, Proj proj) : hash_(std::move(hash)), proj_(std::in_place, std::move(proj)) {}
    std::size_t operator()(const auto& key) const { return hash_(std::invoke(*proj_, key)); }
};

} // namespace beman::range_searcher::detail

#endif // BEMAN_RANGE_SEARCHER_DETAIL_EXPO_ONLY_HPP
