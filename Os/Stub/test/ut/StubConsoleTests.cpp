// ======================================================================
// \title Os/Stub/test/ut/StubConsoleTests.cpp
// \brief interface tests for Os::Console: the wrapper calls its implementation as expected
//
// Runs on the selected Os::Console with a recording test interceptor in place of the implementation's behavior.
// ======================================================================
#include <gtest/gtest.h>
#include "Os/Console.hpp"
#include "Os/test/ut/interceptors/ConsoleTestInterceptor.hpp"

namespace {

//! Records the calls Os::Console makes on its implementation; nothing is written to the real console
class Recorder : public Os::Test::ConsoleTestInterceptor {
  public:
    enum LastFn { NONE_FN, CONSTRUCT_FN, CONSTRUCT_COPY_FN, DESTRUCT_FN, WRITE_FN };

    void constructed(Os::ConsoleInterface& real) override { this->lastCalled = CONSTRUCT_FN; }
    void destructed(Os::ConsoleInterface& real) override { this->lastCalled = DESTRUCT_FN; }
    void copyConstructed(Os::ConsoleInterface& real, const Os::ConsoleInterface& other) override {
        this->lastCalled = CONSTRUCT_COPY_FN;
        this->copyObject = &other;
    }
    void copyAssigned(Os::ConsoleInterface& real, const Os::ConsoleInterface& other) override {
        this->lastCalled = CONSTRUCT_COPY_FN;
        this->copyObject = &other;
    }
    void writeMessage(Os::ConsoleInterface& real, const CHAR* message_in, const FwSizeType size_in) override {
        this->message = message_in;
        this->size = size_in;
        this->lastCalled = WRITE_FN;
    }

    LastFn lastCalled = NONE_FN;                       //!< Last function called
    const Os::ConsoleInterface* copyObject = nullptr;  //!< Source of the last copy
    const CHAR* message = nullptr;                     //!< Last message passed
    FwSizeType size = 0;                               //!< Last size passed
};

class Interface : public ::testing::Test {
  public:
    Recorder recorder;  //!< Installed for the duration of each test
};

}  // namespace

TEST_F(Interface, Construction) {
    Os::Console console;
    ASSERT_EQ(recorder.lastCalled, Recorder::CONSTRUCT_FN);
}

TEST_F(Interface, ConstructionCopy) {
    Os::Console console;
    ASSERT_EQ(recorder.lastCalled, Recorder::CONSTRUCT_FN);
    Os::Console console2(console);
    ASSERT_EQ(recorder.lastCalled, Recorder::CONSTRUCT_COPY_FN);
    ASSERT_EQ(const_cast<Os::ConsoleInterface*>(recorder.copyObject)->getHandle(), console.getHandle());
}

TEST_F(Interface, Copy) {
    Os::Console console;
    Os::Console console2;
    ASSERT_EQ(recorder.lastCalled, Recorder::CONSTRUCT_FN);
    console2 = console;
    ASSERT_EQ(recorder.lastCalled, Recorder::CONSTRUCT_COPY_FN);
    ASSERT_EQ(const_cast<Os::ConsoleInterface*>(recorder.copyObject)->getHandle(), console.getHandle());
}

TEST_F(Interface, Destruction) {
    delete (new Os::Console);
    ASSERT_EQ(recorder.lastCalled, Recorder::DESTRUCT_FN);
}

TEST_F(Interface, Write) {
    Os::Console console;
    const char* message = "hello";
    ASSERT_EQ(recorder.lastCalled, Recorder::CONSTRUCT_FN);
    console.write(message, 6);
    ASSERT_EQ(recorder.lastCalled, Recorder::WRITE_FN);
    ASSERT_EQ(recorder.message, message);
    ASSERT_EQ(recorder.size, 6);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
