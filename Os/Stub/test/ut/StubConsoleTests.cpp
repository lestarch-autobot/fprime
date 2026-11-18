// ======================================================================
// \title Os/Stub/test/ut/StubConsoleTests.cpp
// \brief tests using stub implementation for Os::Console interface testing
// ======================================================================
#include <gtest/gtest.h>
#include "Fw/Types/String.hpp"
#include "Os/Console.hpp"
#include "Os/Stub/test/Console.hpp"

TEST(Interface, Construction) {
    Os::Console console;
    ASSERT_EQ(Os::Stub::Console::Test::StaticData::data.lastCalled, Os::Stub::Console::Test::StaticData::CONSTRUCT_FN);
}
TEST(Interface, ConstructionCopy) {
    Os::Console console;
    ASSERT_EQ(Os::Stub::Console::Test::StaticData::data.lastCalled, Os::Stub::Console::Test::StaticData::CONSTRUCT_FN);
    Os::Console console2(console);
    ASSERT_EQ(const_cast<Os::Stub::Console::Test::TestConsole*>(Os::Stub::Console::Test::StaticData::data.copyObject)
                  ->getHandle(),
              console.getHandle());
    ASSERT_EQ(Os::Stub::Console::Test::StaticData::data.lastCalled,
              Os::Stub::Console::Test::StaticData::CONSTRUCT_COPY_FN);
}
TEST(Interface, Copy) {
    Os::Console console;
    Os::Console console2;
    ASSERT_EQ(Os::Stub::Console::Test::StaticData::data.lastCalled, Os::Stub::Console::Test::StaticData::CONSTRUCT_FN);
    console2 = console;
    ASSERT_EQ(const_cast<Os::Stub::Console::Test::TestConsole*>(Os::Stub::Console::Test::StaticData::data.copyObject)
                  ->getHandle(),
              console.getHandle());
    ASSERT_EQ(Os::Stub::Console::Test::StaticData::data.lastCalled,
              Os::Stub::Console::Test::StaticData::CONSTRUCT_COPY_FN);
}

TEST(Interface, Destruction) {
    delete (new Os::Console);
    ASSERT_EQ(Os::Stub::Console::Test::StaticData::data.lastCalled, Os::Stub::Console::Test::StaticData::DESTRUCT_FN);
}

TEST(Interface, Write) {
    Os::Console console;
    const char* message = "hello";
    ASSERT_EQ(Os::Stub::Console::Test::StaticData::data.lastCalled, Os::Stub::Console::Test::StaticData::CONSTRUCT_FN);
    console.write(message, 6);
    ASSERT_EQ(Os::Stub::Console::Test::StaticData::data.lastCalled, Os::Stub::Console::Test::StaticData::WRITE_FN);
    ASSERT_EQ(Os::Stub::Console::Test::StaticData::data.message, message);
    ASSERT_EQ(Os::Stub::Console::Test::StaticData::data.size, 6);
}

// The ConstStringBase overload supplied by Os::ConsoleInterface reaches the delegate implementation
TEST(Interface, WriteMessageString) {
    Os::Console console;
    const Fw::String message("hello");
    console.writeMessage(message);
    ASSERT_EQ(Os::Stub::Console::Test::StaticData::data.lastCalled, Os::Stub::Console::Test::StaticData::WRITE_FN);
    ASSERT_EQ(Os::Stub::Console::Test::StaticData::data.message, message.toChar());
    ASSERT_EQ(Os::Stub::Console::Test::StaticData::data.size, 5);
}

// A nullptr message with a non-zero size is a coding error caught before reaching the delegate
TEST(Interface, WriteMessageNull) {
    Os::Console console;
    ASSERT_DEATH(console.writeMessage(nullptr, 5), "Assert: ");
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
