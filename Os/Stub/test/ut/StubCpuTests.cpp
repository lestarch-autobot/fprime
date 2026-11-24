// ======================================================================
// \title Os/Stub/test/ut/StubCpuTests.cpp
// \brief interface tests for Os::Cpu: the wrapper calls its implementation as expected
//
// Runs on the selected Os::Cpu with a recording test interceptor in place of the implementation's behavior.
// ======================================================================
#include <gtest/gtest.h>
#include "Os/Cpu.hpp"
#include "Os/test/ut/interceptors/CpuTestInterceptor.hpp"
#include "STest/Pick/Pick.hpp"

namespace {

//! Records the calls Os::Cpu makes on its implementation and returns the injected status
class Recorder : public Os::Test::CpuTestInterceptor {
  public:
    enum LastFn { NONE_FN, CONSTRUCT_FN, DESTRUCT_FN, COUNT_FN, TICKS_FN };

    void constructed(Os::CpuInterface& real) override { this->lastCalled = CONSTRUCT_FN; }
    void destructed(Os::CpuInterface& real) override { this->lastCalled = DESTRUCT_FN; }
    Status _getCount(Os::CpuInterface& real, FwSizeType& cpu_count) override {
        this->lastCalled = COUNT_FN;
        this->count = cpu_count;
        return this->status_out;
    }
    Status _getTicks(Os::CpuInterface& real, Ticks& ticks_in, FwSizeType cpu_index) override {
        this->lastCalled = TICKS_FN;
        this->ticks = ticks_in;
        this->index = cpu_index;
        return this->status_out;
    }

    LastFn lastCalled = NONE_FN;        //!< Last function called
    FwSizeType count = 0;               //!< Last count passed
    FwSizeType index = 0;               //!< Last index passed
    Ticks ticks = {0, 0};               //!< Last ticks passed
    Status status_out = Status::OP_OK;  //!< Status to return
};

class Interface : public ::testing::Test {
  public:
    Recorder recorder;  //!< Installed for the duration of each test
};

}  // namespace

TEST_F(Interface, Construction) {
    Os::Cpu cpu;
    ASSERT_EQ(recorder.lastCalled, Recorder::CONSTRUCT_FN);
}

TEST_F(Interface, Destruction) {
    delete (new Os::Cpu);
    ASSERT_EQ(recorder.lastCalled, Recorder::DESTRUCT_FN);
}

TEST_F(Interface, Count) {
    Os::Cpu cpu;
    FwSizeType count = STest::Pick::lowerUpper(0, 10000);
    FwSizeType count_copy = count;
    Os::Cpu::Status status = Os::Cpu::Status::ERROR;
    recorder.status_out = status;
    ASSERT_EQ(recorder.lastCalled, Recorder::CONSTRUCT_FN);
    ASSERT_EQ(cpu.getCount(count), status);
    ASSERT_EQ(recorder.lastCalled, Recorder::COUNT_FN);
    ASSERT_EQ(recorder.count, count_copy);
    ASSERT_EQ(count, count_copy);
}

TEST_F(Interface, Ticks) {
    Os::Cpu cpu;
    FwSizeType index = STest::Pick::lowerUpper(0, 10000);
    Os::Cpu::Ticks ticks;
    ticks.used = STest::Pick::lowerUpper(0, 10000);
    ticks.total = STest::Pick::lowerUpper(0, 10000);
    Os::Cpu::Ticks ticks_copy = ticks;
    Os::Cpu::Status status = Os::Cpu::Status::ERROR;
    recorder.status_out = status;
    ASSERT_EQ(recorder.lastCalled, Recorder::CONSTRUCT_FN);
    ASSERT_EQ(cpu.getTicks(ticks, index), status);
    ASSERT_EQ(recorder.lastCalled, Recorder::TICKS_FN);
    ASSERT_EQ(recorder.index, index);
    ASSERT_EQ(recorder.ticks.total, ticks.total);
    ASSERT_EQ(recorder.ticks.used, ticks.used);
    ASSERT_EQ(ticks_copy.total, ticks.total);
    ASSERT_EQ(ticks_copy.used, ticks.used);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
