#ifndef UTILS_LIKEHOOD
#define UTILS_LIKEHOOD

namespace utils {

template <typename T> inline bool is_likely(const T& expr) {
    return static_cast<bool>(__builtin_expect(static_cast<long>(expr), 1l));
}

template <typename T> inline bool is_unlikely(const T& expr) {
    return static_cast<bool>(__builtin_expect(static_cast<long>(expr), 0l));
}

} // namespace utils

#endif