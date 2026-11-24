// ======================================================================
// \title Os/Interfaces/FileUserInterface.hpp
// \brief types shared by Os::File users and implementations
//
// Includes no other Os service header, so both Os/File.hpp and the Os::File implementations may include it.
// ======================================================================
#ifndef OS_INTERFACES_FILEUSERINTERFACE_HPP_
#define OS_INTERFACES_FILEUSERINTERFACE_HPP_

#include <Fw/FPrimeBasicTypes.hpp>
#include <Fw/Types/Assert.hpp>
#include <Fw/Types/ConstStringBase.hpp>
#include <Fw/Types/StringUtils.hpp>
#include <Os/Os.hpp>
#include <Utils/Hash/Hash.hpp>
#include <algorithm>
#include <config/FppConstantsAc.hpp>

namespace Os {

//! \brief base implementation of FileHandle
//!
struct FileHandle {};

//! \brief types and constants shared by users of Os::File and by its implementations
struct FileUserInterface {
    enum Mode {
        OPEN_NO_MODE,     //!< File mode not yet selected
        OPEN_READ,        //!< Open file for reading
        OPEN_CREATE,      //!< Open file for writing and truncates file if it exists, ie same flags as creat()
        OPEN_WRITE,       //!< Open file for writing
        OPEN_SYNC_WRITE,  //!< Open file for writing; writes don't return until data is on disk
        OPEN_APPEND,      //!< Open file for appending
        MAX_OPEN_MODE     //!< Maximum value of mode
    };

    enum Status {
        OP_OK,              //!< Operation was successful
        DOESNT_EXIST,       //!< File doesn't exist (for read)
        NO_SPACE,           //!< No space left
        NO_PERMISSION,      //!< No permission to read/write file
        BAD_SIZE,           //!< Invalid size parameter
        NOT_OPENED,         //!< file hasn't been opened yet
        FILE_EXISTS,        //!< file already exist (for CREATE with O_EXCL enabled)
        NOT_SUPPORTED,      //!< Kernel or file system does not support operation
        INVALID_MODE,       //!< Mode for file access is invalid for current operation
        INVALID_ARGUMENT,   //!< Invalid argument passed in
        NO_MORE_RESOURCES,  //!< No more available resources
        OTHER_ERROR,        //!< A catch-all for other errors. Have to look in implementation-specific code
        OUTSIDE_SANDBOX,    //!< Path falls outside the configured sandbox directory
        MAX_STATUS          //!< Maximum value of status
    };

    enum OverwriteType {
        NO_OVERWRITE,  //!< Do NOT overwrite existing files
        OVERWRITE,     //!< Overwrite file when it exists and creation was requested
        MAX_OVERWRITE_TYPE
    };

    enum SeekType {
        RELATIVE,  //!< Relative seek from current file offset
        ABSOLUTE,  //!< Absolute seek from beginning of file
        MAX_SEEK_TYPE
    };

    enum WaitType {
        NO_WAIT,  //!< Do not wait for read/write operation to finish
        WAIT,     //!< Do wait for read/write operation to finish
        MAX_WAIT_TYPE
    };
};

}  // namespace Os

#endif  // OS_INTERFACES_FILEUSERINTERFACE_HPP_
