#ifndef UTILS_SOURCE
#define UTILS_SOURCE

/// TODO: SourceMgr, Dump

#include "ADT/StringRef.hpp"
namespace utils {

struct Location {
    ADT::StringRef filename;
    unsigned line, col;
};

} // namespace utils

#endif
