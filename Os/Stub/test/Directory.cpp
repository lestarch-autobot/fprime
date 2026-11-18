// ======================================================================
// \title Os/Stub/test/Directory.cpp
// \brief implementation for TestDirectory stubs for interface testing
// ======================================================================

#include "Os/Stub/test/Directory.hpp"
#include <Fw/Types/Assert.hpp>
#include "Os/Directory.hpp"

namespace Os {
namespace Stub {
namespace Directory {
namespace Test {

StaticData StaticData::data;

TestDirectory::TestDirectory() {
    StaticData::data.lastCalled = StaticData::LastFn::CONSTRUCT_FN;
}

TestDirectory::~TestDirectory() {
    StaticData::data.lastCalled = StaticData::LastFn::DESTRUCT_FN;
}

TestDirectory::Status TestDirectory::open(const char* path, OpenMode mode) {
    FW_ASSERT(path != nullptr);
    FW_ASSERT(mode >= 0 and mode < OpenMode::MAX_OPEN_MODE);
    StaticData::data.lastCalled = StaticData::LastFn::OPEN_FN;
    this->setOpen(true);
    return Status::OP_OK;
}

TestDirectory::Status TestDirectory::rewind() {
    if (not this->isOpen()) {
        return Status::NOT_OPENED;
    }
    StaticData::data.lastCalled = StaticData::LastFn::REWIND_FN;
    return Status::OP_OK;
}

TestDirectory::Status TestDirectory::read(char* fileNameBuffer, FwSizeType bufSize) {
    if (not this->isOpen()) {
        return Status::NOT_OPENED;
    }
    FW_ASSERT(fileNameBuffer != nullptr);
    FW_ASSERT(bufSize > 0);
    StaticData::data.lastCalled = StaticData::LastFn::READ_FN;
    fileNameBuffer[bufSize - 1] = '\0';  // Guarantee null-termination
    return Status::OP_OK;
}

void TestDirectory::close() {
    StaticData::data.lastCalled = StaticData::LastFn::CLOSE_FN;
    this->setOpen(false);
}

Os::DirectoryHandle* TestDirectory::getHandle() {
    StaticData::data.lastCalled = StaticData::LastFn::GET_HANDLE_FN;
    return nullptr;
}

}  // namespace Test
}  // namespace Directory
}  // namespace Stub
}  // namespace Os
