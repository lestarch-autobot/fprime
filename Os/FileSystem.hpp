// ======================================================================
// \title Os/FileSystem.hpp
// \brief Os::FileSystem interface definition
// ======================================================================

#ifndef _OS_FILESYSTEM_HPP_
#define _OS_FILESYSTEM_HPP_

#include <Fw/FPrimeBasicTypes.hpp>
#include <Fw/Types/Assert.hpp>
#include <Os/Directory.hpp>
#include <Os/File.hpp>
#include <Os/Os.hpp>
#include <algorithm>
#include "config/OsSelection.hpp"  // defines the Os::FileSystem alias and OS_FILE_SYSTEM_HEADER; do not remove

namespace Os {

struct FileSystemHandle {};

//! \brief types and constants shared by users of Os::FileSystem and by its implementations
struct FileSystemUserInterface {
    // Size of file chunks to use for file system operations (e.g. copyFile)
    static constexpr FwSizeType FILE_SYSTEM_FILE_CHUNK_SIZE = FW_FILE_CHUNK_SIZE;  //!< Size of file system chunk

    enum Status {
        OP_OK,             //!<  Operation was successful
        ALREADY_EXISTS,    //!<  File already exists
        NO_SPACE,          //!<  No space left
        NO_PERMISSION,     //!<  No permission to write
        NOT_DIR,           //!<  Path is not a directory
        IS_DIR,            //!< Path is a directory
        NOT_EMPTY,         //!<  directory is not empty
        INVALID_PATH,      //!< Path is too long, too many sym links, etc.
        DOESNT_EXIST,      //!<  Path doesn't exist
        FILE_LIMIT,        //!< Too many files or links
        BUSY,              //!< Operand is in use by the system or by a process
        NO_MORE_FILES,     //!<  Directory stream has no more files
        BUFFER_TOO_SMALL,  //!<  Buffer size is too small to hold full path (for getWorkingDirectory)
        EXDEV_ERROR,       // Operation not supported across devices (e.g. rename)
        OVERFLOW_ERROR,    // Operation failed due to overflow in calculation of the result
        NOT_SUPPORTED,     //!<  Operation is not supported by the current implementation
        OTHER_ERROR,       //!<  other OS-specific error
    };

    enum PathType {
        FILE,       //!< Path is a file
        DIRECTORY,  //!< Path is a directory
        OTHER,      //!< Path is not a file or directory, e.g. a socket
        NOT_EXIST,  //!< Path does not exist
    };
};

//! \brief FileSystem class
//!
//! This class provides a common interface for file system operations.
//! This class uses the singleton pattern and should be accessed through
//! its static functions, for example using `Os::FileSystem::removeFile(path)`.
template <typename T>
class FileSystemCommon final : public FileSystemUserInterface {
  private:
    FileSystemCommon();  //!<  Constructor (private because singleton pattern)
  public:
    ~FileSystemCommon();  //!<  Destructor

    //! \brief return the underlying FileSystem handle (implementation specific)
    //! \return internal FileSystem handle representation
    FileSystemHandle* getHandle();

    // ------------------------------------------------------------
    // Implementation-specific FileSystem member functions
    // ------------------------------------------------------------

    //! \brief Remove a directory at the specified path
    //!
    //! It is invalid to pass `nullptr` as the path.
    //!
    //! \param path The path of the directory to remove
    //! \return Status of the operation
    Status _removeDirectory(const char* path);

    //! \brief Remove a file at the specified path
    //!
    //! It is invalid to pass `nullptr` as the path.
    //!
    //! \param path The path of the file to remove
    //! \return Status of the operation
    Status _removeFile(const char* path);

    //! \brief Rename a file from source to destination
    //!
    //! If the rename fails due to a cross-device operation, this function should return EXDEV_ERROR
    //! and moveFile can be used instead to force a copy-and-remove.
    //!
    //! It is invalid to pass `nullptr` as sourcePath or destPath.
    //!
    //! \param sourcePath The path of the source file
    //! \param destPath The path of the destination file
    //! \return Status of the operation
    Status _rename(const char* sourcePath, const char* destPath);

    //! \brief Get filesystem free and total space in bytes on the filesystem containing the specified path
    //!
    //! It is invalid to pass `nullptr` as the path.
    //!
    //! \param path The path on the filesystem to query
    //! \param totalBytes Reference to store the total bytes on the filesystem
    //! \param freeBytes Reference to store the free bytes on the filesystem
    //! \return Status of the operation
    Status _getFreeSpace(const char* path, FwSizeType& totalBytes, FwSizeType& freeBytes);

    //! \brief Get the current working directory
    //!
    //! Writes the current working directory path to the provided buffer of size bufferSize.
    //! If the buffer is too small to hold the full path, the function will return BUFFER_TOO_SMALL.
    //!
    //! It is invalid to pass `nullptr` as the path.
    //! It is invalid to pass a bufferSize of 0.
    //!
    //! \param path Buffer to store the current working directory path
    //! \param bufferSize Size of the buffer
    //! \return Status of the operation
    Status _getWorkingDirectory(char* path, FwSizeType bufferSize);

    //! \brief Change the current working directory to the specified path
    //!
    //! It is invalid to pass `nullptr` as the path.
    //!
    //! \param path The path of the new working directory
    //! \return Status of the operation
    Status _changeWorkingDirectory(const char* path);

    //! \brief Get the type of the path (file, directory, etc.)
    //!
    //! It is invalid to pass `nullptr` as the path.
    //!
    //! \param path The path to check
    //! \param pathType Reference to store the path type
    //! \return Status of the operation
    Status _getPathType(const char* path, PathType& pathType);

    // ------------------------------------------------------------
    // Implementation-specific FileSystem static functions
    // ------------------------------------------------------------
    // These are static variants that are exposed to the user, and call the above member functions

    //! \brief Remove a directory at the specified path
    //!
    //! It is invalid to pass `nullptr` as the path.
    //!
    //! \param path The path of the directory to remove
    //! \return Status of the operation
    static Status removeDirectory(const char* path);

    //! \brief Remove a file at the specified path
    //!
    //! It is invalid to pass `nullptr` as the path.
    //!
    //! \param path The path of the file to remove
    //! \return Status of the operation
    static Status removeFile(const char* path);

    //! \brief Rename a file from source to destination
    //!
    //! If the rename fails due to a cross-device operation, this function should return EXDEV_ERROR
    //! and moveFile can be used instead to force a copy-and-remove.
    //!
    //! It is invalid to pass `nullptr` as sourcePath or destPath.
    //!
    //! \param sourcePath The path of the source file
    //! \param destPath The path of the destination file
    //! \return Status of the operation
    static Status rename(const char* sourcePath, const char* destPath);

    //! \brief Get filesystem free and total space in bytes on the filesystem containing the specified path
    //!
    //! It is invalid to pass `nullptr` as the path.
    //!
    //! \param path The path on the filesystem to query
    //! \param totalBytes Reference to store the total bytes on the filesystem
    //! \param freeBytes Reference to store the free bytes on the filesystem
    //! \return Status of the operation
    static Status getFreeSpace(const char* path, FwSizeType& totalBytes, FwSizeType& freeBytes);

    //! \brief Get the current working directory
    //!
    //! Writes the current working directory path to the provided buffer of size bufferSize.
    //! If the buffer is too small to hold the full path, the function will return BUFFER_TOO_SMALL.
    //!
    //! It is invalid to pass `nullptr` as the path.
    //! It is invalid to pass a bufferSize of 0.
    //!
    //! \param path Buffer to store the current working directory path
    //! \param bufferSize Size of the buffer
    //! \return Status of the operation
    static Status getWorkingDirectory(char* path, FwSizeType bufferSize);

    //! \brief Change the current working directory to the specified path
    //!
    //! It is invalid to pass `nullptr` as the path.
    //!
    //! \param path The path of the new working directory
    //! \return Status of the operation
    static Status changeWorkingDirectory(const char* path);

    // ------------------------------------------------------------
    // Additional functions built on top of OS-specific operations
    // ------------------------------------------------------------

    //! \brief Return true if the path exists, false otherwise
    //!
    //! It is invalid to pass `nullptr` as the path.
    //!
    //! \param path The path to check for existence
    //! \return True if the path exists, false otherwise
    static bool exists(const char* path);

    //! \brief Return the type of the path (file, directory, or doesn't exist)
    //!
    //! It is invalid to pass `nullptr` as the path.
    //!
    //! \param path The path to check for existence
    //! \return PathType enum representing the type of the path (FILE, DIRECTORY, NOT_EXIST)
    static PathType getPathType(const char* path);

    //! \brief Touch a file at the specified path, creating it if it doesn't exist
    //!
    //! It is invalid to pass `nullptr` as the path.
    //!
    //! \param path The path of the file to touch
    //! \return Status of the operation
    static Status touch(const char* path);

    //! \brief Create a new directory at the specified path.
    //!
    //! The optional errorIfAlreadyExists (default=false) parameter can be set to true
    //! to return an error status if the directory already exists.
    //!
    //! It is invalid to pass `nullptr` as the path.
    //!
    //! \param path The path where the new directory will be created
    //! \param errorIfAlreadyExists If true, returns an error if the directory already exists
    //! \return Status of the operation
    static Status createDirectory(const char* path, bool errorIfAlreadyExists = false);

    //! \brief Append the source file to the destination file
    //!
    //! This function opens both files, and iteratively reads the source by chunks and writes
    //! chunks to the destination.
    //! If the destination file does not exist and createMissingDest is true, a new file is created.
    //!
    //! It is invalid to pass `nullptr` as either the source or destination path.
    //!
    //! \param sourcePath The path of the source file
    //! \param destPath The path of the destination file
    //! \param createMissingDest If true, creates a new file if the destination doesn't exist
    //! \return Status of the operation
    static Status appendFile(const char* sourcePath, const char* destPath, bool createMissingDest = false);

    //! \brief Copy a file from the source path to the destination path
    //!
    //! This function opens both files, and iteratively reads the source by chunks and writes
    //! chunks to the destination.
    //!
    //! It is invalid to pass `nullptr` as either the source or destination path.
    //!
    //! \param sourcePath The path of the source file
    //! \param destPath The path of the destination file
    //! \return Status of the operation
    static Status copyFile(const char* sourcePath, const char* destPath);

    //! \brief Move a file from sourcePath to destPath
    //!
    //! This is done by first trying to rename, and if renaming fails,
    //! copy it and then remove the original
    //!
    //! It is invalid to pass `nullptr` as either the source or destination path.
    //!
    //! \param sourcePath The path of the source file
    //! \param destPath The path of the destination file
    //! \return Status of the operation
    static Status moveFile(const char* sourcePath, const char* destPath);

    //! \brief Get the size of the file (in bytes) at the specified path
    //!
    //! It is invalid to pass `nullptr` as the path.
    //!
    //! \param path The path of the file
    //! \param size Reference to store the size of the file
    //! \return Status of the operation
    static Status getFileSize(const char* path, FwSizeType& size);

  public:
    //! \brief initialize singleton
    static void init();

    //! \brief get a reference to singleton
    //! \return reference to singleton
    static FileSystemCommon& getSingleton();

  private:
    // ------------------------------------------------------------
    // Internal helper functions
    // ------------------------------------------------------------

    //! \brief Convert a File::Status to a FileSystem::Status
    static Status handleFileError(File::Status fileStatus);

    //! \brief Convert a Directory::Status to a FileSystem::Status
    static Status handleDirectoryError(Directory::Status dirStatus);

    //! \brief A helper function that writes all the file information in the source
    //! file to the destination file (replaces/appends to end/etc. depending
    //! on destination file mode).
    //!
    //! Files must already be open and will remain open after this function
    //! completes.
    //!
    //! @param source File to copy data from
    //! @param destination File to copy data to
    //! @param size The number of bytes to copy
    static Status copyFileData(File& source, File& destination, FwSizeType size);

  private:
    // This section is used to store the implementation-defined FileSystem handle. To Os::FileSystem and fprime, this
    // type is opaque and thus normal allocation cannot be done. Instead, we allow the implementor to store then handle
    // in the byte-array here and set `handle` to that address for storage.

    T m_delegate;
};

}  // namespace Os

// Validate that OS_FILE_SYSTEM_HEADER was defined by config/OsSelection.hpp
#ifndef OS_FILE_SYSTEM_HEADER
#error "OS_FILE_SYSTEM_HEADER must be defined in config/OsSelection.hpp"
#endif

#include OS_FILE_SYSTEM_HEADER

namespace Os {

template <typename T>
FileSystemCommon<T>::FileSystemCommon() : m_delegate() {}

template <typename T>
FileSystemCommon<T>::~FileSystemCommon() {}

template <typename T>
FileSystemHandle* FileSystemCommon<T>::getHandle() {
    return this->m_delegate.getHandle();
}

template <typename T>
typename FileSystemCommon<T>::Status FileSystemCommon<T>::_removeDirectory(const char* path) {
    FW_ASSERT(path != nullptr);
    return this->m_delegate._removeDirectory(path);
}

template <typename T>
typename FileSystemCommon<T>::Status FileSystemCommon<T>::_removeFile(const char* path) {
    FW_ASSERT(path != nullptr);
    return this->m_delegate._removeFile(path);
}

template <typename T>
typename FileSystemCommon<T>::Status FileSystemCommon<T>::_rename(const char* sourcePath, const char* destPath) {
    FW_ASSERT(sourcePath != nullptr);
    FW_ASSERT(destPath != nullptr);
    return this->m_delegate._rename(sourcePath, destPath);
}

template <typename T>
typename FileSystemCommon<T>::Status FileSystemCommon<T>::_getPathType(const char* path, PathType& pathType) {
    FW_ASSERT(path != nullptr);
    return this->m_delegate._getPathType(path, pathType);
}

template <typename T>
typename FileSystemCommon<T>::Status FileSystemCommon<T>::_getWorkingDirectory(char* path, FwSizeType bufferSize) {
    FW_ASSERT(path != nullptr);
    FW_ASSERT(bufferSize > 0);  // because bufferSize=0 would trigger a malloc in some implementations (e.g. Posix)
    return this->m_delegate._getWorkingDirectory(path, bufferSize);
}

template <typename T>
typename FileSystemCommon<T>::Status FileSystemCommon<T>::_changeWorkingDirectory(const char* path) {
    FW_ASSERT(path != nullptr);
    return this->m_delegate._changeWorkingDirectory(path);
}

template <typename T>
typename FileSystemCommon<T>::Status FileSystemCommon<T>::_getFreeSpace(const char* path,
                                                                        FwSizeType& totalBytes,
                                                                        FwSizeType& freeBytes) {
    FW_ASSERT(path != nullptr);
    return this->m_delegate._getFreeSpace(path, totalBytes, freeBytes);
}

template <typename T>
void FileSystemCommon<T>::init() {
    // Force trigger on the fly singleton setup
    (void)FileSystemCommon::getSingleton();
}

template <typename T>
FileSystemCommon<T>& FileSystemCommon<T>::getSingleton() {
    static FileSystemCommon s_singleton;
    return s_singleton;
}

// ------------------------------------------------------------
// Static functions calling implementation-specific operations
// ------------------------------------------------------------

template <typename T>
typename FileSystemCommon<T>::Status FileSystemCommon<T>::removeDirectory(const char* path) {
    return FileSystemCommon::getSingleton()._removeDirectory(path);
}

template <typename T>
typename FileSystemCommon<T>::Status FileSystemCommon<T>::removeFile(const char* path) {
    return FileSystemCommon::getSingleton()._removeFile(path);
}

template <typename T>
typename FileSystemCommon<T>::Status FileSystemCommon<T>::rename(const char* sourcePath, const char* destPath) {
    return FileSystemCommon::getSingleton()._rename(sourcePath, destPath);
}

template <typename T>
typename FileSystemCommon<T>::Status FileSystemCommon<T>::getWorkingDirectory(char* path, FwSizeType bufferSize) {
    return FileSystemCommon::getSingleton()._getWorkingDirectory(path, bufferSize);
}

template <typename T>
typename FileSystemCommon<T>::Status FileSystemCommon<T>::changeWorkingDirectory(const char* path) {
    return FileSystemCommon::getSingleton()._changeWorkingDirectory(path);
}

template <typename T>
typename FileSystemCommon<T>::Status FileSystemCommon<T>::getFreeSpace(const char* path,
                                                                       FwSizeType& totalBytes,
                                                                       FwSizeType& freeBytes) {
    return FileSystemCommon::getSingleton()._getFreeSpace(path, totalBytes, freeBytes);
}

// ------------------------------------------------------------
// Additional functions built on top of OS-specific operations
// ------------------------------------------------------------

template <typename T>
typename FileSystemCommon<T>::Status FileSystemCommon<T>::createDirectory(const char* path, bool errorIfAlreadyExists) {
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

template <typename T>
typename FileSystemCommon<T>::Status FileSystemCommon<T>::touch(const char* path) {
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

template <typename T>
typename FileSystemCommon<T>::PathType FileSystemCommon<T>::getPathType(const char* path) {
    FW_ASSERT(path != nullptr);
    PathType pathType;
    Status status = getSingleton()._getPathType(path, pathType);
    if (status != Status::OP_OK) {
        return PathType::NOT_EXIST;
    }
    return pathType;
}  // end getPathType

template <typename T>
bool FileSystemCommon<T>::exists(const char* path) {
    return FileSystemCommon::getPathType(path) != PathType::NOT_EXIST;
}  // end exists

template <typename T>
typename FileSystemCommon<T>::Status FileSystemCommon<T>::copyFile(const char* sourcePath, const char* destPath) {
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

template <typename T>
typename FileSystemCommon<T>::Status FileSystemCommon<T>::appendFile(const char* sourcePath,
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

template <typename T>
typename FileSystemCommon<T>::Status FileSystemCommon<T>::moveFile(const char* source, const char* destination) {
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

template <typename T>
typename FileSystemCommon<T>::Status FileSystemCommon<T>::getFileSize(const char* path, FwSizeType& size) {
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

template <typename T>
typename FileSystemCommon<T>::Status FileSystemCommon<T>::handleFileError(File::Status fileStatus) {
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

template <typename T>
typename FileSystemCommon<T>::Status FileSystemCommon<T>::handleDirectoryError(Directory::Status dirStatus) {
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

template <typename T>
typename FileSystemCommon<T>::Status FileSystemCommon<T>::copyFileData(File& source,
                                                                       File& destination,
                                                                       FwSizeType size) {
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

#endif
