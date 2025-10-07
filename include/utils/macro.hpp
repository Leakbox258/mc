#ifndef UTILS_MACRO
#define UTILS_MACRO

#if defined(__clang__)
#define LIFETIME_BOUND [[clang::lifetimebound]]
#elif defined(__GNUC__)
#define LIFETIME_BOUND
#endif

#endif