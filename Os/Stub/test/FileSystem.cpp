// ======================================================================
// \title Os/Stub/test/FileSystem.cpp
// \brief implementation for TestFileSystem stubs for interface testing
// ======================================================================

#include "Os/Stub/test/FileSystem.hpp"
#include "Fw/Types/Assert.hpp"
#include "Os/FileSystem.hpp"

namespace Os {
namespace Stub {
namespace FileSystem {
namespace Test {

StaticData StaticData::data;

TestFileSystem::TestFileSystem() {
    StaticData::data.lastCalled = StaticData::LastFn::CONSTRUCT_FN;
}

TestFileSystem::~TestFileSystem() {
    StaticData::data.lastCalled = StaticData::LastFn::DESTRUCT_FN;
}

TestFileSystem::Status TestFileSystem::_removeDirectory(const char* path) {
    FW_ASSERT(path != nullptr);
    StaticData::data.lastCalled = StaticData::LastFn::REMOVE_DIR_FN;
    return Status::OP_OK;
}

TestFileSystem::Status TestFileSystem::_removeFile(const char* path) {
    FW_ASSERT(path != nullptr);
    StaticData::data.lastCalled = StaticData::LastFn::REMOVE_FILE_FN;
    return Status::OP_OK;
}

TestFileSystem::Status TestFileSystem::_rename(const char* originPath, const char* destPath) {
    FW_ASSERT(originPath != nullptr);
    FW_ASSERT(destPath != nullptr);
    StaticData::data.lastCalled = StaticData::LastFn::RENAME_FN;
    return Status::OP_OK;
}

TestFileSystem::Status TestFileSystem::_getWorkingDirectory(char* path, FwSizeType size) {
    FW_ASSERT(path != nullptr);
    FW_ASSERT(size > 0);
    StaticData::data.lastCalled = StaticData::LastFn::GET_CWD_FN;
    return Status::OP_OK;
}

TestFileSystem::Status TestFileSystem::_changeWorkingDirectory(const char* path) {
    FW_ASSERT(path != nullptr);
    StaticData::data.lastCalled = StaticData::LastFn::CHANGE_CWD_FN;
    return Status::OP_OK;
}

TestFileSystem::Status TestFileSystem::_getFreeSpace(const char* path, FwSizeType& totalBytes, FwSizeType& freeBytes) {
    FW_ASSERT(path != nullptr);
    StaticData::data.lastCalled = StaticData::LastFn::GET_FREESPACE_FN;
    return Status::OP_OK;
}

Os::FileSystemHandle* TestFileSystem::getHandle() {
    StaticData::data.lastCalled = StaticData::LastFn::GET_HANDLE_FN;
    return &this->m_handle;
}

TestFileSystem::Status TestFileSystem::_getPathType(const char* path, PathType& pathType) {
    FW_ASSERT(path != nullptr);
    StaticData::data.lastCalled = StaticData::GET_PATH_TYPE_FN;
    pathType = PathType::NOT_EXIST;
    return StaticData::data.lastStatus;
}

}  // namespace Test
}  // namespace FileSystem
}  // namespace Stub
}  // namespace Os
