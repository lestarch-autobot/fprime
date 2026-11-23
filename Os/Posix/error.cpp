// ======================================================================
// \title Os/Posix/error.cpp
// \brief implementation for posix errno conversion
// ======================================================================
#include "Os/Posix/error.hpp"
#include <cerrno>

namespace Os {
namespace Posix {

Os::File::Status errno_to_file_status(int errno_input) {
    Os::File::Status status = Os::File::Status::OP_OK;
    switch (errno_input) {
        case 0:
            status = Os::File::Status::OP_OK;
            break;
        // Fallthrough intended
        case ENOSPC:
        case EFBIG:
            status = Os::File::Status::NO_SPACE;
            break;
        case ENOENT:
            status = Os::File::Status::DOESNT_EXIST;
            break;
        // Fallthrough intended
        case EPERM:
        case EACCES:
            status = Os::File::Status::NO_PERMISSION;
            break;
        case EEXIST:
            status = Os::File::Status::FILE_EXISTS;
            break;
        case EBADF:
            status = Os::File::Status::NOT_OPENED;
            break;
        // Fallthrough intended
        case ENOSYS:
        case EOPNOTSUPP:
            status = Os::File::Status::NOT_SUPPORTED;
            break;
        case EINVAL:
            status = Os::File::Status::INVALID_ARGUMENT;
            break;
        default:
            status = Os::File::Status::OTHER_ERROR;
            break;
    }
    return status;
}

Os::FileSystem::Status errno_to_filesystem_status(int errno_input) {
    Os::FileSystem::Status status = Os::FileSystem::Status::OP_OK;
    switch (errno_input) {
        case 0:
            status = Os::FileSystem::Status::OP_OK;
            break;
        // Adjacent cases below intentionally share a status via fallthrough
        case EACCES:
            status = Os::FileSystem::Status::NO_PERMISSION;
            break;
        case EPERM:
        case EROFS:
        case EFAULT:
            status = Os::FileSystem::Status::NO_PERMISSION;
            break;
        case EEXIST:
            status = Os::FileSystem::Status::ALREADY_EXISTS;
            break;
        case ELOOP:
        case ENOENT:
            status = Os::FileSystem::Status::DOESNT_EXIST;
            break;
        case ENAMETOOLONG:
            status = Os::FileSystem::Status::INVALID_PATH;
            break;
        case ENOTDIR:
            status = Os::FileSystem::Status::NOT_DIR;
            break;
        case EDQUOT:
            status = Os::FileSystem::Status::NO_SPACE;
            break;
        case EMLINK:
            status = Os::FileSystem::Status::FILE_LIMIT;
            break;
        case ENOSPC:
        case EFBIG:
            status = Os::FileSystem::Status::NO_SPACE;
            break;
        case ENOSYS:
        case EOPNOTSUPP:
            status = Os::FileSystem::Status::NOT_SUPPORTED;
            break;
        case ERANGE:
            status = Os::FileSystem::Status::BUFFER_TOO_SMALL;
            break;
        case EXDEV:
            status = Os::FileSystem::Status::EXDEV_ERROR;
            break;
        default:
            status = Os::FileSystem::Status::OTHER_ERROR;
            break;
    }
    return status;
}

Os::Directory::Status errno_to_directory_status(int errno_input) {
    Os::Directory::Status status = Os::Directory::Status::OP_OK;
    switch (errno_input) {
        case 0:
            status = Os::Directory::Status::OP_OK;
            break;
        case ENOENT:
            status = Os::Directory::Status::DOESNT_EXIST;
            break;
        case EACCES:
            status = Os::Directory::Status::NO_PERMISSION;
            break;
        case ENOTDIR:
            status = Os::Directory::Status::NOT_DIR;
            break;
        case EEXIST:
            status = Os::Directory::Status::ALREADY_EXISTS;
            break;
        default:
            status = Os::Directory::Status::OTHER_ERROR;
            break;
    }
    return status;
}

RawTime::Status errno_to_rawtime_status(int errno_input) {
    RawTime::Status status = RawTime::Status::OP_OK;
    switch (errno_input) {
        case 0:
            status = RawTime::Status::OP_OK;
            break;
        case EINVAL:
            status = RawTime::Status::INVALID_PARAMS;
            break;
        default:
            status = RawTime::Status::OTHER_ERROR;
            break;
    }
    return status;
}

Os::Task::Status posix_status_to_task_status(int posix_status) {
    Os::Task::Status status = Os::Task::Status::OP_OK;
    switch (posix_status) {
        case 0:
            status = Os::Task::Status::OP_OK;
            break;
        case EINVAL:
            status = Os::Task::Status::INVALID_PARAMS;
            break;
        case EPERM:
            status = Os::Task::Status::ERROR_PERMISSION;
            break;
        case EAGAIN:
            status = Os::Task::Status::ERROR_RESOURCES;
            break;
        default:
            status = Os::Task::Status::UNKNOWN_ERROR;
            break;
    }
    return status;
}

Os::Mutex::Status posix_status_to_mutex_status(int posix_status) {
    Os::Mutex::Status status = Os::Mutex::Status::ERROR_OTHER;
    switch (posix_status) {
        case 0:
            status = Os::Mutex::Status::OP_OK;
            break;
        case EBUSY:
            status = Os::Mutex::Status::ERROR_BUSY;
            break;
        case EDEADLK:
            status = Os::Mutex::Status::ERROR_DEADLOCK;
            break;
        default:
            status = Os::Mutex::Status::ERROR_OTHER;
            break;
    }
    return status;
}

ConditionVariable::Status posix_status_to_conditional_status(int posix_status) {
    ConditionVariable::Status status = ConditionVariable::Status::ERROR_OTHER;
    switch (posix_status) {
        case 0:
            status = ConditionVariable::Status::OP_OK;
            break;
        case EPERM:
            status = ConditionVariable::Status::ERROR_MUTEX_NOT_HELD;
            break;
        default:
            status = ConditionVariable::Status::ERROR_OTHER;
            break;
    }
    return status;
}

CountingSemaphore::Status posix_status_to_semaphore_status(int posix_status) {
    CountingSemaphore::Status status = CountingSemaphore::Status::ERROR_OTHER;
    switch (posix_status) {
        case 0:
            status = CountingSemaphore::Status::OP_OK;
            break;
        case ETIMEDOUT:
        case EAGAIN:
            status = CountingSemaphore::Status::ERROR_TIMEOUT;
            break;
        case EINVAL:
            status = CountingSemaphore::Status::ERROR_INVALID;
            break;
        default:
            status = CountingSemaphore::Status::ERROR_OTHER;
            break;
    }
    return status;
}
}  // namespace Posix
}  // namespace Os
