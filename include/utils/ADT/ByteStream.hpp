#ifndef UTILS_ADT_BYTESTREAM
#define UTILS_ADT_BYTESTREAM

#include "SmallVector.hpp"
#include "StringRef.hpp"

namespace utils {
namespace ADT {

struct ByteStream {
  using size_ty = std::size_t;

  std::vector<uint8_t> buffer;

  explicit ByteStream() : buffer(4096) {}

  const unsigned char* data() const { return buffer.data(); }

  size_ty size() const { return buffer.size(); }

  void balignTo(size_ty balign) {
    auto paddingSize = (balign - (buffer.size() % balign)) % balign;

    for (auto i = 0ull; i < paddingSize; ++i) {
      buffer.push_back('\x00');
    }
  }

  template <IsPOD T> ByteStream& operator<<(T&& Value) {

    this->balignTo(sizeof(T));

    for (int i = 0; i < sizeof(T); ++i) {
      buffer.push_back(((uint8_t*)(&Value))[i]);
    }

    return *this;
  }

  template <size_ty Num> ByteStream& operator<<(const char (&Value)[Num]) {
    for (int i = 0; i < Num; ++i) {
      buffer.push_back(Value[i]);
    }

    return *this;
  }

  /// for .strtab
  size_ty findOffset(StringRef Str) const {
    size_ty offset = 0;
    for (const auto& chr : buffer) {

      if (chr == *Str.begin()) {
        if (!std::memcmp(Str.data(), buffer.data() + offset, Str.size())) {
          return offset;
        }

        ++offset;
      }
    }

    utils::unreachable("cant find Str in ByteStream");
  };

}; // namespace ADT
} // namespace ADT
} // namespace utils
#endif
