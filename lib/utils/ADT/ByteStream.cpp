#include "utils/ADT/ByteStream.hpp"

using namespace utils::ADT;

ByteStream::size_ty ByteStream::findOffset(StringRef Str) const {
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