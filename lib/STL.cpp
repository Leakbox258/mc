#include "utils/ADT/SmallVector.hpp"
#include <print>

template <typename T, std::size_t N>
using SmallVector = utils::ADT::SmallVector<T, N>;

int main() {
  SmallVector<uint64_t, 4> array;

  array.push_back(114514);

  std::print("{}", array[0]);
  return 0;
}