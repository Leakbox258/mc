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
auto size(
    R&& Range,
    std::enable_if_t<
        std::is_base_of<std::random_access_iterator_tag,
                        typename std::iterator_traits<
                            decltype(Range.begin())>::iterator_category>::value,
        void>* = nullptr) {
    return std::distance(Range.begin(), Range.end());
}

} // namespace utils

#endif