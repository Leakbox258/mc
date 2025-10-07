#ifndef UTILS_MEMORY
#define UTILS_MEMORY

#include "sys/addr.hpp"
#include <cstddef>
#include <cstdlib>
namespace utils {
namespace mem {
inline void* malloc(std::size_t Size) {
    void* Res = std::malloc(Size);

    assert(
        sys::VMMapInfo::instance().checkif(Res, sys::VMMapInfo::Segment::Heap));

    return Res;
}

template <typename T> inline void* malloc() {
    void* Res = std::malloc(sizeof(T));

    assert(
        sys::VMMapInfo::instance().checkif(Res, sys::VMMapInfo::Segment::Heap));

    return Res;
}

inline void* realloc(void* MemRef, std::size_t NewSize) {
    assert(NewSize != 0);

    void* Res = std::realloc(MemRef, NewSize);

    assert(
        sys::VMMapInfo::instance().checkif(Res, sys::VMMapInfo::Segment::Heap));

    return Res;
}

} // namespace mem
} // namespace utils

#endif