#ifndef UTILS_ADT_STRINGSWITCH
#define UTILS_ADT_STRINGSWITCH

#include "StringRef.hpp"
#include <algorithm>
#include <optional>
namespace utils {
namespace ADT {

template <typename T> class StringSwitch {
  private:
    const StringRef Str;

    std::optional<T> Result;

  public:
    explicit constexpr StringSwitch(StringRef S) : Str(S), Result() {}

    /// forbidden copy
    StringSwitch(const StringSwitch&) = delete;
    void operator=(const StringSwitch&) = delete;
    void operator=(StringSwitch&& other) = delete;

    StringSwitch(StringSwitch&& other)
        : Str(other.Str), Result(std::move(other.Result)) {}

    ~StringSwitch() = default;

    // using FnTy = std::function<T(const StringRef&)>;

    constexpr StringSwitch& Case(StringRef S, T Value) {
        if (!Result && Str == S) {
            Result = std::move(Value);
        }

        return *this;
    }

    constexpr StringSwitch& Case(StringRef S, auto&& fn) {
        if (!Result && Str == S) {
            Result = std::move(fn(Str));
        }

        return *this;
    }

    constexpr StringSwitch& BeginWith(StringRef S, T Value) {
        if (!Result && Str.begin_with(S)) {
            Result = std::move(Value);
        }

        return *this;
    }

    constexpr StringSwitch& BeginWith(StringRef S, auto&& fn) {
        if (!Result && Str.begin_with(S)) {
            Result = std::move(fn(Str));
        }

        return *this;
    }

    constexpr StringSwitch& EndWith(StringRef S, T Value) {
        if (!Result && Str.end_with(S)) {
            Result = std::move(Value);
        }

        return *this;
    }

    constexpr StringSwitch& EndWith(StringRef S, auto&& fn) {
        if (!Result && Str.end_with(S)) {
            Result = std::move(fn(Str));
        }

        return *this;
    }

    /// end your stringswitch with this
    constexpr T Default(T Value) {
        if (Result) {
            return std::move(*Result);
        }

        return Value;
    }

    constexpr T Default(auto&& fn) {
        if (Result) {
            return std::move(*Result);
        }

        return fn(Str);
    }

    constexpr T Default() { return std::move(*Result); }
};

} // namespace ADT
} // namespace utils

#endif