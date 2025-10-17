#ifndef UTILS_ADT_STRINGSWITCH
#define UTILS_ADT_STRINGSWITCH

#include "StringRef.hpp"
#include "utils/logger.hpp"
#include <algorithm>
#include <optional>
#include <string_view>
#include <tuple>
#include <type_traits>
namespace utils {
namespace ADT {

template <typename T> class StringSwitch {
private:
  const StringRef Str;

  std::optional<T> Result;

public:
  explicit constexpr StringSwitch(StringRef S) : Str(S), Result(std::nullopt) {}

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

private:
  template <unsigned N> constexpr bool CaseImpl(auto&& tuple) {
    if constexpr (N >= std::tuple_size_v<std::decay_t<decltype(tuple)>> - 1) {
      return false;
    } else {
      if (StringRef(std::get<N>(tuple)) == this->Str) {
        return true;
      }
      return CaseImpl<N + 1>(tuple);
    }
  }

public:
  template <typename... ArgTys>
    requires(sizeof...(ArgTys) > 2)
  constexpr StringSwitch& Case(ArgTys&&... Args) {
    auto TupleArgs = std::make_tuple(std::forward<ArgTys>(Args)...);

    auto LastArg = std::get<sizeof...(ArgTys) - 1>(TupleArgs);

    if (!Result && CaseImpl<0>(TupleArgs)) {
      if constexpr (std::is_same_v<std::decay_t<decltype(LastArg)>, T>) {
        Result = std::move(LastArg);
      } else {
        Result = LastArg(this->Str);
      }
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

  T Error() {
    if (!Result) {
      utils::unreachable("StringSwitch::Error: unreachable condicate");
    }

    return std::move(*Result);
  }

  T Error(std::string_view message) {
    if (!Result) {
      utils::unreachable(message);
    }

    return std::move(*Result);
  }
};

} // namespace ADT
} // namespace utils

#endif