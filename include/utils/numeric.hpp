#ifndef UTILS_NUMERIC
#define UTILS_NUMERIC

#include <functional>
namespace utils {

template <typename T, bool LeftBounded, bool RightBounded>
inline bool in_interval(T left, T right, T val) {

    std::less<T> LessThan;
    std::equal_to<T> Equal;

    if (LessThan(left, val) && LessThan(val, right)) {
        return true;
    }

    if constexpr (LeftBounded) {
        return Equal(val, left);
    }

    if constexpr (RightBounded) {
        return Equal(val, right);
    }

    return false;
}

template <typename T> inline bool in_interval_t(T left, T right, T val) {

    return in_interval<T, true, false>(left, right, val);
}

template <typename R>
inline auto size(
    R&& Range,
    std::enable_if_t<
        std::is_base_of<std::random_access_iterator_tag,
                        typename std::iterator_traits<
                            decltype(Range.begin())>::iterator_category>::value,
        void>* = nullptr) {
    return std::distance(Range.begin(), Range.end());
}

/// constexpr
/// TODO: replace return value with std::optional
constexpr inline int stoi(const char* str, std::size_t len, int base = 10) {
    if (base < 2 || base > 36) {
        return -1;
    }

    if (!str) {
        return -1;
    }

    int result = 0;
    bool is_negative = false;
    size_t i = 0;

    if (str[i] == '-') {
        is_negative = true;
        ++i;
    }

    for (; i < len; ++i) {
        char ch = str[i];
        int digit = 0;

        if (ch >= '0' && ch <= '9') {
            digit = ch - '0';
        } else if (ch >= 'a' && ch <= 'z') {
            digit = ch - 'a' + 10;
        } else if (ch >= 'A' && ch <= 'Z') {
            digit = ch - 'A' + 10;
        } else {
            return -1;
        }

        if (digit >= base) {
            return -1;
        }

        result = result * base + digit;
    }

    return is_negative ? -result : result;
}

} // namespace utils

#endif