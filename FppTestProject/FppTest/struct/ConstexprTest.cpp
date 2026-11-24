// ======================================================================
// \title  ConstexprTest.cpp
// \brief  Compile-time tests for constexpr struct classes
// ======================================================================

#include "FppTest/struct/EmptySerializableAc.hpp"
#include "FppTest/struct/PrimitiveSerializableAc.hpp"

#include "gtest/gtest.h"

namespace FppTest {
namespace Struct {

constexpr Empty emptyValue;
constexpr Primitive defaultValue;
constexpr Primitive memberValue(true, 1, 2, 3.0);
constexpr Primitive copyValue(memberValue);

static_assert(!defaultValue.get_mBool() && defaultValue.get_mU32() == 0, "default constructor");
static_assert(memberValue.get_mBool() && memberValue.get_mU32() == 1, "member constructor");
static_assert(memberValue.get_mI16() == 2 && memberValue.get_mF64() > 2.5, "member constructor");
static_assert(copyValue.get_mU32() == 1 && copyValue.get_mI16() == 2, "copy constructor");

// Constant structs match their runtime-constructed equivalents
TEST(ConstexprTest, Struct) {
    Empty runtimeEmpty;
    ASSERT_EQ(emptyValue, runtimeEmpty);
    Primitive runtimeValue(true, 1, 2, 3.0);
    ASSERT_EQ(memberValue, runtimeValue);
    ASSERT_EQ(copyValue, runtimeValue);
}

}  // namespace Struct
}  // namespace FppTest
