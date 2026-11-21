// ======================================================================
// \title Os/FileSystem.cpp
// \brief common function implementation for Os::FileSystem
// ======================================================================
#include <Fw/Types/Assert.hpp>
#include <Os/FileSystem.hpp>
#include <algorithm>

namespace Os {

FileSystemCommon::FileSystemCommon()
    : m_handle_storage(), m_delegate(*FileSystemInterface::getDelegate(m_handle_storage)) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<FileSystemInterface*>(&this->m_handle_storage[0]));
}

FileSystemCommon::~FileSystemCommon() {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<FileSystemInterface*>(&this->m_handle_storage[0]));
    m_delegate.~FileSystemInterface();
}

FileSystemHandle* FileSystemCommon::getHandle() {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<FileSystemInterface*>(&this->m_handle_storage[0]));
    return this->m_delegate.getHandle();
}

FileSystemCommon::Status FileSystemCommon::_removeDirectory(const char* path) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<FileSystemInterface*>(&this->m_handle_storage[0]));
    FW_ASSERT(path != nullptr);
    return this->m_delegate._removeDirectory(path);
}

FileSystemCommon::Status FileSystemCommon::_removeFile(const char* path) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<FileSystemInterface*>(&this->m_handle_storage[0]));
    FW_ASSERT(path != nullptr);
    return this->m_delegate._removeFile(path);
}

FileSystemCommon::Status FileSystemCommon::_rename(const char* sourcePath, const char* destPath) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<FileSystemInterface*>(&this->m_handle_storage[0]));
    FW_ASSERT(sourcePath != nullptr);
    FW_ASSERT(destPath != nullptr);
    return this->m_delegate._rename(sourcePath, destPath);
}

FileSystemCommon::Status FileSystemCommon::_getPathType(const char* path, PathType& pathType) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<FileSystemInterface*>(&this->m_handle_storage[0]));
    FW_ASSERT(path != nullptr);
    return this->m_delegate._getPathType(path, pathType);
}

FileSystemCommon::Status FileSystemCommon::_getWorkingDirectory(char* path, FwSizeType bufferSize) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<FileSystemInterface*>(&this->m_handle_storage[0]));
    FW_ASSERT(path != nullptr);
    FW_ASSERT(bufferSize > 0);  // because bufferSize=0 would trigger a malloc in some implementations (e.g. Posix)
    return this->m_delegate._getWorkingDirectory(path, bufferSize);
}

FileSystemCommon::Status FileSystemCommon::_changeWorkingDirectory(const char* path) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<FileSystemInterface*>(&this->m_handle_storage[0]));
    FW_ASSERT(path != nullptr);
    return this->m_delegate._changeWorkingDirectory(path);
}

FileSystemCommon::Status FileSystemCommon::_getFreeSpace(const char* path,
                                                         FwSizeType& totalBytes,
                                                         FwSizeType& freeBytes) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<FileSystemInterface*>(&this->m_handle_storage[0]));
    FW_ASSERT(path != nullptr);
    return this->m_delegate._getFreeSpace(path, totalBytes, freeBytes);
}

void FileSystemCommon::init() {
    // Force trigger on the fly singleton setup
    (void)FileSystemCommon::getSingleton();
}

FileSystemCommon& FileSystemCommon::getSingleton() {
    static FileSystemCommon s_singleton;
    return s_singleton;
}

// ------------------------------------------------------------
// Static functions calling implementation-specific operations
// ------------------------------------------------------------

FileSystemCommon::Status FileSystemCommon::removeDirectory(const char* path) {
    return FileSystemCommon::getSingleton()._removeDirectory(path);
}

FileSystemCommon::Status FileSystemCommon::removeFile(const char* path) {
    return FileSystemCommon::getSingleton()._removeFile(path);
}

FileSystemCommon::Status FileSystemCommon::rename(const char* sourcePath, const char* destPath) {
    return FileSystemCommon::getSingleton()._rename(sourcePath, destPath);
}

FileSystemCommon::Status FileSystemCommon::getWorkingDirectory(char* path, FwSizeType bufferSize) {
    return FileSystemCommon::getSingleton()._getWorkingDirectory(path, bufferSize);
}

FileSystemCommon::Status FileSystemCommon::changeWorkingDirectory(const char* path) {
    return FileSystemCommon::getSingleton()._changeWorkingDirectory(path);
}

FileSystemCommon::Status FileSystemCommon::getFreeSpace(const char* path,
                                                        FwSizeType& totalBytes,
                                                        FwSizeType& freeBytes) {
    return FileSystemCommon::getSingleton()._getFreeSpace(path, totalBytes, freeBytes);
}

// ------------------------------------------------------------
// Additional functions built on top of OS-specific operations
// ------------------------------------------------------------

FileSystemCommon::Status FileSystemCommon::createDirectory(const char* path, bool errorIfAlreadyExists) {
    FW_ASSERT(path != nullptr);
    Status status = Status::OP_OK;
    Os::Directory dir;
    // If errorIfAlreadyExists is true, use CREATE_EXCLUSIVE mode, otherwise use CREATE_IF_MISSING
    Directory::OpenMode mode =
        errorIfAlreadyExists ? Directory::OpenMode::CREATE_EXCLUSIVE : Directory::OpenMode::CREATE_IF_MISSING;
    Directory::Status dirStatus = dir.open(path, mode);
    dir.close();
    if (dirStatus != Directory::OP_OK) {
        return FileSystemCommon::handleDirectoryError(dirStatus);
    }
    return status;
}

FileSystemCommon::Status FileSystemCommon::touch(const char* path) {
    FW_ASSERT(path != nullptr);
    Status status = Status::OP_OK;
    Os::File file;
    File::Status file_status = file.open(path, Os::File::OPEN_WRITE);
    file.close();
    if (file_status != File::OP_OK) {
        status = FileSystemCommon::handleFileError(file_status);
    }
    return status;
}

FileSystemCommon::PathType FileSystemCommon::getPathType(const char* path) {
    FW_ASSERT(path != nullptr);
    PathType pathType;
    Status status = getSingleton()._getPathType(path, pathType);
    if (status != Status::OP_OK) {
        return PathType::NOT_EXIST;
    }
    return pathType;
}  // end getPathType

bool FileSystemCommon::exists(const char* path) {
    return FileSystemCommon::getPathType(path) != PathType::NOT_EXIST;
}  // end exists

FileSystemCommon::Status FileSystemCommon::copyFile(const char* sourcePath, const char* destPath) {
    FW_ASSERT(sourcePath != nullptr);
    FW_ASSERT(destPath != nullptr);
    Os::File source;
    Os::File destination;
    Os::File::Status fileStatus = source.open(sourcePath, Os::File::OPEN_READ);
    if (fileStatus != Os::File::OP_OK) {
        return FileSystemCommon::handleFileError(fileStatus);
    }
    fileStatus = destination.open(destPath, Os::File::OPEN_CREATE, Os::File::OverwriteType::OVERWRITE);
    if (fileStatus != Os::File::OP_OK) {
        return FileSystemCommon::handleFileError(fileStatus);
    }

    FwSizeType sourceFileSize = 0;
    FileSystemCommon::Status fs_status = FileSystemCommon::getFileSize(sourcePath, sourceFileSize);
    if (fs_status != FileSystemCommon::Status::OP_OK) {
        return fs_status;
    }

    fs_status = FileSystemCommon::copyFileData(source, destination, sourceFileSize);

    return fs_status;
}  // end copyFile

FileSystemCommon::Status FileSystemCommon::appendFile(const char* sourcePath,
                                                      const char* destPath,
                                                      bool createMissingDest) {
    Os::File source;
    Os::File destination;

    // If requested, check if destination file exists and exit if does not exist
    const bool destExists = FileSystemCommon::exists(destPath);
    if (not createMissingDest and not destExists) {
        return Status::DOESNT_EXIST;
    }

    Os::File::Status fileStatus = source.open(sourcePath, Os::File::OPEN_READ);
    if (fileStatus != Os::File::OP_OK) {
        return FileSystemCommon::handleFileError(fileStatus);
    }
    fileStatus = destination.open(destPath, Os::File::OPEN_APPEND);
    if (fileStatus != Os::File::OP_OK) {
        return FileSystemCommon::handleFileError(fileStatus);
    }

    FileSystemCommon::Status fs_status = FileSystemCommon::OP_OK;

    FwSizeType sourceFileSize = 0;
    fs_status = FileSystemCommon::getFileSize(sourcePath, sourceFileSize);
    if (fs_status != FileSystemCommon::Status::OP_OK) {
        return fs_status;
    }

    fs_status = FileSystemCommon::copyFileData(source, destination, sourceFileSize);

    return fs_status;
}  // end appendFile

FileSystemCommon::Status FileSystemCommon::moveFile(const char* source, const char* destination) {
    Status status = Status::OP_OK;

    // Try to rename the file
    status = FileSystemCommon::rename(source, destination);

    // If rename fails because of cross-device rename, attempt to copy and remove instead
    if (status == Status::EXDEV_ERROR) {
        status = FileSystemCommon::copyFile(source, destination);
        if (status != Status::OP_OK) {
            return status;
        }
        status = FileSystemCommon::removeFile(source);
    }

    return status;
}

FileSystemCommon::Status FileSystemCommon::getFileSize(const char* path, FwSizeType& size) {
    Os::File file;
    Os::File::Status status = file.open(path, Os::File::OPEN_READ);
    if (status != File::Status::OP_OK) {
        return FileSystemCommon::handleFileError(status);
    }
    status = file.size(size);
    if (status != File::Status::OP_OK) {
        return FileSystemCommon::handleFileError(status);
    }
    return FileSystemCommon::OP_OK;
}

// ------------------------------------------------------------
// Internal helper functions
// ------------------------------------------------------------

FileSystemCommon::Status FileSystemCommon::handleFileError(File::Status fileStatus) {
    FileSystemCommon::Status status = FileSystemCommon::OTHER_ERROR;

    switch (fileStatus) {
        case File::NO_SPACE:
            status = FileSystemCommon::NO_SPACE;
            break;
        case File::NO_PERMISSION:
            status = FileSystemCommon::NO_PERMISSION;
            break;
        case File::DOESNT_EXIST:
            status = FileSystemCommon::DOESNT_EXIST;
            break;
        default:
            status = FileSystemCommon::OTHER_ERROR;
    }
    return status;
}  // end handleFileError

FileSystemCommon::Status FileSystemCommon::handleDirectoryError(Directory::Status dirStatus) {
    FileSystemCommon::Status status = FileSystemCommon::OTHER_ERROR;

    switch (dirStatus) {
        case Directory::DOESNT_EXIST:
            status = FileSystemCommon::DOESNT_EXIST;
            break;
        case Directory::NO_PERMISSION:
            status = FileSystemCommon::NO_PERMISSION;
            break;
        case Directory::ALREADY_EXISTS:
            status = FileSystemCommon::ALREADY_EXISTS;
            break;
        case Directory::NOT_SUPPORTED:
            status = FileSystemCommon::NOT_SUPPORTED;
            break;
        default:
            status = FileSystemCommon::OTHER_ERROR;
    }
    return status;
}  // end handleFileError

FileSystemCommon::Status FileSystemCommon::copyFileData(File& source, File& destination, FwSizeType size) {
    static_assert(FILE_SYSTEM_FILE_CHUNK_SIZE != 0, "FILE_SYSTEM_FILE_CHUNK_SIZE must be >0");
    U8 fileBuffer[FILE_SYSTEM_FILE_CHUNK_SIZE];
    File::Status file_status;

    FwSizeType copiedSize = 0;
    FwSizeType chunkSize = FILE_SYSTEM_FILE_CHUNK_SIZE;

    // Loop up to 2 times for each by, bounded to prevent infinite loop
    const FwSizeType maximum =
        (size > (std::numeric_limits<FwSizeType>::max() / 2)) ? std::numeric_limits<FwSizeType>::max() : size * 2;

    // Copy the file in chunks - loop until all data is copied
    FwSizeType i = 0;
    for (copiedSize = 0; (copiedSize < size) && (i < maximum); i++) {
        // chunkSize is FILE_SYSTEM_FILE_CHUNK_SIZE unless size-copiedSize is less than that
        // in which case chunkSize is size-copiedSize, ensuring the last chunk reads the remaining data
        // static_cast needed to avoid ODR linker issue with static constexpr passed by reference to std::min
        chunkSize = std::min(static_cast<FwSizeType>(FILE_SYSTEM_FILE_CHUNK_SIZE), size - copiedSize);
        FwSizeType readSize = chunkSize;
        file_status = source.read(fileBuffer, readSize, Os::File::WaitType::WAIT);
        if (file_status != File::OP_OK) {
            return FileSystemCommon::handleFileError(file_status);
        }
        // A zero-byte read means the source ended before the expected size (e.g. truncated mid-copy)
        if (readSize == 0) {
            break;
        }
        FwSizeType writeSize = readSize;
        file_status = destination.write(fileBuffer, writeSize, Os::File::WaitType::WAIT);
        if (file_status != File::OP_OK) {
            return FileSystemCommon::handleFileError(file_status);
        }
        // A short write means the destination did not receive all the data (e.g. device full)
        if (writeSize != readSize) {
            return FileSystemCommon::OTHER_ERROR;
        }
        copiedSize += writeSize;
    }

    // A copy that did not transfer the full expected size must not report success
    if (copiedSize != size) {
        return FileSystemCommon::OTHER_ERROR;
    }

    return FileSystemCommon::OP_OK;
}  // end copyFileData

}  // namespace Os
