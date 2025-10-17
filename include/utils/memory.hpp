#ifndef UTILS_MEMORY
#define UTILS_MEMORY

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
namespace utils {

inline void* malloc(std::size_t Size) {
  void* Res = std::malloc(Size);

  assert(Res);

  return Res;
}

template <typename T> inline void* malloc() {
  void* Res = std::malloc(sizeof(T));

  assert(Res);

  return Res;
}

inline void* realloc(void* MemRef, std::size_t NewSize) {
  assert(NewSize != 0);

  void* Res = std::realloc(MemRef, NewSize);

  assert(Res);

  return Res;
}

constexpr int memcmp(const void* ptr1, const void* ptr2, size_t num) {
  const char* p1 = static_cast<const char*>(ptr1);
  const char* p2 = static_cast<const char*>(ptr2);

  for (size_t i = 0; i < num; ++i) {
    if (p1[i] != p2[i]) {
      return static_cast<int>(p1[i]) - static_cast<int>(p2[i]);
    }
  }
  return 0; // If the memory blocks are equal
}

} // namespace utils

#endif