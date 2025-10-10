#ifndef UTILS_ADT_ARRAYREF
#define UTILS_ADT_ARRAYREF

#include "SmallVector.hpp"
#include "iterator_range.hpp"
#include "utils/macro.hpp"
#include "utils/misc.hpp"
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <type_traits>
#include <vector>
namespace utils {
namespace ADT {

template <typename Ty, typename ADTViewTy> // ADTViewTy = usually a ptr
concept ViewType =
    std::is_integral_v<decltype(std::declval<const Ty&>().size())> &&
    std::is_convertible_v<decltype(std::declval<const Ty&>().data())*,
                          const ADTViewTy*>;

template <typename T> class ArrayRef {
  public:
    using value_ty = T;
    using ptr = T*;
    using const_ptr = const T*;
    using ref = T&;
    using const_ref = const T&;
    using iter = T*;
    using const_iter = const T*;
    using rev_iter = std::reverse_iterator<T*>;
    using const_rev_iter = std::reverse_iterator<const T*>;
    using size_ty = std::size_t;
    using difference_ty = std::ptrdiff_t;

  private:
    const T* Data = nullptr;
    size_ty Length = 0;

  public:
    ArrayRef() = default;
    ArrayRef(const T& Elem LIFETIME_BOUND) : Data(&Elem), Length(1) {}
    ArrayRef(T&& Elem) = delete;
    constexpr ArrayRef(const T* data LIFETIME_BOUND, std::size_t length)
        : Data(data), Length(length) {}
    constexpr ArrayRef(const T* begin LIFETIME_BOUND,
                       const T* end LIFETIME_BOUND)
        : Data(begin), Length(end - begin) {
        assert(begin <= end);
    }

    template <typename C>
        requires ViewType<C, std::decay_t<decltype(Data)>>
    constexpr ArrayRef(const C& V) : Data(V.data()), Length(V.size()) {}

    /// C-style array
    template <std::size_t N>
    constexpr ArrayRef(const T (&Array LIFETIME_BOUND)[N])
        : Data(Array), Length(N) {}

    /// initialize_list
    constexpr ArrayRef(std::initializer_list<T> Vec LIFETIME_BOUND)
        : Data(Vec.empty() ? nullptr : Vec.begin()), Length(Vec.size()) {}

    /// iterator_range<U*>
    template <typename U, typename = std::enable_if_t<
                              std::is_convertible_v<U* const*, T* const*>>>
    constexpr ArrayRef(const iterator_range<U*>& Range)
        : Data(Range.begin()), Length(utils::size(Range)) {}

  public:
    iter begin() const { return Data; }
    iter end() const { return Data + Length; }

    rev_iter rbegin() const { return rev_iter(end()); }
    rev_iter rend() const { return rev_iter(begin()); }

    bool empty() const { return !Length; }
    size_ty size() const { return Length; }

    const T* data() const { return Data; }

    /// lifetime of a specific elem has nothing to do with `this`
    const T& front() const {
        assert(!empty());
        return Data[0];
    }
    const T& back() const {
        assert(!empty());
        return Data[Length - 1];
    }

    /// get a subview of `this`
    ArrayRef<T> slice(std::size_t N, std::size_t M) const {
        assert(N + M <= size() && "Invalid specifier");
        return ArrayRef<T>(data() + N, M);
    }

    const T& operator[](std::size_t Index) const {
        assert(Index < size());
        return Data[Index];
    }

    template <typename U>
    std::enable_if_t<std::is_same_v<U, T>, ArrayRef<T>>&
    operator=(U&& Temp) = delete;

    template <typename U>
    std::enable_if_t<std::is_same_v<U, T>, ArrayRef<T>>&
    operator=(std::initializer_list<U>) = delete;

    /// here is how to get a copy
    /// or to extend lifetime of inner elements
    std::vector<T> vec() const {
        return std::vector<T>(this->begin(), this->end());
    }
    operator std::vector<T>() const { return this->vec(); }
};

} // namespace ADT
} // namespace utils

#endif