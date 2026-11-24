// ======================================================================
// \title Os/Stub/test/ut/StubMemoryTests.cpp
// \brief interface tests for Os::Memory: the wrapper calls its implementation as expected
//
// Runs on the selected Os::Memory with a recording test interceptor in place of the implementation's behavior.
// ======================================================================
#include <gtest/gtest.h>
#include "Os/Memory.hpp"
#include "Os/test/ut/interceptors/MemoryTestInterceptor.hpp"
#include "STest/Pick/Pick.hpp"

namespace {

//! Records the calls Os::Memory makes on its implementation and returns the injected status
class Recorder : public Os::Test::MemoryTestInterceptor {
  public:
    enum LastFn { NONE_FN, CONSTRUCT_FN, DESTRUCT_FN, USAGE_FN };

    void constructed(Os::MemoryInterface& real) override { this->lastCalled = CONSTRUCT_FN; }
    void destructed(Os::MemoryInterface& real) override { this->lastCalled = DESTRUCT_FN; }
    Status _getUsage(Os::MemoryInterface& real, Usage& memory_usage) override {
        this->lastCalled = USAGE_FN;
        this->usage = memory_usage;
        return this->status_out;
    }

    LastFn lastCalled = NONE_FN;        //!< Last function called
    Usage usage = {0, 0};               //!< Last usage passed
    Status status_out = Status::OP_OK;  //!< Status to return
};

class Interface : public ::testing::Test {
  public:
    Recorder recorder;  //!< Installed for the duration of each test
};

}  // namespace

TEST_F(Interface, Construction) {
    Os::Memory memory;
    ASSERT_EQ(recorder.lastCalled, Recorder::CONSTRUCT_FN);
}

TEST_F(Interface, Destruction) {
    delete (new Os::Memory);
    ASSERT_EQ(recorder.lastCalled, Recorder::DESTRUCT_FN);
}

TEST_F(Interface, Usage) {
    Os::Memory memory;
    Os::Memory::Usage usage;
    usage.used = STest::Pick::lowerUpper(0, 10000);
    usage.total = STest::Pick::lowerUpper(0, 10000);
    Os::Memory::Usage usage_copy = usage;
    Os::Memory::Status status = Os::Memory::Status::ERROR;
    recorder.status_out = status;
    ASSERT_EQ(recorder.lastCalled, Recorder::CONSTRUCT_FN);
    ASSERT_EQ(memory.getUsage(usage), status);
    ASSERT_EQ(recorder.lastCalled, Recorder::USAGE_FN);
    ASSERT_EQ(recorder.usage.total, usage.total);
    ASSERT_EQ(recorder.usage.used, usage.used);
    ASSERT_EQ(usage_copy.total, usage.total);
    ASSERT_EQ(usage_copy.used, usage.used);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
