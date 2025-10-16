#ifndef UTILS_ADT_ITERATOR_RANGE
#define UTILS_ADT_ITERATOR_RANGE

#include <iterator>
#include <type_traits>
#include <utility>
namespace utils {
namespace ADT {

template <typename IterT> class iterator_range {
  IterT begin_iter, end_iter;

  template <typename From, typename To>
  using IterT_convertible = std::is_convertible<
      decltype(std::begin(std::declval<std::add_rvalue_reference_t<From>>())),
      To>;

public:
  template <typename Container>
    requires IterT_convertible<Container, IterT>::value
  iterator_range(Container&& c)
      : begin_iter(std::begin(std::forward<Container>(c))),
        end_iter(std::end(std::forward<Container>(c))) {}

  iterator_range(IterT _begin, IterT _end)
      : begin_iter(std::move(_begin)), end_iter(std::move(_end)) {}

  IterT begin() const { return begin_iter; }
  IterT end() const { return end_iter; }
  bool empty() const { return begin_iter == end_iter; }
};

template <typename T> iterator_range<T> makeRange(T x, T y) {
  return iterator_range<T>(std::move(x), std::move(y));
}

template <typename T> iterator_range<T> makeRange(std::pair<T, T> pair) {
  return iterator_range<T>(std::move(pair.first), std::move(pair.second));
}

} // namespace ADT

} // namespace utils

#endif