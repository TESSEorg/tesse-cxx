#ifndef TTG_BUFFER_H
#define TTG_BUFFER_H

#include <memory>
#include "ttg/impl_selector.h"


namespace ttg {

template<typename T, typename Allocator = std::allocator<T>>
using buffer = TTG_IMPL_NS::buffer<T, Allocator>;

namespace detail {
  template<typename T>
  struct is_buffer : std::false_type
  { };

  template<typename T, typename A>
  struct is_buffer<ttg::buffer<T, A>> : std::true_type
  { };

  template<typename T>
  constexpr bool is_buffer_v = is_buffer<T>::value;

  static_assert(is_buffer_v<ttg::buffer<double>>);
  static_assert(is_buffer_v<TTG_IMPL_NS::buffer<double>>);
} // namespace detail

} // namespace ttg

#endif // TTG_buffer_H