// ======================================================================
// \title Os/Stub/test/ut/StubMemoryTests.cpp
// \brief tests using stub implementation for Os::Memory interface testing
// ======================================================================
#include <gtest/gtest.h>
#include "Os/Memory.hpp"

TEST(Basic, Usage) {
    Os::Memory::Usage usage_output;
    Os::Memory::getUsage(usage_output);

    ASSERT_GT(usage_output.used, 0);
    ASSERT_GT(usage_output.total, 0);
}

TEST(Basic, Singleton) {
    Os::Memory::init();
    Os::Memory& singleton = Os::Memory::getSingleton();
    ASSERT_EQ(&singleton, &Os::Memory::getSingleton());

    Os::Memory::Usage usage_output;
    ASSERT_EQ(singleton._getUsage(usage_output), Os::Memory::Status::OP_OK);
    ASSERT_GT(usage_output.total, 0);
    ASSERT_NE(singleton.getHandle(), nullptr);
}
