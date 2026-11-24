// ======================================================================
// \title Os/Stub/test/ut/StubRawTimeTests.cpp
// \brief interface tests for Os::RawTime: the wrapper calls its implementation as expected
//
// Runs on the selected Os::RawTime with a recording test interceptor in place of the implementation's behavior.
// ======================================================================
#include <gtest/gtest.h>
#include "Fw/Buffer/Buffer.hpp"
#include "Fw/Types/Serializable.hpp"
#include "Os/RawTime.hpp"
#include "Os/test/ut/interceptors/RawTimeTestInterceptor.hpp"

namespace {

//! Records the calls Os::RawTime makes on its implementation; the real time source is never read
class Recorder : public Os::Test::RawTimeTestInterceptor {
  public:
    enum LastFn {
        NONE_FN,
        CONSTRUCT_FN,
        COPY_CONSTRUCT_FN,
        COPY_ASSIGN_FN,
        DESTRUCT_FN,
        GET_TIME_FN,
        GET_INTERVAL_FN,
        SERIALIZE_FN,
        DESERIALIZE_FN,
        GET_HANDLE_FN
    };

    void constructed(Os::RawTimeInterface& real) override { this->lastCalled = CONSTRUCT_FN; }
    void destructed(Os::RawTimeInterface& real) override { this->lastCalled = DESTRUCT_FN; }
    void copyConstructed(Os::RawTimeInterface& real, const Os::RawTimeInterface& other) override {
        this->lastCalled = COPY_CONSTRUCT_FN;
    }
    void copyAssigned(Os::RawTimeInterface& real, const Os::RawTimeInterface& other) override {
        this->lastCalled = COPY_ASSIGN_FN;
    }
    Os::RawTimeHandle* getHandle(Os::RawTimeInterface& real) override {
        this->lastCalled = GET_HANDLE_FN;
        return real.getHandle();
    }
    Status now(Os::RawTimeInterface& real) override {
        this->lastCalled = GET_TIME_FN;
        return Status::OP_OK;
    }
    Status getTimeInterval(const Os::RawTimeInterface& real,
                           const Os::RawTimeInterface& other,
                           Fw::TimeInterval& interval) override {
        this->lastCalled = GET_INTERVAL_FN;
        this->intervalReal = &real;
        this->intervalOther = &other;
        return Status::OP_OK;
    }
    Fw::SerializeStatus serializeTo(const Os::RawTimeInterface& real,
                                    Fw::SerialBufferBase& buffer,
                                    Fw::Endianness mode) override {
        this->lastCalled = SERIALIZE_FN;
        return Fw::FW_SERIALIZE_OK;
    }
    Fw::SerializeStatus deserializeFrom(Os::RawTimeInterface& real,
                                        Fw::SerialBufferBase& buffer,
                                        Fw::Endianness mode) override {
        this->lastCalled = DESERIALIZE_FN;
        return Fw::FW_SERIALIZE_OK;
    }

    LastFn lastCalled = NONE_FN;                          //!< Last function called
    const Os::RawTimeInterface* intervalReal = nullptr;   //!< Implementation getTimeInterval was called on
    const Os::RawTimeInterface* intervalOther = nullptr;  //!< Implementation passed to getTimeInterval
};

class Interface : public ::testing::Test {
  public:
    Recorder recorder;  //!< Installed for the duration of each test
};

}  // namespace

// Ensure that Os::RawTime properly calls the implementation constructor
TEST_F(Interface, Construction) {
    Os::RawTime rawtime;
    ASSERT_EQ(recorder.lastCalled, Recorder::CONSTRUCT_FN);
}

// Ensure that Os::RawTime properly calls the implementation copy constructor
TEST_F(Interface, CopyConstruction) {
    Os::RawTime rawtime;
    Os::RawTime rawtime_copy(rawtime);
    ASSERT_EQ(recorder.lastCalled, Recorder::COPY_CONSTRUCT_FN);
}

// Ensure that Os::RawTime properly calls the implementation copy assignment
TEST_F(Interface, CopyAssignment) {
    Os::RawTime rawtime;
    Os::RawTime rawtime_copy;
    rawtime_copy = rawtime;
    ASSERT_EQ(recorder.lastCalled, Recorder::COPY_ASSIGN_FN);
}

// Ensure that Os::RawTime properly calls the implementation destructor
TEST_F(Interface, Destruction) {
    delete (new Os::RawTime);
    ASSERT_EQ(recorder.lastCalled, Recorder::DESTRUCT_FN);
}

// Ensure that Os::RawTime properly calls the implementation now()
TEST_F(Interface, Now) {
    Os::RawTime rawtime;
    ASSERT_EQ(rawtime.now(), Os::RawTime::Status::OP_OK);
    ASSERT_EQ(recorder.lastCalled, Recorder::GET_TIME_FN);
}

// Ensure that Os::RawTime properly calls the implementation getTimeInterval() with the other implementation
TEST_F(Interface, GetTimeInterval) {
    Os::RawTime rawtime;
    Os::RawTime other;
    Fw::TimeInterval unused;
    ASSERT_EQ(rawtime.getTimeInterval(other, unused), Os::RawTime::Status::OP_OK);
    ASSERT_EQ(recorder.lastCalled, Recorder::GET_INTERVAL_FN);
    ASSERT_NE(recorder.intervalReal, nullptr);
    ASSERT_NE(recorder.intervalOther, nullptr);
    ASSERT_NE(recorder.intervalReal, recorder.intervalOther);
}

// Ensure that Os::RawTime properly calls the implementation serializeTo()
TEST_F(Interface, Serialize) {
    Os::RawTime rawtime;
    Fw::Buffer buffer;
    auto esb = buffer.getSerializer();
    ASSERT_EQ(rawtime.serializeTo(esb), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(recorder.lastCalled, Recorder::SERIALIZE_FN);
}

// Ensure that Os::RawTime properly calls the implementation deserializeFrom()
TEST_F(Interface, Deserialize) {
    Os::RawTime rawtime;
    Fw::Buffer buffer;
    auto esb = buffer.getDeserializer();
    ASSERT_EQ(rawtime.deserializeFrom(esb), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(recorder.lastCalled, Recorder::DESERIALIZE_FN);
}

// Ensure that Os::RawTime properly calls the implementation getHandle()
TEST_F(Interface, GetHandle) {
    Os::RawTime rawtime;
    ASSERT_NE(rawtime.getHandle(), nullptr);
    ASSERT_EQ(recorder.lastCalled, Recorder::GET_HANDLE_FN);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
