#ifndef TTG_VOID_H
#define TTG_VOID_H
#include <iostream>
#include "ttg/util/meta.h"

namespace ttg {
  /// @brief A complete version of void

  /// Void can be used interchangeably with void as key or value type, but is also hashable, etc.
  /// May reduce the amount of metaprogramming relative to void.
  class Void {
  public:
    Void() = default;
    template <typename T> Void(T&&) {}
  };

  static inline
  bool operator==(const Void&, const Void&) { return true; }
  static inline
  bool operator!=(const Void&, const Void&) { return false; }

  static inline
  std::ostream& operator<<(std::ostream& os, const ttg::Void&) {
    return os;
  }

  static_assert(meta::is_empty_tuple_v<std::tuple<>>,"ouch");
  static_assert(meta::is_empty_tuple_v<std::tuple<Void>>,"ouch");

}  // namespace ttg

namespace std {
  template <>
  struct hash<ttg::Void> {
    template <typename ... Args> int64_t operator()(Args&& ... args) const { return 0; }
  };
} // namespace std

#endif // TTG_VOID_H
