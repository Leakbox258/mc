#ifndef UTILS_ADT_STRINGREF
#define UTILS_ADT_STRINGREF

#include "ArrayRef.hpp"
#include "iterator_range.hpp"
#include "utils/macro.hpp"
#include <cassert>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <string>
#include <string_view>
#include <type_traits>
namespace utils {
namespace ADT {

class StringRef {
  public:
    using iter = const char*;
    using const_iter = const char*;
    using size_ty = size_t;
    using value_ty = char;
    using rev_iter = std::reverse_iterator<iter>;
    using const_rev_iter = std::reverse_iterator<const_iter>;

  private:
    const char* Data = nullptr;
    std::size_t Length = 0;

  public:
    StringRef() = default;

    constexpr StringRef(std::string_view Str)
        : Data(Str.data()), Length(Str.size()) {}

    constexpr StringRef(const std::string& Str LIFETIME_BOUND)
        : Data(Str.data()), Length(Str.size()) {}

    constexpr StringRef(const char* data LIFETIME_BOUND, std::size_t length)
        : Data(data), Length(length) {}

    constexpr StringRef(const char* Str LIFETIME_BOUND)
        : StringRef(Str ? std::string_view(Str) : std::string_view()) {}

    constexpr StringRef(StringRef&& StrRef)
        : Data(StrRef.Data), Length(StrRef.Length) {}

    constexpr StringRef(const StringRef& StrRef)
        : Data(StrRef.Data), Length(StrRef.Length) {}

    template <typename T> StringRef& operator=(T&& StrRef) = delete;

    [[nodiscard]] constexpr const char* data() const { return Data; }
    [[nodiscard]] constexpr size_ty size() const { return Length; }
    [[nodiscard]] constexpr bool empty() const { return !Length; }

    /// @}
    /// @name Iters
    /// @{

    iter begin() const { return data(); }

    iter end() const { return data() + size(); }

    rev_iter rbegin() const { return std::make_reverse_iterator(end()); }

    rev_iter rend() const { return std::make_reverse_iterator(begin()); }

    const unsigned char* bytes_begin() const {
        return reinterpret_cast<const unsigned char*>(begin());
    }
    const unsigned char* bytes_end() const {
        return reinterpret_cast<const unsigned char*>(end());
    }
    iterator_range<const unsigned char*> bytes() const {
        return makeRange(bytes_begin(), bytes_end());
    }

    [[nodiscard]] char front() const {
        assert(!empty());
        return data()[0];
    }
    [[nodiscard]] char back() const {
        assert(!empty());
        return data()[size() - 1];
    }

    /// check if the String the same
    template <typename T>
        requires ViewType<T, std::decay_t<decltype(Data)>>
    [[nodiscard]] bool operator==(T&& StrRef) const {
        if (StrRef.size() != this->size()) {
            return false;
        }

        return !std::memcmp(this->data(), StrRef.data(), this->size());
    }

    /// c-style char array
    template <std::size_t N>
    [[nodiscard]] bool operator==(const char (&Array)[N]) const {
        if (this->size() != N) {
            return false;
        }

        return !std::memcmp(this->data(), Array, N);
    }

    template <typename T>
        requires ViewType<T, std::decay_t<decltype(Data)>>
    [[nodiscard]]
    bool same_instance(T&& StrRef) {
        return this->data() == StrRef.data(); // no size check
    }

    template <std::size_t N>
    [[nodiscard]] bool same_instance(const char (&Array)[N]) const {
        return N == this->size();
    }

    [[nodiscard]] char operator[](std::size_t index) const {
        assert(index < this->size());
        return data()[index];
    }
};

} // namespace ADT
} // namespace utils

#endif