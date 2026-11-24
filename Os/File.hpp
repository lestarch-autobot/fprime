// ======================================================================
// \title Os/File.hpp
// \brief common function definitions for Os::File
// ======================================================================
#ifndef Os_File_hpp_
#define Os_File_hpp_

#include <Fw/FPrimeBasicTypes.hpp>
#include <Fw/Types/Assert.hpp>
#include <Fw/Types/ConstStringBase.hpp>
#include <Fw/Types/StringUtils.hpp>
#include <Os/Os.hpp>
#include <Utils/Hash/Hash.hpp>
#include <algorithm>
#include <config/FppConstantsAc.hpp>
#include "config/OsSelection.hpp"  // defines the Os::File alias and OS_FILE_HEADER; do not remove

// Forward declaration for UTs
namespace Os {
namespace Test {
namespace FileTest {
struct Tester;
}
}  // namespace Test
}  // namespace Os

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

template <typename T>
class FileCommon final : public FileUserInterface {
    friend struct Os::Test::FileTest::Tester;

  public:
    //! \brief constructor
    //!
    FileCommon();
    //! \brief destructor
    //!
    //! Destructor closes the file if it is open
    ~FileCommon();

    //! \brief copy constructor that copies the internal representation
    FileCommon(const FileCommon& other);

    //! \brief assignment operator that copies the internal representation
    FileCommon& operator=(const FileCommon& other);

    //! \brief determine if the file is open
    //! \return true if file is open, false otherwise
    //!
    bool isOpen() const;

    // ------------------------------------
    // Functions supplying default values
    // ------------------------------------

    //! \brief open file with supplied path and mode
    //!
    //! Open the file passed in with the given mode. Opening files with `OPEN_CREATE` mode will not clobber existing
    //! files. Use other `open` method to set overwrite flag and clobber existing files. The status of the open
    //! request is returned from the function call. Delegates to the chosen implementation's `open` function.
    //!
    //! It is invalid to send `nullptr` as the path.
    //! It is invalid to supply `mode` as a non-enumerated value.
    //!
    //! \param path: c-string of path to open
    //! \param mode: file operation mode
    //! \return: status of the open
    //!
    Os::FileUserInterface::Status open(const char* path, Mode mode);

    //! \brief open file with supplied path, bounded length, and mode
    //!
    //! Open the file passed in with the given mode. The path length is bounded by `length`.
    //! Opening files with `OPEN_CREATE` mode will not clobber existing files. Use the overload
    //! accepting `OverwriteType` to set overwrite flag and clobber existing files.
    //!
    //! It is invalid to send `nullptr` as the path.
    //! It is invalid to supply `mode` as a non-enumerated value.
    //! It is invalid for the path to not be null-terminated within `length` characters.
    //!
    //! \param path: c-string of path to open
    //! \param length: bound on the path buffer size
    //! \param mode: file operation mode
    //! \return: status of the open
    //!
    Os::FileUserInterface::Status open(const char* path, FwSizeType length, Mode mode);

    //! \brief open file with supplied string path and mode
    //!
    //! Open the file passed in with the given mode. Opening files with `OPEN_CREATE` mode will not clobber existing
    //! files. Use the overload accepting `OverwriteType` to set overwrite flag and clobber existing files.
    //!
    //! It is invalid to supply `mode` as a non-enumerated value.
    //!
    //! \param path: ConstStringBase reference of path to open
    //! \param mode: file operation mode
    //! \return: status of the open
    //!
    Os::FileUserInterface::Status open(const Fw::ConstStringBase& path, Mode mode);

    //! \brief open file with supplied string path, mode, and overwrite type
    //!
    //! Open the file passed in with the given mode. If overwrite is set to OVERWRITE, then opening files in
    //! OPEN_CREATE mode will clobber existing files. Set overwrite to NO_OVERWRITE to preserve existing files.
    //!
    //! It is invalid to supply `mode` as a non-enumerated value.
    //! It is invalid to supply `overwrite` as a non-enumerated value.
    //!
    //! \param path: ConstStringBase reference of path to open
    //! \param mode: file operation mode
    //! \param overwrite: overwrite existing file on create
    //! \return: status of the open
    //!
    Os::FileUserInterface::Status open(const Fw::ConstStringBase& path, Mode mode, OverwriteType overwrite);

    //! \brief read data from this file into supplied buffer bounded by size
    //!
    //! Read data from this file up to the `size` and store it in `buffer`.  This version will
    //! will block until the requested size has been read successfully read or the end of the file has been
    //! reached.
    //!
    //! `size` will be updated to the count of bytes actually read. Status will reflect the success/failure of
    //! the read operation.
    //!
    //! It is invalid to pass `nullptr` to this function call.
    //! It is invalid to pass a negative `size`.
    //!
    //! \param buffer: memory location to store data read from file
    //! \param size: size of data to read
    //! \return OP_OK on success otherwise error status
    //!
    Status read(U8* buffer, FwSizeType& size);

    //! \brief write data to this file from the supplied buffer bounded by size
    //!
    //! Write data from `buffer` up to the `size` and store it in this file. This call
    //! will block until the requested size has been written. Otherwise, this call will write without blocking.
    //!
    //! `size` will be updated to the count of bytes actually written. Status will reflect the success/failure of
    //! the write operation.
    //!
    //! It is invalid to pass `nullptr` to this function call.
    //! It is invalid to pass a negative `size`.
    //!
    //! \param buffer: memory location of data to write to file
    //! \param size: size of data to write
    //! \return OP_OK on success otherwise error status
    //!
    Status write(const U8* buffer, FwSizeType& size);

    // ------------------------------------
    // Functions overrides
    // ------------------------------------

    //! \brief open file with supplied path and mode
    //!
    //! Open the file passed in with the given mode. If overwrite is set to OVERWRITE, then opening files in
    //! OPEN_CREATE mode will clobber existing files. Set overwrite to NO_OVERWRITE to preserve existing files.
    //! The status of the open request is returned from the function call. Delegates to the chosen
    //! implementation's `open` function.
    //!
    //! It is invalid to send `nullptr` as the path.
    //! It is invalid to supply `mode` as a non-enumerated value.
    //! It is invalid to supply `overwrite` as a non-enumerated value.
    //!
    //! \param path: c-string of path to open
    //! \param mode: file operation mode
    //! \param overwrite: overwrite existing file on create
    //! \return: status of the open
    //!
    Os::FileUserInterface::Status open(const char* path, Mode mode, OverwriteType overwrite);

    //! \brief open file with supplied path, bounded length, mode, and overwrite type
    //!
    //! Open the file passed in with the given mode. The path length is bounded by `length`.
    //! If overwrite is set to OVERWRITE, then opening files in OPEN_CREATE mode will clobber
    //! existing files. Set overwrite to NO_OVERWRITE to preserve existing files. This is the
    //! core open implementation to which all other open overloads delegate.
    //!
    //! It is invalid to send `nullptr` as the path.
    //! It is invalid to supply `mode` as a non-enumerated value.
    //! It is invalid to supply `overwrite` as a non-enumerated value.
    //! It is invalid for the path to not be null-terminated within `length` characters.
    //!
    //! \param path: c-string of path to open
    //! \param length: bound on the path buffer size
    //! \param mode: file operation mode
    //! \param overwrite: overwrite existing file on create
    //! \return: status of the open
    //!
    Os::FileUserInterface::Status open(const char* path, FwSizeType length, Mode mode, OverwriteType overwrite);

    //! \brief close the file, if not opened then do nothing
    //!
    //! Closes the file, if open. Otherwise this function does nothing. Delegates to the chosen implementation's
    //! `closeInternal` function. `mode` is set to `OPEN_NO_MODE`.
    //!
    void close();

    //! \brief get size of currently open file
    //!
    //! Get the size of the currently open file and fill the size parameter. Return status of the operation.
    //! \param size: output parameter for size.
    //! \return OP_OK on success otherwise error status
    //!
    Status size(FwSizeType& size_result);

    //! \brief get file pointer position of the currently open file
    //!
    //! Get the current position of the read/write pointer of the open file.
    //! \param position: output parameter for size.
    //! \return OP_OK on success otherwise error status
    //!
    Status position(FwSizeType& position_result);

    //! \brief pre-allocate file storage
    //!
    //! Pre-allocates file storage with at least `length` storage starting at `offset`. No-op on implementations
    //! that cannot pre-allocate.
    //!
    //! It is invalid to pass a negative `offset`.
    //! It is invalid to pass a negative `length`.
    //!
    //! \param offset: offset into file
    //! \param length: length after offset to preallocate
    //! \return OP_OK on success otherwise error status
    //!
    Status preallocate(FwSizeType offset, FwSizeType length);

    //! \brief seek the file pointer to the given offset
    //!
    //! Seek the file pointer to the given `offset`. If `seekType` is set to `ABSOLUTE` then the offset is calculated
    //! from the start of the file, and if it is set to `RELATIVE` it is calculated from the current position.
    //!
    //! \param offset: offset to seek to
    //! \param seekType: `ABSOLUTE` for seeking from beginning of file, `RELATIVE` to use current position.
    //! \return OP_OK on success otherwise error status
    //!
    Status seek(FwSignedSizeType offset, SeekType seekType);

    //! \brief seek the file pointer to the given offset absolutely with the full range
    //!
    //! Seek the file pointer to the given `offset` absolutely from the beginning of the file. This function is
    //! equivalent to calling `seek` with `ABSOLUTE` as the `seekType` with the exception that it can handle the
    //! full range of `FwSizeType` values as returned by `size` and `position` calls.
    //!
    //! Internally, it will perform multiple seeks to reach the desired offset while never exceeding the signed
    //! limit of the basic `seek` function.
    //!
    //! \param offset_unsigned: offset to absolutely seek to
    //! \return OP_OK on success otherwise error status
    Status seek_absolute(FwSizeType offset_unsigned);

    //! \brief flush file contents to storage
    //!
    //! Flushes the file contents to storage (i.e. out of the OS cache to disk). Does nothing in implementations
    //! that do not support flushing.
    //!
    //! \return OP_OK on success otherwise error status
    //!
    Status flush();

    //! \brief read data from this file into supplied buffer bounded by size
    //!
    //! Read data from this file up to the `size` and store it in `buffer`.  When `wait` is set to `WAIT`, this
    //! will block until the requested size has been read successfully read or the end of the file has been
    //! reached. When `wait` is set to `NO_WAIT` it will return whatever data is currently available.
    //!
    //! `size` will be updated to the count of bytes actually read. Status will reflect the success/failure of
    //! the read operation.
    //!
    //! It is invalid to pass `nullptr` to this function call.
    //! It is invalid to pass a negative `size`.
    //! It is invalid to supply wait as a non-enumerated value.
    //!
    //! \param buffer: memory location to store data read from file
    //! \param size: size of data to read
    //! \param wait: `WAIT` to wait for data, `NO_WAIT` to return what is currently available

    //!
    Status read(U8* buffer, FwSizeType& size, WaitType wait);

    //! \brief read a line from the file using `\n` as the delimiter
    //!
    //! Reads a single line from the file including the terminating '\n'. This will return an error if no line is
    //! found within the specified buffer size. In the case of EOF, the line is read without the terminating '\n'.
    //!
    //! In the case of an error, this function will seek to the original location in the file. Otherwise, the
    //! pointer will point to the first character after the `\n` or EOF in the case of no `\n`.
    //!
    //! It is invalid to send a null buffer.
    //! It is invalid to send a size less than 0.
    //! It is an error if the file is not opened for reading.
    //!
    //! \param buffer: memory location to store data read from file
    //! \param size: maximum size of buffer to store the new line
    //! \param wait: `WAIT` to wait for data, `NO_WAIT` to return what is currently available
    //! \return OP_OK on success otherwise error status
    Status readline(U8* buffer, FwSizeType& size, WaitType wait);

    //! \brief read data from this file into supplied buffer bounded by size
    //!
    //! Write data to this file up to the `size` from the `buffer`.  When `wait` is set to `WAIT`, this
    //! will block until the requested size has been written successfully to disk. When `wait` is set to
    //! `NO_WAIT` it will return once the data is sent to the OS.
    //!
    //! `size` will be updated to the count of bytes actually written. Status will reflect the success/failure of
    //! the read operation.
    //!
    //! It is invalid to pass `nullptr` to this function call.
    //! It is invalid to pass a negative `size`.
    //! It is invalid to supply wait as a non-enumerated value.
    //!
    //! \param buffer: memory location to store data read from file
    //! \param size: size of data to read
    //! \param wait: `WAIT` to wait for data to write to disk, `NO_WAIT` to return what is currently available
    //! \return OP_OK on success otherwise error status
    //!
    Status write(const U8* buffer, FwSizeType& size, WaitType wait);

    //! \brief returns the raw file handle
    //!
    //! Gets the raw file handle from the implementation. Note: users must include the implementation specific
    //! header to make any real use of this handle. Otherwise it//!must* be passed as an opaque type.
    //!
    //! \return raw file handle
    //!
    FileHandle* getHandle();

    //! \brief calculate the CRC32 of the entire file
    //!
    //! Calculates the CRC32 of the file's contents. The `crc` parameter will be updated to contain the CRC or 0 on
    //! failure. Status will represent failure conditions. This call will be decomposed into calculations on
    //! sections of the file `FW_FILE_CHUNK_SIZE` bytes long.
    //!
    //! This function requires that the file already be opened for "READ" mode.
    //!
    //! On error crc will be set to 0.
    //!
    //! \note: the file pointer will be positioned at the end of the file after this call.
    //!
    //! This function is equivalent to the following pseudo-code:
    //!
    //! ```
    //! U32 crc;
    //! do {
    //!     size = FW_FILE_CHUNK_SIZE;
    //!     m_file.incrementalCrc(size);
    //! while (size == FW_FILE_CHUNK_SIZE);
    //! m_file.finalize(crc);
    //! ```
    //! \param crc: U32 bit value to fill with CRC
    //! \return OP_OK on success otherwise error status
    //!
    Status calculateCrc(U32& crc);

    //! \brief calculate the CRC32 of the next section of data
    //!
    //! Starting at the current file pointer, this will add `size` bytes of data to the currently calculated CRC.
    //! Call `finalizeCrc` to retrieve the CRC or `calculateCrc` to perform a CRC on the entire file. This call will
    //! not block waiting for data on the underlying read, nor will it reset the file position pointer. On error,
    //! the current CRC results should be discarded by reopening the file or calling `finalizeCrc` and
    //! discarding its result. `size` will be updated with the `size` actually read and used in the CRC calculation.
    //!
    //! This function requires that the file already be opened for "READ" mode.
    //!
    //! It is illegal for size to be less than or equal to 0 or greater than FW_FILE_CHUNK_SIZE.
    //!
    //! \param size: size of data to read for CRC
    //! \return: status of the CRC calculation
    //!
    Status incrementalCrc(FwSizeType& size);

    //! \brief finalize and retrieve the CRC value
    //!
    //! Finalizes the CRC computation and returns the CRC value. The `crc` value will be modified to contain the
    //! crc or 0 on error. Note: this will reset any active CRC calculation and effectively re-initializes any
    //! `incrementalCrc` calculation.
    //!
    //! On error crc will be set to 0.
    //!
    //! \param crc: value to fill
    //! \return status of the CRC calculation
    //!
    Status finalizeCrc(U32& crc);

  private:
    static const U32 INITIAL_CRC = 0xFFFFFFFF;  //!< Initial value for CRC calculation

    Mode m_mode = Mode::OPEN_NO_MODE;  //!< Stores mode for error checking

    Utils::Hash m_hash;  //!< Hash object for incremental CRC calculation
    U8 m_crc_buffer[FW_FILE_CHUNK_SIZE];

    // This section is used to store the implementation-defined file handle. To Os::File and fprime, this type is
    // opaque and thus normal allocation cannot be done. Instead, we allow the implementor to store then handle in
    // the byte-array here and set `handle` to that address for storage.
    //
    T m_delegate;  //!< Delegate for the real implementation
};

}  // namespace Os
// Validate that OS_FILE_HEADER was defined by config/OsSelection.hpp
#ifndef OS_FILE_HEADER
#error "OS_FILE_HEADER must be defined in config/OsSelection.hpp"
#endif

#include OS_FILE_HEADER

namespace Os {

template <typename T>
FileCommon<T>::FileCommon() : m_crc_buffer(), m_delegate() {}

template <typename T>
FileCommon<T>::~FileCommon() {
    if (this->m_mode != OPEN_NO_MODE) {
        this->close();
    }
}

template <typename T>
FileCommon<T>::FileCommon(const FileCommon& other)
    : m_mode(other.m_mode), m_hash(other.m_hash), m_crc_buffer(), m_delegate(other.m_delegate) {}

template <typename T>
FileCommon<T>& FileCommon<T>::operator=(const FileCommon& other) {
    if (this != &other) {
        // The delegate below is assigned over the existing one. Any file this object currently
        // holds must be closed first or its handle is orphaned permanently.
        if (this->m_mode != OPEN_NO_MODE) {
            this->close();
        }
        this->m_mode = other.m_mode;
        this->m_hash = other.m_hash;
        this->m_delegate = other.m_delegate;
    }
    return *this;
}

template <typename T>
typename FileCommon<T>::Status FileCommon<T>::open(const CHAR* filepath, FileCommon<T>::Mode requested_mode) {
    return this->open(filepath, requested_mode, OverwriteType::NO_OVERWRITE);
}

template <typename T>
typename FileCommon<T>::Status FileCommon<T>::open(const CHAR* filepath,
                                                   FileCommon::Mode requested_mode,
                                                   FileCommon::OverwriteType overwrite) {
    FW_ASSERT(nullptr != filepath);
    return this->open(filepath, static_cast<FwSizeType>(FileNameStringSize + 1), requested_mode, overwrite);
}

template <typename T>
typename FileCommon<T>::Status FileCommon<T>::open(const CHAR* filepath,
                                                   FwSizeType length,
                                                   FileCommon<T>::Mode requested_mode) {
    return this->open(filepath, length, requested_mode, OverwriteType::NO_OVERWRITE);
}

template <typename T>
typename FileCommon<T>::Status FileCommon<T>::open(const CHAR* filepath,
                                                   FwSizeType length,
                                                   FileCommon::Mode requested_mode,
                                                   FileCommon::OverwriteType overwrite) {
    FW_ASSERT(nullptr != filepath);
    const FwSizeType string_len = static_cast<FwSizeType>(Fw::StringUtils::string_length(filepath, length));
    FW_ASSERT(string_len < length, static_cast<FwAssertArgType>(string_len), static_cast<FwAssertArgType>(length));
    FW_ASSERT(FileCommon::Mode::OPEN_NO_MODE < requested_mode && FileCommon::Mode::MAX_OPEN_MODE > requested_mode);
    FW_ASSERT((0 <= this->m_mode) && (this->m_mode < Mode::MAX_OPEN_MODE));
    FW_ASSERT((0 <= overwrite) && (overwrite < OverwriteType::MAX_OVERWRITE_TYPE));
    // Check for already opened file
    if (this->isOpen()) {
        return FileCommon::Status::INVALID_MODE;
    }
    FileCommon::Status status = this->m_delegate.open(filepath, requested_mode, overwrite);
    if (status == FileCommon::Status::OP_OK) {
        this->m_mode = requested_mode;
        // Reset any open CRC calculations
        this->m_hash.init();
    }

    return status;
}

template <typename T>
typename FileCommon<T>::Status FileCommon<T>::open(const Fw::ConstStringBase& path,
                                                   FileCommon<T>::Mode requested_mode) {
    return this->open(path.toChar(), static_cast<FwSizeType>(path.getCapacity()), requested_mode,
                      OverwriteType::NO_OVERWRITE);
}

template <typename T>
typename FileCommon<T>::Status FileCommon<T>::open(const Fw::ConstStringBase& path,
                                                   FileCommon::Mode requested_mode,
                                                   FileCommon::OverwriteType overwrite) {
    return this->open(path.toChar(), static_cast<FwSizeType>(path.getCapacity()), requested_mode, overwrite);
}

template <typename T>
void FileCommon<T>::close() {
    FW_ASSERT(this->m_mode < Mode::MAX_OPEN_MODE);
    FW_ASSERT((0 <= this->m_mode) && (this->m_mode < Mode::MAX_OPEN_MODE));
    this->m_delegate.close();
    this->m_mode = Mode::OPEN_NO_MODE;
}

template <typename T>
bool FileCommon<T>::isOpen() const {
    FW_ASSERT((0 <= this->m_mode) && (this->m_mode < Mode::MAX_OPEN_MODE));
    return this->m_mode != Mode::OPEN_NO_MODE;
}

template <typename T>
typename FileCommon<T>::Status FileCommon<T>::size(FwSizeType& size_result) {
    FW_ASSERT((0 <= this->m_mode) && (this->m_mode < Mode::MAX_OPEN_MODE));
    if (OPEN_NO_MODE == this->m_mode) {
        return FileCommon::Status::NOT_OPENED;
    }
    return this->m_delegate.size(size_result);
}

template <typename T>
typename FileCommon<T>::Status FileCommon<T>::position(FwSizeType& position_result) {
    FW_ASSERT((0 <= this->m_mode) && (this->m_mode < Mode::MAX_OPEN_MODE));
    // Check that the file is open before attempting operation
    if (OPEN_NO_MODE == this->m_mode) {
        return FileCommon::Status::NOT_OPENED;
    }
    return this->m_delegate.position(position_result);
}

template <typename T>
typename FileCommon<T>::Status FileCommon<T>::preallocate(FwSizeType offset, FwSizeType length) {
    FW_ASSERT((0 <= this->m_mode) && (this->m_mode < Mode::MAX_OPEN_MODE));
    // Check that the file is open before attempting operation
    if (OPEN_NO_MODE == this->m_mode) {
        return FileCommon::Status::NOT_OPENED;
    } else if (OPEN_READ == this->m_mode) {
        return FileCommon::Status::INVALID_MODE;
    }
    return this->m_delegate.preallocate(offset, length);
}

template <typename T>
typename FileCommon<T>::Status FileCommon<T>::seek(FwSignedSizeType offset, FileCommon<T>::SeekType seekType) {
    FW_ASSERT((0 <= seekType) && (seekType < SeekType::MAX_SEEK_TYPE));
    // Cannot do a seek with a negative offset in absolute mode
    FW_ASSERT((seekType == FileCommon::SeekType::RELATIVE) || (offset >= 0));
    FW_ASSERT((0 <= this->m_mode) && (this->m_mode < Mode::MAX_OPEN_MODE));
    // Check that the file is open before attempting operation
    if (OPEN_NO_MODE == this->m_mode) {
        return FileCommon::Status::NOT_OPENED;
    }
    return this->m_delegate.seek(offset, seekType);
}

template <typename T>
typename FileCommon<T>::Status FileCommon<T>::seek_absolute(FwSizeType offset) {
    FileCommon::Status status = FileCommon::Status::OTHER_ERROR;
    // If the offset can be represented by a signed value, then we can perform a single seek
    if (static_cast<FwSizeType>(std::numeric_limits<FwSignedSizeType>::max()) >= offset) {
        // Check that the bounding above is correct
        FW_ASSERT(static_cast<FwSignedSizeType>(offset) >= 0);
        status = this->seek(static_cast<FwSignedSizeType>(offset), FileCommon::SeekType::ABSOLUTE);
    }
    // Otherwise, a full seek to any value represented by FwSizeType can be performed
    // by at most 3 seeks of a FwSignedSizeType. Two half seeks (rounded down) that are
    // strictly bounded by std::numeric_limits<FwSignedSizeType>::max() and one seek of
    // a possibile "odd" byte to ensure odds offsets do not introduce an off-by-one-error.
    // Thus we perform 3 seeks to guarantee that we can reach any position.
    else {
        FwSignedSizeType half_offset = static_cast<FwSignedSizeType>(offset >> 1);
        bool is_odd = (offset % 2) == 1;
        status = this->seek(half_offset, FileCommon::SeekType::ABSOLUTE);
        if (status == FileCommon::Status::OP_OK) {
            status = this->seek(half_offset, FileCommon::SeekType::RELATIVE);
        }
        if (status == FileCommon::Status::OP_OK) {
            status = this->seek((is_odd) ? 1 : 0, FileCommon::SeekType::RELATIVE);
        }
    }
    return status;
}

template <typename T>
typename FileCommon<T>::Status FileCommon<T>::flush() {
    FW_ASSERT(this->m_mode < Mode::MAX_OPEN_MODE);
    // Check that the file is open before attempting operation
    if (OPEN_NO_MODE == this->m_mode) {
        return FileCommon::Status::NOT_OPENED;
    } else if (OPEN_READ == this->m_mode) {
        return FileCommon::Status::INVALID_MODE;
    }
    return this->m_delegate.flush();
}

template <typename T>
typename FileCommon<T>::Status FileCommon<T>::read(U8* buffer, FwSizeType& size) {
    return this->read(buffer, size, WaitType::WAIT);
}

template <typename T>
typename FileCommon<T>::Status FileCommon<T>::read(U8* buffer, FwSizeType& size, FileCommon<T>::WaitType wait) {
    FW_ASSERT(buffer != nullptr);
    FW_ASSERT(this->m_mode < Mode::MAX_OPEN_MODE);
    // Check that the file is open before attempting operation
    if (OPEN_NO_MODE == this->m_mode) {
        size = 0;
        return FileCommon::Status::NOT_OPENED;
    } else if (OPEN_READ != this->m_mode) {
        size = 0;
        return FileCommon::Status::INVALID_MODE;
    }
    return this->m_delegate.read(buffer, size, wait);
}

template <typename T>
typename FileCommon<T>::Status FileCommon<T>::write(const U8* buffer, FwSizeType& size) {
    return this->write(buffer, size, WaitType::WAIT);
}

template <typename T>
typename FileCommon<T>::Status FileCommon<T>::write(const U8* buffer, FwSizeType& size, FileCommon<T>::WaitType wait) {
    FW_ASSERT(buffer != nullptr);
    FW_ASSERT(this->m_mode < Mode::MAX_OPEN_MODE);
    // Check that the file is open before attempting operation
    if (OPEN_NO_MODE == this->m_mode) {
        size = 0;
        return FileCommon::Status::NOT_OPENED;
    } else if (OPEN_READ == this->m_mode) {
        size = 0;
        return FileCommon::Status::INVALID_MODE;
    }
    return this->m_delegate.write(buffer, size, wait);
}

template <typename T>
FileHandle* FileCommon<T>::getHandle() {
    return this->m_delegate.getHandle();
}

template <typename T>
typename FileCommon<T>::Status FileCommon<T>::calculateCrc(U32& crc) {
    FileCommon::Status status = FileCommon::Status::OP_OK;
    FwSizeType size = FW_FILE_CHUNK_SIZE;
    crc = 0;
    for (FwSizeType i = 0; i < std::numeric_limits<FwSizeType>::max(); i++) {
        status = this->incrementalCrc(size);
        // Break on eof or error
        if ((size != FW_FILE_CHUNK_SIZE) || (status != FileCommon::OP_OK)) {
            break;
        }
    }
    // When successful, finalize the CRC
    if (status == FileCommon::OP_OK) {
        status = this->finalizeCrc(crc);
    }
    return status;
}

template <typename T>
typename FileCommon<T>::Status FileCommon<T>::incrementalCrc(FwSizeType& size) {
    FileCommon::Status status = FileCommon::Status::OP_OK;
    FW_ASSERT(size <= FW_FILE_CHUNK_SIZE, FwAssertArgType(size));
    if (OPEN_NO_MODE == this->m_mode) {
        status = FileCommon::Status::NOT_OPENED;
    } else if (OPEN_READ != this->m_mode) {
        status = FileCommon::Status::INVALID_MODE;
    } else {
        // Read data without waiting for additional data to be available
        status = this->read(this->m_crc_buffer, size, FileCommon::WaitType::NO_WAIT);
        if (OP_OK == status) {
            FW_ASSERT(size <= FW_FILE_CHUNK_SIZE, FwAssertArgType(size));
            this->m_hash.update(this->m_crc_buffer, size);
        }
    }
    return status;
}

template <typename T>
typename FileCommon<T>::Status FileCommon<T>::finalizeCrc(U32& crc) {
    FileCommon::Status status = FileCommon::Status::OP_OK;
    this->m_hash.finalize(crc);
    // Historically, the CRC calculation in File omitted the final 1's complement step. Utils::Hash performs that step
    // and as such, we must undo it before returning the value to ensure backwards compatibility.
    crc = ~crc;
    this->m_hash.init();
    return status;
}

template <typename T>
typename FileCommon<T>::Status FileCommon<T>::readline(U8* buffer, FwSizeType& size, FileCommon<T>::WaitType wait) {
    const FwSizeType requested_size = size;
    FW_ASSERT(buffer != nullptr);
    FW_ASSERT(this->m_mode < Mode::MAX_OPEN_MODE);
    // Check that the file is open before attempting operation
    if (OPEN_NO_MODE == this->m_mode) {
        size = 0;
        return FileCommon::Status::NOT_OPENED;
    } else if (OPEN_READ != this->m_mode) {
        size = 0;
        return FileCommon::Status::INVALID_MODE;
    }
    FwSizeType original_location = 0;
    FileCommon::Status status = this->position(original_location);
    if (status != FileCommon::Status::OP_OK) {
        size = 0;
        (void)this->seek_absolute(original_location);
        return status;
    }
    FwSizeType read = 0;
    // Loop reading chunk by chunk
    for (FwSizeType i = 0; i < size; i += read) {
        // read in chunks to avoid large buffer allocations
        FwSizeType current_chunk_size = std::min(size - i, static_cast<FwSizeType>(FW_FILE_CHUNK_SIZE));
        read = current_chunk_size;
        status = this->read(buffer + i, read, wait);
        if (status != FileCommon::Status::OP_OK) {
            // Contract: on error, seek back to the original location
            size = 0;
            (void)this->seek_absolute(original_location);
            return status;
        }
        // EOF break out now
        if (read == 0) {
            size = i;
            return FileCommon::Status::OP_OK;
        }
        // Loop from i to i + current_chunk_size looking for `\n`
        const FwSizeType chunk_end = i + read;
        for (FwSizeType j = i; j < chunk_end; j++) {
            // Newline seek back to after it, return the size read
            if (buffer[j] == '\n') {
                size = j + 1;
                // Ensure that the computation worked and there is not overflow
                FW_ASSERT(size <= requested_size);
                FW_ASSERT(std::numeric_limits<FwSizeType>::max() - size >= original_location);
                (void)this->seek_absolute(original_location + j + 1);
                return FileCommon::Status::OP_OK;
            }
        }
    }
    // Failed to find newline within data available
    // Contract: on error, seek back to the original location
    size = 0;
    (void)this->seek_absolute(original_location);
    return FileCommon::Status::OTHER_ERROR;
}
}  // namespace Os

#endif
