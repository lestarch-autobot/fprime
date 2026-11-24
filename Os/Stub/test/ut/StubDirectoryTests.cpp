// ======================================================================
// \title Os/Stub/test/ut/StubDirectoryTests.cpp
// \brief stub implementation for Os::DirectoryInterface testing
// This ensures the delegation of function calls happens properly
// ======================================================================
#include <gtest/gtest.h>
#include "Os/Directory.hpp"
#include "Os/FileSystem.hpp"
#include "Os/Stub/test/Directory.hpp"
#include "Os/test/ut/directory/CommonTests.hpp"
#include "Os/test/ut/directory/RulesHeaders.hpp"

// Wrapper under test: Os::DirectoryCommon on the implementation named here, not the selection in config/OsSelection.hpp
using DirectoryUnderTest = Os::DirectoryCommon<Os::Stub::Directory::Test::TestDirectory>;

using namespace Os::Stub::Directory::Test;

// Basic file tests
class Interface : public ::testing::Test {
  public:
    //! Setup function delegating to UT setUp function
    void SetUp() override { StaticData::data = StaticData(); }

    //! Setup function delegating to UT tearDown function
    void TearDown() override { StaticData::data = StaticData(); }
};

// Ensure that Os::Directory properly calls the implementation constructor
TEST_F(Interface, Construction) {
    DirectoryUnderTest directory;
    ASSERT_EQ(StaticData::data.lastCalled, StaticData::LastFn::CONSTRUCT_FN);
}

// Ensure that Os::Directory properly calls the implementation destructor
TEST_F(Interface, Destruction) {
    delete (new DirectoryUnderTest);
    ASSERT_EQ(StaticData::data.lastCalled, StaticData::LastFn::DESTRUCT_FN);
}

// Ensure that Os::Directory properly calls the implementation open()
TEST_F(Interface, Open) {
    DirectoryUnderTest directory;
    directory.open("/does/not/matter", DirectoryUnderTest::OpenMode::CREATE_IF_MISSING);
    ASSERT_EQ(StaticData::data.lastCalled, StaticData::LastFn::OPEN_FN);
}

// Ensure that Os::Directory properly calls the implementation rewind()
TEST_F(Interface, Rewind) {
    DirectoryUnderTest directory;
    directory.open("/does/not/matter", DirectoryUnderTest::OpenMode::READ);
    directory.rewind();
    ASSERT_EQ(StaticData::data.lastCalled, StaticData::LastFn::REWIND_FN);
}

// Ensure that Os::Directory properly calls the implementation open()
TEST_F(Interface, Read) {
    DirectoryUnderTest directory;
    directory.open("/does/not/matter", DirectoryUnderTest::OpenMode::READ);
    char buffer[4];
    directory.read(buffer, 4);
    ASSERT_EQ(StaticData::data.lastCalled, StaticData::LastFn::READ_FN);
}

// Ensure that Os::Directory properly calls the implementation open()
TEST_F(Interface, Close) {
    DirectoryUnderTest directory;
    directory.close();
    ASSERT_EQ(StaticData::data.lastCalled, StaticData::LastFn::CLOSE_FN);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    STest::Random::seed();
    return RUN_ALL_TESTS();
}
