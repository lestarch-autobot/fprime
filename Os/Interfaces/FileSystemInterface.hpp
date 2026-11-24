// ======================================================================
// \title Os/Interfaces/FileSystemInterface.hpp
// \brief pure-virtual interface implemented by Os::FileSystem implementations
// ======================================================================
// Included before the guard: Os/FileSystem.hpp includes the selected implementation, which includes this file
#include <Os/FileSystem.hpp>

#ifndef OS_INTERFACES_FILESYSTEMINTERFACE_HPP_
#define OS_INTERFACES_FILESYSTEMINTERFACE_HPP_

namespace Os {

class FileSystemInterface : public FileSystemUserInterface {
  public:
    //! \brief default constructor
    FileSystemInterface() = default;

    //! \brief default virtual destructor
    virtual ~FileSystemInterface() = default;

    //! \brief copy constructor is forbidden
    FileSystemInterface(const FileSystemInterface& other) = delete;

    //! \brief assignment operator is forbidden
    FileSystemInterface& operator=(const FileSystemInterface& other) = delete;  // NO_CODESONAR (cpp:S3657)

    //! \brief return the underlying FileSystem handle (implementation specific)
    //! \return internal FileSystem handle representation
    virtual FileSystemHandle* getHandle() = 0;

    // ------------------------------------------------------------------
    // FileSystem operations to be implemented by an OSAL implementation
    // ------------------------------------------------------------------
    // These functions are to be overridden in each OS implementation
    // See an example in in Os/Posix/FileSystem.hpp

    //! \brief Remove a directory at the specified path
    //! \param path The path of the directory to remove
    //! \return Status of the operation
    virtual Status _removeDirectory(const char* path) = 0;

    //! \brief Remove a file at the specified path
    //! \param path The path of the file to remove
    //! \return Status of the operation
    virtual Status _removeFile(const char* path) = 0;

    //! \brief Rename (or move) a file from source to destination
    //! \param sourcePath The path of the source file
    //! \param destPath The path of the destination file
    //! \return Status of the operation
    virtual Status _rename(const char* sourcePath, const char* destPath) = 0;

    //! \brief Get filesystem free and total space in bytes on the filesystem containing the specified path
    //! \param path The path on the filesystem to query
    //! \param totalBytes Reference to store the total bytes on the filesystem
    //! \param freeBytes Reference to store the free bytes on the filesystem
    //! \return Status of the operation
    virtual Status _getFreeSpace(const char* path, FwSizeType& totalBytes, FwSizeType& freeBytes) = 0;

    //! \brief Get the type of the path (file, directory, etc.)
    //!
    //! It is invalid to pass `nullptr` as the path.
    //!
    //! \param path The path to check
    //! \param pathType Reference to store the path type
    //! \return Status of the operation
    virtual Status _getPathType(const char* path, PathType& pathType) = 0;

    //! \brief Get the current working directory
    //! \param path Buffer to store the current working directory path
    //! \param bufferSize Size of the buffer
    //! \return Status of the operation
    virtual Status _getWorkingDirectory(char* path, FwSizeType bufferSize) = 0;

    //! \brief Change the current working directory to the specified path
    //! \param path The path of the new working directory
    //! \return Status of the operation
    virtual Status _changeWorkingDirectory(const char* path) = 0;
};

}  // namespace Os

#endif  // OS_INTERFACES_FILESYSTEMINTERFACE_HPP_
