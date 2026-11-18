// ======================================================================
// \title Os/FileSystem.cpp
// \brief common function implementation for Os::FileSystemInterface
//
// These functions are shared by every Os::FileSystem selection: the
// link-time delegate and compile-time aliases of an implementation.
// ======================================================================
#include <Fw/Types/Assert.hpp>
#include <Os/FileSystem.hpp>
#include <algorithm>
#include <type_traits>

namespace Os {

static_assert(std::is_base_of<FileSystemInterface, FileSystem>::value,
              "Os::FileSystem must be selected as a type derived from Os::FileSystemInterface");

void FileSystemInterface::init() {
    // Force trigger on the fly singleton setup
    (void)FileSystem::getSingleton();
}

FileSystem& FileSystemInterface::getSingleton() {
    static FileSystem s_singleton;
    return s_singleton;
}

// ------------------------------------------------------------
// Static functions calling implementation-specific operations
// ------------------------------------------------------------

FileSystemInterface::Status FileSystemInterface::removeDirectory(const char* path) {
    return FileSystem::getSingleton()._removeDirectory(path);
}

FileSystemInterface::Status FileSystemInterface::removeFile(const char* path) {
    return FileSystem::getSingleton()._removeFile(path);
}

FileSystemInterface::Status FileSystemInterface::rename(const char* sourcePath, const char* destPath) {
    return FileSystem::getSingleton()._rename(sourcePath, destPath);
}

FileSystemInterface::Status FileSystemInterface::getWorkingDirectory(char* path, FwSizeType bufferSize) {
    return FileSystem::getSingleton()._getWorkingDirectory(path, bufferSize);
}

FileSystemInterface::Status FileSystemInterface::changeWorkingDirectory(const char* path) {
    return FileSystem::getSingleton()._changeWorkingDirectory(path);
}

FileSystemInterface::Status FileSystemInterface::getFreeSpace(const char* path,
                                                              FwSizeType& totalBytes,
                                                              FwSizeType& freeBytes) {
    return FileSystem::getSingleton()._getFreeSpace(path, totalBytes, freeBytes);
}

// ------------------------------------------------------------
// Additional functions built on top of OS-specific operations
// ------------------------------------------------------------

FileSystemInterface::Status FileSystemInterface::createDirectory(const char* path, bool errorIfAlreadyExists) {
    FW_ASSERT(path != nullptr);
    Status status = Status::OP_OK;
    Os::Directory dir;
    // If errorIfAlreadyExists is true, use CREATE_EXCLUSIVE mode, otherwise use CREATE_IF_MISSING
    Directory::OpenMode mode =
        errorIfAlreadyExists ? Directory::OpenMode::CREATE_EXCLUSIVE : Directory::OpenMode::CREATE_IF_MISSING;
    Directory::Status dirStatus = dir.open(path, mode);
    dir.close();
    if (dirStatus != Directory::OP_OK) {
        return FileSystem::handleDirectoryError(dirStatus);
    }
    return status;
}

FileSystemInterface::Status FileSystemInterface::touch(const char* path) {
    FW_ASSERT(path != nullptr);
    Status status = Status::OP_OK;
    Os::File file;
    File::Status file_status = file.open(path, Os::File::OPEN_WRITE);
    file.close();
    if (file_status != File::OP_OK) {
        status = FileSystem::handleFileError(file_status);
    }
    return status;
}

FileSystemInterface::PathType FileSystemInterface::getPathType(const char* path) {
    FW_ASSERT(path != nullptr);
    PathType pathType;
    Status status = getSingleton()._getPathType(path, pathType);
    if (status != Status::OP_OK) {
        return PathType::NOT_EXIST;
    }
    return pathType;
}  // end getPathType

bool FileSystemInterface::exists(const char* path) {
    return FileSystem::getPathType(path) != PathType::NOT_EXIST;
}  // end exists

FileSystemInterface::Status FileSystemInterface::copyFile(const char* sourcePath, const char* destPath) {
    FW_ASSERT(sourcePath != nullptr);
    FW_ASSERT(destPath != nullptr);
    Os::File source;
    Os::File destination;
    Os::File::Status fileStatus = source.open(sourcePath, Os::File::OPEN_READ);
    if (fileStatus != Os::File::OP_OK) {
        return FileSystem::handleFileError(fileStatus);
    }
    fileStatus = destination.open(destPath, Os::File::OPEN_CREATE, Os::File::OverwriteType::OVERWRITE);
    if (fileStatus != Os::File::OP_OK) {
        return FileSystem::handleFileError(fileStatus);
    }

    FwSizeType sourceFileSize = 0;
    FileSystemInterface::Status fs_status = FileSystem::getFileSize(sourcePath, sourceFileSize);
    if (fs_status != FileSystem::Status::OP_OK) {
        return fs_status;
    }

    fs_status = FileSystem::copyFileData(source, destination, sourceFileSize);

    return fs_status;
}  // end copyFile

FileSystemInterface::Status FileSystemInterface::appendFile(const char* sourcePath,
                                                            const char* destPath,
                                                            bool createMissingDest) {
    FW_ASSERT(sourcePath != nullptr);
    FW_ASSERT(destPath != nullptr);
    Os::File source;
    Os::File destination;

    // If requested, check if destination file exists and exit if does not exist
    const bool destExists = FileSystem::exists(destPath);
    if (not createMissingDest and not destExists) {
        return Status::DOESNT_EXIST;
    }

    Os::File::Status fileStatus = source.open(sourcePath, Os::File::OPEN_READ);
    if (fileStatus != Os::File::OP_OK) {
        return FileSystem::handleFileError(fileStatus);
    }
    fileStatus = destination.open(destPath, Os::File::OPEN_APPEND);
    if (fileStatus != Os::File::OP_OK) {
        return FileSystem::handleFileError(fileStatus);
    }

    FileSystemInterface::Status fs_status = FileSystem::OP_OK;

    FwSizeType sourceFileSize = 0;
    fs_status = FileSystem::getFileSize(sourcePath, sourceFileSize);
    if (fs_status != FileSystem::Status::OP_OK) {
        return fs_status;
    }

    fs_status = FileSystem::copyFileData(source, destination, sourceFileSize);

    return fs_status;
}  // end appendFile

FileSystemInterface::Status FileSystemInterface::moveFile(const char* source, const char* destination) {
    FW_ASSERT(source != nullptr);
    FW_ASSERT(destination != nullptr);
    Status status = Status::OP_OK;

    // Try to rename the file
    status = FileSystem::rename(source, destination);

    // If rename fails because of cross-device rename, attempt to copy and remove instead
    if (status == Status::EXDEV_ERROR) {
        status = FileSystem::copyFile(source, destination);
        if (status != Status::OP_OK) {
            return status;
        }
        status = FileSystem::removeFile(source);
    }

    return status;
}

FileSystemInterface::Status FileSystemInterface::getFileSize(const char* path, FwSizeType& size) {
    FW_ASSERT(path != nullptr);
    Os::File file;
    Os::File::Status status = file.open(path, Os::File::OPEN_READ);
    if (status != File::Status::OP_OK) {
        return FileSystem::handleFileError(status);
    }
    status = file.size(size);
    if (status != File::Status::OP_OK) {
        return FileSystem::handleFileError(status);
    }
    return FileSystem::OP_OK;
}

// ------------------------------------------------------------
// Internal helper functions
// ------------------------------------------------------------

FileSystemInterface::Status FileSystemInterface::handleFileError(File::Status fileStatus) {
    FileSystemInterface::Status status = FileSystem::OTHER_ERROR;

    switch (fileStatus) {
        case File::NO_SPACE:
            status = FileSystem::NO_SPACE;
            break;
        case File::NO_PERMISSION:
            status = FileSystem::NO_PERMISSION;
            break;
        case File::DOESNT_EXIST:
            status = FileSystem::DOESNT_EXIST;
            break;
        default:
            status = FileSystem::OTHER_ERROR;
    }
    return status;
}  // end handleFileError

FileSystemInterface::Status FileSystemInterface::handleDirectoryError(Directory::Status dirStatus) {
    FileSystemInterface::Status status = FileSystem::OTHER_ERROR;

    switch (dirStatus) {
        case Directory::DOESNT_EXIST:
            status = FileSystem::DOESNT_EXIST;
            break;
        case Directory::NO_PERMISSION:
            status = FileSystem::NO_PERMISSION;
            break;
        case Directory::ALREADY_EXISTS:
            status = FileSystem::ALREADY_EXISTS;
            break;
        case Directory::NOT_SUPPORTED:
            status = FileSystem::NOT_SUPPORTED;
            break;
        default:
            status = FileSystem::OTHER_ERROR;
    }
    return status;
}  // end handleFileError

FileSystemInterface::Status FileSystemInterface::copyFileData(File& source, File& destination, FwSizeType size) {
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
            return FileSystem::handleFileError(file_status);
        }
        // A zero-byte read means the source ended before the expected size (e.g. truncated mid-copy)
        if (readSize == 0) {
            break;
        }
        FwSizeType writeSize = readSize;
        file_status = destination.write(fileBuffer, writeSize, Os::File::WaitType::WAIT);
        if (file_status != File::OP_OK) {
            return FileSystem::handleFileError(file_status);
        }
        // A short write means the destination did not receive all the data (e.g. device full)
        if (writeSize != readSize) {
            return FileSystem::OTHER_ERROR;
        }
        copiedSize += writeSize;
    }

    // A copy that did not transfer the full expected size must not report success
    if (copiedSize != size) {
        return FileSystem::OTHER_ERROR;
    }

    return FileSystem::OP_OK;
}  // end copyFileData

}  // namespace Os
