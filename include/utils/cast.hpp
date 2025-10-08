#ifndef UTILS_CAST
#define UTILS_CAST

#include <cassert>
namespace utils {
template <typename To, typename From>
[[nodiscard]] inline bool isa(From* Val) noexcept {
    if (!Val) {
        return false;
    }

    return To::classof(Val);
}

template <typename To, typename From>
[[nodiscard]] inline const To* cast(From* Val) noexcept {
    assert(isa<To>(Val));

    return (To*)Val;
}

template <typename To, typename From>
[[nodiscard]] inline const To* dyn_cast(From* Val) noexcept {
    if (!Val && !isa<To>(Val)) {
        return nullptr;
    }

    return cast<To>(Val);
}

} // namespace utils
#endif