// ======================================================================
// \title Os/Stub/test/ut/StubFileTests.cpp
// \brief tests using stub implementation for Os::File interface testing
// ======================================================================
#include <gtest/gtest.h>
#include "Os/Console.hpp"
#include "Os/Stub/test/Console.hpp"

// Wrapper under test: Os::ConsoleCommon on the implementation named here, not the selection in config/OsSelection.hpp
using ConsoleUnderTest = Os::ConsoleCommon<Os::Stub::Console::Test::TestConsole>;

TEST(Interface, Construction) {
    ConsoleUnderTest console;
    ASSERT_EQ(Os::Stub::Console::Test::StaticData::data.lastCalled, Os::Stub::Console::Test::StaticData::CONSTRUCT_FN);
}
TEST(Interface, ConstructionCopy) {
    ConsoleUnderTest console;
    ASSERT_EQ(Os::Stub::Console::Test::StaticData::data.lastCalled, Os::Stub::Console::Test::StaticData::CONSTRUCT_FN);
    ConsoleUnderTest console2(console);
    ASSERT_EQ(const_cast<Os::Stub::Console::Test::TestConsole*>(Os::Stub::Console::Test::StaticData::data.copyObject)
                  ->getHandle(),
              console.getHandle());
    ASSERT_EQ(Os::Stub::Console::Test::StaticData::data.lastCalled,
              Os::Stub::Console::Test::StaticData::CONSTRUCT_COPY_FN);
}
TEST(Interface, Copy) {
    ConsoleUnderTest console;
    ConsoleUnderTest console2;
    ASSERT_EQ(Os::Stub::Console::Test::StaticData::data.lastCalled, Os::Stub::Console::Test::StaticData::CONSTRUCT_FN);
    console2 = console;
    ASSERT_EQ(const_cast<Os::Stub::Console::Test::TestConsole*>(Os::Stub::Console::Test::StaticData::data.copyObject)
                  ->getHandle(),
              console.getHandle());
    ASSERT_EQ(Os::Stub::Console::Test::StaticData::data.lastCalled,
              Os::Stub::Console::Test::StaticData::CONSTRUCT_COPY_FN);
}

TEST(Interface, Destruction) {
    delete (new ConsoleUnderTest);
    ASSERT_EQ(Os::Stub::Console::Test::StaticData::data.lastCalled, Os::Stub::Console::Test::StaticData::DESTRUCT_FN);
}

TEST(Interface, Write) {
    ConsoleUnderTest console;
    const char* message = "hello";
    ASSERT_EQ(Os::Stub::Console::Test::StaticData::data.lastCalled, Os::Stub::Console::Test::StaticData::CONSTRUCT_FN);
    console.write(message, 6);
    ASSERT_EQ(Os::Stub::Console::Test::StaticData::data.lastCalled, Os::Stub::Console::Test::StaticData::WRITE_FN);
    ASSERT_EQ(Os::Stub::Console::Test::StaticData::data.message, message);
    ASSERT_EQ(Os::Stub::Console::Test::StaticData::data.size, 6);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
