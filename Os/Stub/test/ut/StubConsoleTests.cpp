// ======================================================================
// \title Os/Stub/test/ut/StubFileTests.cpp
// \brief tests using stub implementation for Os::File interface testing
// ======================================================================
#include <gtest/gtest.h>
#include "Fw/Logger/Logger.hpp"
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

TEST(Interface, WriteNullMessage) {
    Os::Console console;
    console.writeMessage(nullptr, 0);
    ASSERT_EQ(Os::Stub::Console::Test::StaticData::data.lastCalled, Os::Stub::Console::Test::StaticData::WRITE_FN);
    // intentional death test
    ASSERT_DEATH(console.writeMessage(nullptr, 1), "Os/Stub/test/Console\\.cpp");
}

TEST(Interface, SingletonWrite) {
    const char* message = "hello";
    Os::Console::init();
    Os::Console::write(message, 6);
    ASSERT_EQ(Os::Stub::Console::Test::StaticData::data.lastCalled, Os::Stub::Console::Test::StaticData::WRITE_FN);
    ASSERT_EQ(Os::Stub::Console::Test::StaticData::data.message, message);
    ASSERT_EQ(Os::Stub::Console::Test::StaticData::data.size, 6);
    const Fw::String string("string");
    Os::Console::write(string);
    ASSERT_EQ(Os::Stub::Console::Test::StaticData::data.message, string.toChar());
    ASSERT_EQ(Os::Stub::Console::Test::StaticData::data.size, string.length());
    Os::Console::getSingleton().writeMessage(message, 5);
    ASSERT_EQ(Os::Stub::Console::Test::StaticData::data.message, message);
    ASSERT_EQ(Os::Stub::Console::Test::StaticData::data.size, 5);
}

TEST(Interface, SingletonRegistersLogger) {
    const Fw::String logged("logged");
    Os::Console::init();
    Os::Stub::Console::Test::StaticData::data.lastCalled = Os::Stub::Console::Test::StaticData::NONE_FN;
    Fw::Logger::log(logged);
    ASSERT_EQ(Os::Stub::Console::Test::StaticData::data.lastCalled, Os::Stub::Console::Test::StaticData::WRITE_FN);
    ASSERT_EQ(Os::Stub::Console::Test::StaticData::data.message, logged.toChar());
    ASSERT_EQ(Os::Stub::Console::Test::StaticData::data.size, logged.length());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST(Interface, SingletonRegistersLoggerOnce) {
    class OtherLogger : public Fw::Logger {
      public:
        bool m_called = false;
        void writeMessage(const Fw::ConstStringBase& message) override { this->m_called = true; }
    } other;
    Os::Console::init();
    Fw::Logger::registerLogger(&other);
    // Singleton access must not replace the logger registered after first use
    Os::Console::write("x", 1);
    Os::Stub::Console::Test::StaticData::data.lastCalled = Os::Stub::Console::Test::StaticData::NONE_FN;
    Fw::Logger::log(Fw::String("y"));
    ASSERT_TRUE(other.m_called);
    ASSERT_EQ(Os::Stub::Console::Test::StaticData::data.lastCalled, Os::Stub::Console::Test::StaticData::NONE_FN);
    Fw::Logger::registerLogger(&Os::Console::getSingleton());
}
