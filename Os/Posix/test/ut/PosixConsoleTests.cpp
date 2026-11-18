// ======================================================================
// \title Os/Posix/test/ut/PosixConsoleTests.cpp
// \brief tests for posix implementation for Os::Console
// ======================================================================
#include <gtest/gtest.h>
#include <cstdio>
#include <string>
#include "Fw/Logger/Logger.hpp"
#include "Fw/Types/String.hpp"
#include "Os/Console.hpp"
#include "Os/Posix/Console.hpp"

TEST(Nominal, SwitchStream) {
    Os::Posix::Console::PosixConsole posix_console;
    ASSERT_EQ(reinterpret_cast<Os::Posix::Console::PosixConsoleHandle*>(posix_console.getHandle())->m_file_descriptor,
              stdout);
    posix_console.setOutputStream(Os::Posix::Console::PosixConsole::Stream::STANDARD_ERROR);
    ASSERT_EQ(reinterpret_cast<Os::Posix::Console::PosixConsoleHandle*>(posix_console.getHandle())->m_file_descriptor,
              stderr);
    posix_console.setOutputStream(Os::Posix::Console::PosixConsole::Stream::STANDARD_OUT);
    ASSERT_EQ(reinterpret_cast<Os::Posix::Console::PosixConsoleHandle*>(posix_console.getHandle())->m_file_descriptor,
              stdout);
}
TEST(OffNominal, SwitchStream) {
    Os::Posix::Console::PosixConsole posix_console;
    // intentional death test
    ASSERT_DEATH(posix_console.setOutputStream(static_cast<  // NOLINT(clang-analyzer-optin.core.EnumCastOutOfRange)
                                               Os::Posix::Console::PosixConsole::Stream>(3)),
                 "Assert: .*Console\\.cpp");
}

// Redirect the console under test to a temporary file and return that file
static FILE* redirectToTemporaryFile(Os::Posix::Console::PosixConsole& posix_console) {
    FILE* capture = ::tmpfile();
    EXPECT_NE(capture, nullptr);
    static_cast<Os::Posix::Console::PosixConsoleHandle*>(posix_console.getHandle())->m_file_descriptor = capture;
    return capture;
}

// Read back everything written to the capture file and close it
static std::string readBackAndClose(FILE* capture) {
    std::string output;
    ::rewind(capture);
    for (int character = ::fgetc(capture); character != EOF; character = ::fgetc(capture)) {
        output.push_back(static_cast<char>(character));
    }
    (void)::fclose(capture);
    return output;
}

// The raw write is bounded by the supplied size, not by the terminator
TEST(Nominal, WriteMessageRaw) {
    Os::Posix::Console::PosixConsole posix_console;
    FILE* capture = redirectToTemporaryFile(posix_console);
    ASSERT_NE(capture, nullptr);
    posix_console.writeMessage("hello world", 5);
    ASSERT_EQ(readBackAndClose(capture), "hello");
}

// The ConstStringBase overload supplied by Os::ConsoleInterface reaches the Posix implementation directly
TEST(Nominal, WriteMessageString) {
    Os::Posix::Console::PosixConsole posix_console;
    FILE* capture = redirectToTemporaryFile(posix_console);
    ASSERT_NE(capture, nullptr);
    const Fw::String message("direct string\n");
    posix_console.writeMessage(message);
    ASSERT_EQ(readBackAndClose(capture), "direct string\n");
}

// A PosixConsole is usable as the Fw::Logger without any wrapping class
TEST(Nominal, WriteThroughLogger) {
    Os::Posix::Console::PosixConsole posix_console;
    FILE* capture = redirectToTemporaryFile(posix_console);
    ASSERT_NE(capture, nullptr);
    Fw::Logger::registerLogger(&posix_console);
    Fw::Logger::log("value %d\n", 42);
    Fw::Logger::registerLogger(nullptr);
    ASSERT_EQ(readBackAndClose(capture), "value 42\n");
}

// A zero-sized write is permitted with a nullptr message and writes nothing
TEST(Nominal, WriteMessageEmpty) {
    Os::Posix::Console::PosixConsole posix_console;
    FILE* capture = redirectToTemporaryFile(posix_console);
    ASSERT_NE(capture, nullptr);
    posix_console.writeMessage(nullptr, 0);
    ASSERT_EQ(readBackAndClose(capture), "");
}

// A nullptr message with a non-zero size is a coding error
TEST(OffNominal, WriteMessageNull) {
    Os::Posix::Console::PosixConsole posix_console;
    ASSERT_DEATH(posix_console.writeMessage(nullptr, 5), "Assert: ");
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
