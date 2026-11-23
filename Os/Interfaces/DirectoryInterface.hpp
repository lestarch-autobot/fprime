// ======================================================================
// \title Os/Interfaces/DirectoryInterface.hpp
// \brief pure-virtual interface implemented by Os::Directory implementations
// ======================================================================
// Included before the guard: Os/Directory.hpp includes the selected implementation, which includes this file
#include <Os/Directory.hpp>

#ifndef OS_INTERFACES_DIRECTORYINTERFACE_HPP_
#define OS_INTERFACES_DIRECTORYINTERFACE_HPP_

namespace Os {

class DirectoryInterface : public DirectoryUserInterface {
  public:
    //! \brief default constructor
    DirectoryInterface() = default;

    //! \brief default virtual destructor
    virtual ~DirectoryInterface() = default;

    //! \brief copy constructor is forbidden
    DirectoryInterface(const DirectoryInterface& other) = delete;

    //! \brief assignment operator is forbidden
    DirectoryInterface& operator=(const DirectoryInterface& other) = delete;

    //! \brief return the underlying Directory handle (implementation specific)
    //! \return internal Directory handle representation
    virtual DirectoryHandle* getHandle() = 0;

    // -----------------------------------------------------------------
    // Directory operations to be implemented by an OSAL implementation
    // -----------------------------------------------------------------
    // These functions are to be overridden in each OS implementation
    // See an example in in Os/Posix/Directory.hpp

    //! \brief Open or create a directory
    //!
    //! Using the path provided, this function will open or create a directory.
    //! Use OpenMode::READ to open an existing directory and error if the directory is not found
    //! Use OpenMode::CREATE_IF_MISSING to open a directory, creating the directory if it doesn't exist
    //! Use OpenMode::CREATE_EXCLUSIVE to open a directory, creating the directory and erroring if it already exists
    //!
    //! It is invalid to pass `nullptr` as the path.
    //! It is invalid to supply `mode` as a non-enumerated value.
    //!
    //! \param path: path of directory to open
    //! \param mode: enum (READ, CREATE_IF_MISSING, CREATE_EXCLUSIVE). See notes above for more information
    //! \return status of the operation
    virtual Status open(const char* path, OpenMode mode) = 0;

    //! \brief Rewind directory stream
    //!
    //! Each read operation moves the seek position forward. This function resets the seek position to the beginning.
    //!
    //! \return status of the operation
    virtual Status rewind() = 0;

    //! \brief Get next filename from directory stream
    //!
    //! Write at most buffSize characters of the file name to fileNameBuffer and guarantee null-termination.
    //! This function skips the current directory (.) and parent directory (..) entries.
    //! Returns NO_MORE_FILES if there are no more files to read from the buffer.
    //!
    //! It is invalid to pass `nullptr` as fileNameBuffer.
    //!
    //! \param fileNameBuffer: buffer to store filename
    //! \param buffSize: size of fileNameBuffer
    //! \return status of the operation
    virtual Status read(char* fileNameBuffer, FwSizeType buffSize) = 0;

    //! \brief Get next filename from directory stream and write it to a Fw::StringBase object
    //!
    //! \param filename: Fw::StringBase (or derived) object to store filename in
    //! \return status of the operation
    // virtual Status read(Fw::StringBase& filename) = 0;

    //! \brief Close directory
    virtual void close() = 0;
};

}  // namespace Os

#endif  // OS_INTERFACES_DIRECTORYINTERFACE_HPP_
