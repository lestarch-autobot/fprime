// ======================================================================
// \title Os/Interfaces/FileSystemUserInterface.hpp
// \brief types shared by Os::FileSystem users and implementations
//
// Includes no other Os service header, so both Os/FileSystem.hpp and the Os::FileSystem implementations may include it.
// ======================================================================
#ifndef OS_INTERFACES_FILESYSTEMUSERINTERFACE_HPP_
#define OS_INTERFACES_FILESYSTEMUSERINTERFACE_HPP_

#include <Fw/FPrimeBasicTypes.hpp>
#include <Fw/Types/Assert.hpp>
#include <Os/Os.hpp>
#include <algorithm>

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

}  // namespace Os

#endif  // OS_INTERFACES_FILESYSTEMUSERINTERFACE_HPP_
