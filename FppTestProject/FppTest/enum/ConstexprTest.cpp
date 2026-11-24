// ======================================================================
// \title  ConstexprTest.cpp
// \brief  Compile-time tests for constexpr enum classes
// ======================================================================

#include "FppTest/enum/ImplicitEnumAc.hpp"

#include "gtest/gtest.h"

namespace FppTest {
namespace Enum {

constexpr Implicit defaultValue;
constexpr Implicit rawValue(Implicit::C);
constexpr Implicit serialValue(static_cast<Implicit::SerialType>(Implicit::D));
constexpr Implicit copyValue(rawValue);

static_assert(defaultValue == Implicit::A, "default constructor");
static_assert(rawValue == Implicit::C, "raw enum constructor");
static_assert(serialValue == Implicit::D, "serial type constructor");
static_assert(copyValue == Implicit::C, "copy constructor");
static_assert(rawValue != Implicit::A, "inequality operator");
static_assert(Implicit::isValid(static_cast<Implicit::SerialType>(Implicit::E)), "isValid");
static_assert(static_cast<Implicit::T>(rawValue) == Implicit::C, "conversion operator");

// Constant enums match their runtime-constructed equivalents
TEST(ConstexprTest, Enum) {
    Implicit runtimeValue(Implicit::C);
    ASSERT_EQ(rawValue, runtimeValue);
    ASSERT_EQ(copyValue, runtimeValue);
}

}  // namespace Enum
}  // namespace FppTest
