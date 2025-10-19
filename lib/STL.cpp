#include "utils/ADT/SmallVector.hpp"
#include "utils/ADT/StringRef.hpp"
#include "utils/misc.hpp"
#include <cstdio>
#include <print>

template <typename T, std::size_t N>
using SmallVector = utils::ADT::SmallVector<T, N>;

int main() {
  SmallVector<uint64_t, 4> array;

  array.push_back(114514);

  std::print("array size: {}\n", array.size());
  std::print("array[0]: {}\n", array[0]);

  utils::ADT::StringRef str("rs0[4:0]");
  std::print("str num: {}\n",
             utils::stoi(str.slice(6, str.size() - 2).c_str(), 1));

  utils::ADT::StringRef range("11:0");

  int length = 0;
  for (auto pattern : range.split<8>('|')) {

    if (pattern.empty()) {
      continue;
    }

    auto nums = pattern.split<8>(':');

    auto high = nums[0];
    auto low = nums[1].empty() ? nums[0] : nums[1];

    auto high_bit = utils::stoi(high.c_str(), high.size());
    auto low_bit = utils::stoi(low.c_str(), low.size());

    length += high_bit - low_bit + 1;
  }
  std::print("length: {}\n", length);

  // utils::ADT::StringRef pattern(
  //     "offset[12|10:5] rs2[4:0] rs1[4:0] 000 offset[4:1|11] 1100011");

  // auto pattern_array = pattern.split<8>(' ');

  // for (auto& pat : pattern_array) {
  //   // std::printf("%s\n", pat.c_str());
  // }

  utils::ADT::StringRef pattern2("[20|10:1|11|19:12]");

  auto pattern_array2 = pattern2.slice(1, pattern2.size() - 1).split<8>('|');

  std::printf("%s size: %ld \n", pattern2.slice(1, pattern2.size() - 1).c_str(),
              pattern2.slice(1, pattern2.size() - 1).size());

  for (auto& pat2 : pattern_array2) {
    std::printf("%s size: %ld\n", pat2.c_str(), pat2.size());
  }

  return 0;
}