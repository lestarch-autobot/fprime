// ======================================================================
// \title Os/Posix/Directory.cpp
// \brief Posix implementation for Os::Directory
// ======================================================================
#include <sys/stat.h>
#include <cerrno>
#include <cstring>

#include <Fw/Types/Assert.hpp>
#include <Fw/Types/StringUtils.hpp>
#include <Os/Posix/Directory.hpp>
#include <Os/Posix/error.hpp>

namespace Os {
namespace Posix {
namespace Directory {

PosixDirectory::PosixDirectory() : Os::DirectoryInterface(), m_handle() {}

PosixDirectory::~PosixDirectory() {
    // Not dispatched virtually: the directory stream is owned by this class
    PosixDirectory::close();
}

DirectoryHandle* PosixDirectory::getHandle() {
    return &this->m_handle;
}

PosixDirectory::Status PosixDirectory::open(const char* path, OpenMode mode) {
    FW_ASSERT(path != nullptr);
    FW_ASSERT(mode >= 0 and mode < OpenMode::MAX_OPEN_MODE);
    Status status = Status::OP_OK;

    // Re-opening releases the previous directory stream
    if (this->isOpen()) {
        this->close();
    }

    // If one of the CREATE mode, attempt to create the directory
    if (mode == OpenMode::CREATE_EXCLUSIVE || mode == OpenMode::CREATE_IF_MISSING) {
        if (::mkdir(path, S_IRWXU) == -1) {
            status = errno_to_directory_status(errno);
            // If error is not ALREADY_EXISTS, return the error
            // If any error and mode CREATE_EXCLUSIVE, return the error
            // Else, we keep going with OP_OK
            if (status != Status::ALREADY_EXISTS || mode == OpenMode::CREATE_EXCLUSIVE) {
                return status;
            } else {
                status = Status::OP_OK;
            }
        }
    }

    DIR* dir = ::opendir(path);

    if (dir == nullptr) {
        status = errno_to_directory_status(errno);
    }

    this->m_handle.m_dir_descriptor = dir;
    this->setOpen(dir != nullptr);
    return status;
}

PosixDirectory::Status PosixDirectory::rewind() {
    if (not this->isOpen()) {
        return Status::NOT_OPENED;
    }
    Status status = Status::OP_OK;
    // no errors defined in man page for rewinddir
    ::rewinddir(this->m_handle.m_dir_descriptor);
    return status;
}

PosixDirectory::Status PosixDirectory::read(char* fileNameBuffer, FwSizeType bufSize) {
    if (not this->isOpen()) {
        return Status::NOT_OPENED;
    }
    FW_ASSERT(fileNameBuffer != nullptr);
    FW_ASSERT(bufSize > 0);

    Status status = Status::OP_OK;

    // Set errno to 0 so we know why we exited readdir
    // This is recommended by the manual pages (man 3 readdir)
    errno = 0;

    struct dirent* direntData = ::readdir(this->m_handle.m_dir_descriptor);
    while (direntData != nullptr) {
        // Skip . and .. directory entries
        if ((direntData->d_name[0] == '.' and direntData->d_name[1] == '\0') or
            (direntData->d_name[0] == '.' and direntData->d_name[1] == '.' and direntData->d_name[2] == '\0')) {
            direntData = ::readdir(this->m_handle.m_dir_descriptor);
        } else {
            (void)Fw::StringUtils::string_copy(fileNameBuffer, direntData->d_name, bufSize);
            break;
        }
    }
    if (direntData == nullptr) {
        // loop ended because readdir failed, did it error or did we run out of files?
        if (errno != 0) {
            // Only error from readdir is EBADF
            status = Status::BAD_DESCRIPTOR;
        } else {
            status = Status::NO_MORE_FILES;
        }
    }
    // Guarantee null-termination regardless of the outcome
    fileNameBuffer[bufSize - 1] = '\0';
    return status;
}

void PosixDirectory::close() {
    // ::closedir errors if dir descriptor is nullptr
    if (this->m_handle.m_dir_descriptor != nullptr) {
        (void)::closedir(this->m_handle.m_dir_descriptor);
    }
    this->m_handle.m_dir_descriptor = nullptr;
    this->setOpen(false);
}

}  // namespace Directory
}  // namespace Posix
}  // namespace Os
