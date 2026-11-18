#include <gtest/gtest.h>
#include <Os/Condition.hpp>
#include <Os/Console.hpp>
#include <Os/Cpu.hpp>
#include <Os/Directory.hpp>
#include <Os/File.hpp>
#include <Os/FileSystem.hpp>
#include <Os/Memory.hpp>
#include <Os/Mutex.hpp>
#include <Os/Queue.hpp>
#include <Os/RawTime.hpp>
#include <Os/Stub/ConditionVariable.hpp>
#include <Os/Task.hpp>

TEST(Stub, File) {
    Os::File _;
}

TEST(Stub, Task) {
    Os::Task _;
}

TEST(Stub, Queue) {
    Os::Queue _;
}

TEST(Stub, ConditionVariable) {
    Os::ConditionVariable _;
}

// The stub implementation itself rejects a second mutex, whether or not it is reached through the delegate
TEST(Stub, ConditionVariableDifferentMutex) {
    Os::Mutex first;
    Os::Mutex second;
    Os::Stub::Mutex::StubConditionVariable variable;
    ASSERT_EQ(variable.pend(first), Os::ConditionVariableInterface::Status::ERROR_NOT_IMPLEMENTED);
    ASSERT_EQ(variable.pend(second), Os::ConditionVariableInterface::Status::ERROR_DIFFERENT_MUTEX);
    ASSERT_EQ(variable.pend(first), Os::ConditionVariableInterface::Status::ERROR_NOT_IMPLEMENTED);
}

TEST(Stub, Mutex) {
    Os::Mutex _;
}

TEST(Stub, Console) {
    Os::Console _;
}

TEST(Stub, FileSystem) {
    Os::FileSystem::getSingleton();
}

TEST(Stub, Directory) {
    Os::Directory _;
}

TEST(Stub, Cpu) {
    Os::Cpu::getSingleton();
}

TEST(Stub, Memory) {
    Os::Memory::getSingleton();
}

TEST(Stub, RawTime) {
    Os::RawTime _;
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
