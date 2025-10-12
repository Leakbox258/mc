#ifndef UTILS_MACRO
#define UTILS_MACRO

#if defined(__clang__)
#define LIFETIME_BOUND [[clang::lifetimebound]]
#elif defined(__GNUC__)
#define LIFETIME_BOUND
#endif

#include "utils/logger.hpp"
#include <source_location>

#ifndef NDEBUG
#define utils_assert(condition, message)                                       \
  utils::assert_handler(static_cast<bool>(condition), #condition, (message),   \
                        std::source_location::current())
#else
#define utils_assert(condition, message) ((void)0)
#endif

#endif
