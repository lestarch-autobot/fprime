// ======================================================================
// \title Os/Stub/test/ut/StubRawTimeTests.cpp
// \brief stub implementation for Os::RawTimeInterface testing
// This ensures the delegation of function calls happens properly
// ======================================================================
#include <gtest/gtest.h>
#include "Fw/Buffer/Buffer.hpp"
#include "Fw/Types/Serializable.hpp"
#include "Os/Stub/test/RawTime.hpp"
#include "Os/test/ut/rawtime/CommonTests.hpp"
#include "Os/test/ut/rawtime/RulesHeaders.hpp"

using namespace Os::Stub::RawTime::Test;

// Basic file tests
class Interface : public ::testing::Test {
  public:
    //! Setup function delegating to UT setUp function
    void SetUp() override { StaticData::data = StaticData(); }

    //! Setup function delegating to UT tearDown function
    void TearDown() override { StaticData::data = StaticData(); }
};

// Ensure that Os::RawTime properly calls the implementation constructor
TEST_F(Interface, Construction) {
    Os::RawTime rawtime;
    ASSERT_EQ(StaticData::data.lastCalled, StaticData::LastFn::CONSTRUCT_FN);
}

// Ensure that Os::RawTime properly calls the implementation constructor
TEST_F(Interface, CopyConstruction) {
    Os::RawTime rawtime;
    Os::RawTime rawtime_copy(rawtime);
    ASSERT_EQ(StaticData::data.lastCalled, StaticData::LastFn::COPY_CONSTRUCT_FN);
}

// Ensure that Os::RawTime constructed with a timer source calls the implementation constructor and reports the source
TEST_F(Interface, SourceConstruction) {
    const Os::RawTimeSource source = static_cast<Os::RawTimeSource>(Os::RAWTIME_DEFAULT + 1);
    Os::RawTime rawtime(source);
    ASSERT_EQ(StaticData::data.lastCalled, StaticData::LastFn::CONSTRUCT_FN);
    ASSERT_EQ(rawtime.getSource(), source);
    Os::RawTime rawtime_default;
    ASSERT_EQ(rawtime_default.getSource(), Os::RAWTIME_DEFAULT);
}

// Ensure that copies of Os::RawTime preserve the timer source
TEST_F(Interface, CopyPreservesSource) {
    const Os::RawTimeSource source = static_cast<Os::RawTimeSource>(Os::RAWTIME_DEFAULT + 1);
    Os::RawTime rawtime(source);
    Os::RawTime rawtime_copy(rawtime);
    ASSERT_EQ(StaticData::data.lastCalled, StaticData::LastFn::COPY_CONSTRUCT_FN);
    ASSERT_EQ(rawtime_copy.getSource(), source);
    Os::RawTime rawtime_assigned;
    rawtime_assigned = rawtime;
    ASSERT_EQ(rawtime_assigned.getSource(), source);
}

// Ensure that Os::RawTime properly calls the implementation destructor
TEST_F(Interface, Destruction) {
    delete (new Os::RawTime);
    ASSERT_EQ(StaticData::data.lastCalled, StaticData::LastFn::DESTRUCT_FN);
}

// Ensure that Os::RawTime properly calls the following delegate function
TEST_F(Interface, Now) {
    Os::RawTime rawtime;
    ASSERT_EQ(rawtime.now(), Os::RawTime::Status::OP_OK);
    ASSERT_EQ(StaticData::data.lastCalled, StaticData::LastFn::GET_TIME_FN);
}

// Ensure that Os::RawTime properly calls the following delegate function
TEST_F(Interface, GetTimeInterval) {
    Os::RawTime rawtime;
    Fw::TimeInterval unused;
    ASSERT_EQ(rawtime.getTimeInterval(rawtime, unused), Os::RawTime::Status::OP_OK);
    ASSERT_EQ(StaticData::data.lastCalled, StaticData::LastFn::GET_INTERVAL_FN);
}

// Ensure that Os::RawTime properly calls the following delegate function
TEST_F(Interface, Serialize) {
    Os::RawTime rawtime;
    Fw::Buffer buffer;
    auto esb = buffer.getSerializer();
    ASSERT_EQ(rawtime.serializeTo(esb), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(StaticData::data.lastCalled, StaticData::LastFn::SERIALIZE_FN);
}

// Ensure that Os::RawTime properly calls the following delegate function
TEST_F(Interface, Deserialize) {
    Os::RawTime rawtime;
    Fw::Buffer buffer;
    auto esb = buffer.getDeserializer();
    ASSERT_EQ(rawtime.deserializeFrom(esb), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(StaticData::data.lastCalled, StaticData::LastFn::DESERIALIZE_FN);
}

// Ensure that Os::RawTime properly calls the implementation getHandle()
TEST_F(Interface, GetHandle) {
    Os::RawTime rawtime;
    rawtime.getHandle();
    ASSERT_EQ(StaticData::data.lastCalled, StaticData::LastFn::GET_HANDLE_FN);
}

// Ensure the test stub implementation itself records, reports and copies the timer source
TEST_F(Interface, ImplementationSource) {
    const Os::RawTimeSource source = static_cast<Os::RawTimeSource>(Os::RAWTIME_DEFAULT + 1);
    TestRawTime implementation(source);
    ASSERT_EQ(StaticData::data.lastCalled, StaticData::LastFn::CONSTRUCT_FN);
    ASSERT_EQ(implementation.getSource(), source);
    TestRawTime copied(implementation);
    ASSERT_EQ(copied.getSource(), source);
    TestRawTime assigned;
    ASSERT_EQ(assigned.getSource(), Os::RAWTIME_DEFAULT);
    assigned = implementation;
    ASSERT_EQ(assigned.getSource(), source);
    const Os::RawTimeInterface& interface = implementation;
    ASSERT_EQ(interface.getSource(), source);
    // Interface default, used by implementations that do not override getSource()
    ASSERT_EQ(implementation.Os::RawTimeInterface::getSource(), Os::RAWTIME_DEFAULT);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    STest::Random::seed();
    return RUN_ALL_TESTS();
}
