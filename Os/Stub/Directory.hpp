// ======================================================================
// \title Os/Stub/Directory.hpp
// \brief stub definitions for Os::Directory
// ======================================================================
#ifndef OS_STUB_DIRECTORY_HPP
#define OS_STUB_DIRECTORY_HPP

#include "Os/Directory.hpp"
namespace Os {
namespace Stub {
namespace Directory {

struct StubDirectoryHandle : public DirectoryHandle {};

//! \brief stub implementation of Os::Directory
//!
//! Stub implementation of `DirectoryInterface` usable through the link-time delegate or directly as
//! `Os::Directory`. Every operation fails: `open` returns NOT_SUPPORTED, so the directory is never open.
//!
class StubDirectory final : public DirectoryInterface {
  public:
    //! \brief constructor
    StubDirectory() = default;

    //! \brief destructor
    ~StubDirectory() override = default;

    //! \brief Check if Directory is open or not
    //! \return false: the stub directory is never open
    bool isOpen() const override;

    //! \brief return the underlying Directory handle (implementation specific)
    //! \return internal Directory handle representation
    DirectoryHandle* getHandle() override;

    // Bring the base class's read(Fw::StringBase&) overload back into scope; it would
    // otherwise be hidden by the read(char*, FwSizeType) override declared below.
    using DirectoryInterface::read;

    // ------------------------------------------------------------
    // Implementation-specific Directory member functions
    // ------------------------------------------------------------

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
    Status open(const char* path, OpenMode mode) override;

    //! \brief Rewind directory stream
    //!
    //! Each read operation moves the seek position forward. This function resets the seek position to the beginning.
    //!
    //! \return status of the operation
    Status rewind() override;

    //! \brief Get next filename from directory stream
    //!
    //! The stub directory is never open, so this returns NOT_OPENED without writing to fileNameBuffer.
    //!
    //! It is invalid to pass `nullptr` as fileNameBuffer.
    //!
    //! \param fileNameBuffer: buffer to store filename
    //! \param buffSize: size of fileNameBuffer
    //! \return status of the operation
    Status read(char* fileNameBuffer, FwSizeType buffSize) override;

    //! \brief Close directory
    void close() override;

  private:
    //! Handle for StubDirectory
    StubDirectoryHandle m_handle;
};

}  // namespace Directory
}  // namespace Stub
}  // namespace Os
#endif  // OS_STUB_DIRECTORY_HPP
