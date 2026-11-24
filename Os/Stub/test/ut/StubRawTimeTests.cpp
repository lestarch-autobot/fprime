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

// Os::RawTime wrapper instantiated on the test stub, independent of the selection in config/OsSelection.hpp
using RawTimeUnderTest = Os::RawTimeCommon<TestRawTime>;

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
    RawTimeUnderTest rawtime;
    ASSERT_EQ(StaticData::data.lastCalled, StaticData::LastFn::CONSTRUCT_FN);
}

// Ensure that Os::RawTime properly calls the implementation constructor
TEST_F(Interface, CopyConstruction) {
    RawTimeUnderTest rawtime;
    RawTimeUnderTest rawtime_copy(rawtime);
    ASSERT_EQ(StaticData::data.lastCalled, StaticData::LastFn::COPY_CONSTRUCT_FN);
}

// Ensure that Os::RawTime properly calls the implementation destructor
TEST_F(Interface, Destruction) {
    delete (new RawTimeUnderTest);
    ASSERT_EQ(StaticData::data.lastCalled, StaticData::LastFn::DESTRUCT_FN);
}

// Ensure that Os::RawTime properly calls the following delegate function
TEST_F(Interface, Now) {
    RawTimeUnderTest rawtime;
    ASSERT_EQ(rawtime.now(), Os::RawTime::Status::OP_OK);
    ASSERT_EQ(StaticData::data.lastCalled, StaticData::LastFn::GET_TIME_FN);
}

// Ensure that Os::RawTime properly calls the following delegate function
TEST_F(Interface, GetTimeInterval) {
    RawTimeUnderTest rawtime;
    Fw::TimeInterval unused;
    ASSERT_EQ(rawtime.getTimeInterval(rawtime, unused), Os::RawTime::Status::OP_OK);
    ASSERT_EQ(StaticData::data.lastCalled, StaticData::LastFn::GET_INTERVAL_FN);
}

// Ensure that Os::RawTime properly calls the following delegate function
TEST_F(Interface, Serialize) {
    RawTimeUnderTest rawtime;
    Fw::Buffer buffer;
    auto esb = buffer.getSerializer();
    ASSERT_EQ(rawtime.serializeTo(esb), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(StaticData::data.lastCalled, StaticData::LastFn::SERIALIZE_FN);
}

// Ensure that Os::RawTime properly calls the following delegate function
TEST_F(Interface, Deserialize) {
    RawTimeUnderTest rawtime;
    Fw::Buffer buffer;
    auto esb = buffer.getDeserializer();
    ASSERT_EQ(rawtime.deserializeFrom(esb), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(StaticData::data.lastCalled, StaticData::LastFn::DESERIALIZE_FN);
}

// Ensure that Os::RawTime properly calls the implementation getHandle()
TEST_F(Interface, GetHandle) {
    RawTimeUnderTest rawtime;
    rawtime.getHandle();
    ASSERT_EQ(StaticData::data.lastCalled, StaticData::LastFn::GET_HANDLE_FN);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    STest::Random::seed();
    return RUN_ALL_TESTS();
}
