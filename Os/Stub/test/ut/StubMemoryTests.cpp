// ======================================================================
// \title Os/Stub/test/ut/StubMemoryTests.cpp
// \brief tests using stub implementation for Os::Memory interface testing
// ======================================================================
#include <gtest/gtest.h>
#include "Os/Memory.hpp"
#include "Os/Stub/test/Memory.hpp"
#include "STest/Pick/Pick.hpp"

// Wrapper under test: Os::MemoryCommon on the implementation named here, not the selection in config/OsSelection.hpp
using MemoryUnderTest = Os::MemoryCommon<Os::Stub::Memory::Test::TestMemory>;

TEST(Interface, Construction) {
    MemoryUnderTest memory;
    ASSERT_EQ(Os::Stub::Memory::Test::StaticData::data.lastCalled, Os::Stub::Memory::Test::StaticData::CONSTRUCT_FN);
}

TEST(Interface, Destruction) {
    delete (new MemoryUnderTest);
    ASSERT_EQ(Os::Stub::Memory::Test::StaticData::data.lastCalled, Os::Stub::Memory::Test::StaticData::DESTRUCT_FN);
}

TEST(Interface, Usage) {
    MemoryUnderTest memory;
    MemoryUnderTest::Usage usage;
    usage.used = STest::Pick::lowerUpper(0, 10000);
    usage.total = STest::Pick::lowerUpper(0, 10000);
    MemoryUnderTest::Usage usage_copy = usage;
    Os::MemoryInterface::Status status = Os::MemoryInterface::Status::ERROR;
    Os::Stub::Memory::Test::StaticData::data.status_out = status;
    ASSERT_EQ(Os::Stub::Memory::Test::StaticData::data.lastCalled, Os::Stub::Memory::Test::StaticData::CONSTRUCT_FN);
    ASSERT_EQ(MemoryUnderTest::getUsage(usage), status);
    ASSERT_EQ(Os::Stub::Memory::Test::StaticData::data.lastCalled, Os::Stub::Memory::Test::StaticData::USAGE_FN);
    ASSERT_EQ(Os::Stub::Memory::Test::StaticData::data.usage.total, usage.total);
    ASSERT_EQ(Os::Stub::Memory::Test::StaticData::data.usage.used, usage.used);
    ASSERT_EQ(usage_copy.total, usage.total);
    ASSERT_EQ(usage_copy.used, usage.used);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
