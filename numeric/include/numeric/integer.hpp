#pragma once

#include "./representative.hpp"

#include <stdexcept>

namespace numeric {

template<Sign S, Bits B>
struct Int
{
  static_assert(S != Sign::Nil || B == 0, "Nil Sign implies zero Bits");
  Repr<S, B> repr;

  explicit Int(IsRepr auto repr_)
    : repr(static_cast<Repr<S, B>>(repr_))
  {
    if (!internal::mask_check<S, B>(repr_))
      throw std::runtime_error{ "int not fit value" };
  }

  template<Sign S_, Bits B_>
  explicit Int(Int<S_, B_> rhs)
    : repr(static_cast<Repr<S, B>>(rhs.repr))
  {
    if (!internal::mask_check<S, B>(rhs.repr))
      throw std::runtime_error{ "int not fit value" };
  }
};

}
