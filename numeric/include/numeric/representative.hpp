#pragma once

#include "../assert.hpp"

#include <cstdint>
#include <type_traits>

namespace numeric {

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

enum struct Sign : u8
{
  Nil = 0b00,
  Pos = 0b01,
  Neg = 0b10,
  Var = 0b11,
};

using Bits = u8;

namespace internal {
/// representative helper
template<Sign S, Bits B>
constexpr auto
repr_helper_()
{
  constexpr auto S_BIT = static_cast<Bits>(S == Sign::Var);
  constexpr auto BYTES = 7U + B + S_BIT >> 3U;
  static_assert(BYTES <= 8U, "no supported int type can hold the requested amount of bits");
  if constexpr (BYTES > 4U && S_BIT)
    return i64{};
  else if constexpr (BYTES > 4U)
    return u64{};
  else if constexpr (BYTES > 2U && S_BIT)
    return i32{};
  else if constexpr (BYTES > 2U)
    return u32{};
  else if constexpr (BYTES > 1U && S_BIT)
    return i16{};
  else if constexpr (BYTES > 1U)
    return u16{};
  else if constexpr (S_BIT)
    return i8{};
  else
    return u8{};
}
}

template<Sign S, Bits B>
using Repr = decltype(internal::repr_helper_<S, B>());

// clang-format off
template<typename T>
concept IsRepr = std::is_same_v<T, u8>  || std::is_same_v<T, i8>
              || std::is_same_v<T, u16> || std::is_same_v<T, i16>
              || std::is_same_v<T, u32> || std::is_same_v<T, i32>
              || std::is_same_v<T, u64> || std::is_same_v<T, i64>;
// clang-format on

enum struct CheckPolicy : u8
{
  Unsafe = 0,    //< assume defined behaviour (fast)
  Safe = 1,      //< fail program on undefined behaviour (slow)
  SilentFix = 2, //< silently enforce defined stated by rounding (fast, not recommended)
};

template<Sign S, Bits B, CheckPolicy C>
Repr<S, B>
repr_cast(IsRepr auto repr_) noexcept(C == CheckPolicy::Unsafe)
{
  constexpr auto REPR_SIGN = static_cast<Bits>(std::is_signed_v<decltype(repr_)>);
  constexpr auto REPR_BITS = static_cast<Bits>((sizeof(repr_) << 3) - REPR_SIGN);
  constexpr auto REPR_SIGN_MASK = static_cast<u64>(REPR_SIGN) << REPR_BITS;

  constexpr auto ONE_MASK = static_cast<u64>(1);
  constexpr auto SIGN_MASK = static_cast<u64>(std::is_signed_v<decltype(repr_)>) << 63;
  constexpr auto BITS_MASK = ((ONE_MASK << B) - ONE_MASK) | SIGN_MASK;

  if constexpr (C == CheckPolicy::Unsafe) {
    // unsafe strategy assumes consistency
    return static_cast<Repr<S, B>>(repr_);

  } else if constexpr (C == CheckPolicy::Safe) {
    // safe strategy checks for consistency

    switch (S) {
      case Sign::Nil:
        ASSERT_EQ(repr_, 0, "Nil Sign implies zero value");
        return 0;

      case Sign::Pos:
      case Sign::Neg:
        if (REPR_SIGN)
          ASSERT_EQ(repr_ & REPR_SIGN_MASK, 0, "Pos and Neg Sign are implicit");

        ASSERT_EQ(repr_ & ~BITS_MASK, 0);
        return static_cast<Repr<S, B>>(repr_);

      case Sign::Var:
        // TODO ...
        const auto bits = static_cast<Repr<S, B>>(repr_ & ~REPR_SIGN_MASK);
        if (repr_ & REPR_SIGN_MASK)
          return -static_cast<Repr<S, B>>(bits);
        return static_cast<Repr<S, B>>(bits);
    }

  } else {

    switch (S) {
      case Sign::Nil:
        return 0;

      case Sign::Pos:
      case Sign::Neg:
        // discard sign if present
        return static_cast<Repr<S, B>>(repr_ & ~REPR_SIGN_MASK);

      case Sign::Var:
        const auto bits = static_cast<Repr<S, B>>(repr_ & ~REPR_SIGN_MASK);
        if (static_cast<Repr<S, B>>(repr_ & REPR_SIGN_MASK))
          return -static_cast<Repr<S, B>>(bits);
        return static_cast<Repr<S, B>>(bits);
    }

    if (S == Sign::Nil)
      return 0;

    if (S != Sign::Var && REPR_SIGN)
      // discard sign bit
      repr_ = repr_ & ~REPR_SIGN_MASK;
  }

  constexpr auto ONE = static_cast<u64>(1);
  constexpr auto SIGN = static_cast<u64>(std::is_signed_v<decltype(repr_)>) << 63;
  constexpr auto MASK = ((ONE << B) - ONE) | SIGN;
  return (repr_ & ~MASK) == 0;
}

}
