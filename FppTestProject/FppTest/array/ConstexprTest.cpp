// ======================================================================
// \title  ConstexprTest.cpp
// \brief  Compile-time tests for constexpr array classes
// ======================================================================

#include "FppTest/array/EnumArrayAc.hpp"
#include "FppTest/array/StructArrayAc.hpp"
#include "FppTest/array/Uint32ArrayAc.hpp"
#include "FppTest/array/Uint32ArrayArrayAc.hpp"

#include "gtest/gtest.h"

namespace FppTest {
namespace Array {

constexpr Uint32 defaultValue;
constexpr Uint32 listValue = {1, 2};
constexpr Uint32 singleValue(3);
constexpr Uint32 copyValue(listValue);
constexpr U32 primitiveArray[Uint32::SIZE] = {4, 5};
constexpr Uint32 primitiveValue(primitiveArray);
constexpr Uint32Array nestedValue(listValue);
constexpr Enum enumValue;
constexpr Struct structValue(S(6, 7));

static_assert(defaultValue[0] == 0 && defaultValue[1] == 0, "default constructor");
static_assert(listValue[0] == 1 && listValue[1] == 2, "initializer list constructor");
static_assert(singleValue[0] == 3 && singleValue[1] == 3, "single element constructor");
static_assert(copyValue[0] == 1 && copyValue[1] == 2, "copy constructor");
static_assert(primitiveValue[0] == 4 && primitiveValue[1] == 5, "primitive array constructor");
static_assert(nestedValue[2][1] == 2, "array of arrays");
static_assert(enumValue[0] == E::A && enumValue[2] == E::C, "array of enums with default value");
static_assert(structValue[2].get_mU32() == 6 && structValue[2].get_mU32Arr()[2] == 7, "array of structs");

// Constant arrays match their runtime-constructed equivalents
TEST(ConstexprTest, Array) {
    Uint32 runtimeValue = {1, 2};
    ASSERT_EQ(listValue, runtimeValue);
    Enum runtimeEnum;
    ASSERT_EQ(enumValue, runtimeEnum);
    Struct runtimeStruct(S(6, 7));
    ASSERT_EQ(structValue, runtimeStruct);
}

}  // namespace Array
}  // namespace FppTest
