#ifndef UTILS_SOURCE
#define UTILS_SOURCE

/// TODO: SourceMgr, Dump

#include "ADT/StringRef.hpp"
#include <cstddef>
namespace utils {

struct Location {
    std::size_t line, col;
};

} // namespace utils

#endif
