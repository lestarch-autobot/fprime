// ======================================================================
// \title Os/Directory.hpp
// \brief Os::Directory interface definition
// ======================================================================

#ifndef _OS_DIRECTORY_HPP_
#define _OS_DIRECTORY_HPP_

#include <Fw/DataStructures/ExternalArray.hpp>
#include <Fw/Deprecate.hpp>
#include <Fw/FPrimeBasicTypes.hpp>
#include <Fw/Types/Assert.hpp>
#include <Fw/Types/String.hpp>
#include <Os/Interfaces/DirectoryUserInterface.hpp>
#include <Os/Os.hpp>
#include "config/DirectorySelection.hpp"
#ifdef BUILD_UT
#include <Os/test/ut/implementations/UtDirectory.hpp>
#endif

namespace Os {

template <typename T>
class DirectoryCommon;

#ifdef BUILD_UT
//! \brief Os::Directory in unit test builds: wraps the implementation selected in config/DirectorySelection.hpp in the
//! test-only UtDirectory, so that tests may intercept its calls
using Directory = DirectoryCommon<Ut::Directory::UtDirectory>;
#else
//! \brief Os::Directory: the common class on the implementation selected in config/DirectorySelection.hpp
using Directory = DirectoryCommon<Selected::Directory>;
#endif

//! \brief Directory class
//!
//! This class provides a common interface for directory operations, such as reading files in a directory
//! and getting the number of files in a directory.
template <typename T>
class DirectoryCommon final : public DirectoryUserInterface {
  public:
    //! \brief Constructor
    DirectoryCommon();

    //! \brief Destructor
    //!
    //! Destructor will close the Directory if it is open
    ~DirectoryCommon();

    //! \brief return the underlying Directory handle (implementation specific)
    //! \return internal Directory handle representation
    DirectoryHandle* getHandle();

    // ------------------------------------------------------------
    // Implementation-specific Directory member functions
    // ------------------------------------------------------------
    // These functions are overridden in each OS implementation (e.g. in Os/Posix/Directory.hpp)

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
    Status open(const char* path, OpenMode mode);

    //! \brief Check if Directory is open or not
    //! \return true if Directory is open, false otherwise
    bool isOpen();

    //! \brief Rewind directory stream
    //!
    //! Each read operation moves the seek position forward. This function resets the seek position to the beginning.
    //!
    //! \return status of the operation
    Status rewind();

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
    Status read(char* fileNameBuffer, FwSizeType buffSize);

    //! \brief Close directory
    void close();

    // ------------------------------------------------------------
    // Common functions built on top of OS-specific functions
    // ------------------------------------------------------------

    //! \brief Get next filename from directory stream and write it to a Fw::StringBase object
    //!
    //! \param filename: Fw::StringBase (or derived) object to store filename in
    //! \return status of the operation
    Status read(Fw::StringBase& filename);

    //! \brief Read the contents of the directory and store filenames in the supplied array.
    //!
    //! Reads at most filenameArray.getSize() filenames.
    //! The function first rewinds the directory stream to ensure reading starts from the beginning.
    //! After reading, it rewinds the directory stream again, resetting seek position to beginning.
    //!
    //! \param filenameArray: array to store filenames
    //! \param filenameCount: number of filenames written to filenameArray (output)
    //! \return status of the operation
    Status readDirectory(Fw::ExternalArray<Fw::String>& filenameArray, FwSizeType& filenameCount);

    //! \brief Read the contents of the directory and store filenames in filenameArray of size arraySize.
    //!
    //! The function first rewinds the directory stream to ensure reading starts from the beginning.
    //! After reading, it rewinds the directory stream again, resetting seek position to beginning.
    //!
    //! \param filenameArray: array to store filenames
    //! \param arraySize: size of filenameArray
    //! \param filenameCount: number of filenames written to filenameArray (output)
    //! \return status of the operation
    DEPRECATED(Status readDirectory(Fw::String filenameArray[], const FwSizeType arraySize, FwSizeType& filenameCount),
               "Use readDirectory(Fw::ExternalArray<Fw::String>& filenameArray, FwSizeType& filenameCount) instead");

    //! \brief Get the number of files in the directory.
    //!
    //! Counts the number of files in the directory by reading each file entry and writing the count to fileCount.
    //!
    //! The function first rewinds the directory stream to ensure counting starts from the beginning.
    //! After counting, it rewinds the directory stream again, resetting seek position to beginning.
    //!
    //! \param fileCount Reference to a variable where the file count will be stored.
    //! \return Status indicating the result of the operation.
    Status getFileCount(FwSizeType& fileCount);

  private:
    bool m_is_open;  //!< Flag indicating if the directory has been open

  private:
    // This section is used to store the implementation-defined Directory handle. To Os::Directory and fprime, this type
    // is opaque and thus normal allocation cannot be done. Instead, we allow the implementor to store then handle in
    // the byte-array here and set `handle` to that address for storage.
    T m_delegate;
};

}  // namespace Os

namespace Os {

template <typename T>
DirectoryCommon<T>::DirectoryCommon() : m_is_open(false), m_delegate() {}

template <typename T>
DirectoryCommon<T>::~DirectoryCommon() {
    if (this->m_is_open) {
        this->close();
    }
}

// ------------------------------------------------------------
// Directory operations delegating to implementation
// ------------------------------------------------------------
template <typename T>
DirectoryHandle* DirectoryCommon<T>::getHandle() {
    return this->m_delegate.getHandle();
}

template <typename T>
typename DirectoryCommon<T>::Status DirectoryCommon<T>::open(const char* path, OpenMode mode) {
    FW_ASSERT(path != nullptr);
    FW_ASSERT(mode >= 0 and mode < OpenMode::MAX_OPEN_MODE);
    Status status = this->m_delegate.open(path, mode);
    if (status == Status::OP_OK) {
        this->m_is_open = true;
    }
    return status;
}

template <typename T>
bool DirectoryCommon<T>::isOpen() {
    return this->m_is_open;
}
template <typename T>
typename DirectoryCommon<T>::Status DirectoryCommon<T>::rewind() {
    if (not this->m_is_open) {
        return Status::NOT_OPENED;
    }
    return this->m_delegate.rewind();
}

template <typename T>
typename DirectoryCommon<T>::Status DirectoryCommon<T>::read(char* fileNameBuffer, FwSizeType bufSize) {
    if (not this->m_is_open) {
        return Status::NOT_OPENED;
    }
    FW_ASSERT(fileNameBuffer != nullptr);
    FW_ASSERT(bufSize > 0);
    Status status = this->m_delegate.read(fileNameBuffer, bufSize);
    fileNameBuffer[bufSize - 1] = '\0';  // Guarantee null-termination
    return status;
}

template <typename T>
typename DirectoryCommon<T>::Status DirectoryCommon<T>::read(Fw::StringBase& filename) {
    if (not this->m_is_open) {
        return Status::NOT_OPENED;
    }
    return this->m_delegate.read(const_cast<char*>(filename.toChar()), filename.getCapacity());
}

template <typename T>
void DirectoryCommon<T>::close() {
    this->m_is_open = false;
    return this->m_delegate.close();
}

// ------------------------------------------------------------
// Common functions built on top of OS-specific functions
// ------------------------------------------------------------

template <typename T>
typename DirectoryCommon<T>::Status DirectoryCommon<T>::getFileCount(FwSizeType& fileCount) {
    if (not this->m_is_open) {
        return Status::NOT_OPENED;
    }
    // Rewind to ensure we start from the beginning of the stream
    if (this->rewind() != Status::OP_OK) {
        return Status::OTHER_ERROR;
    }
    const FwSizeType loopLimit = std::numeric_limits<FwSizeType>::max();
    FwSizeType count = 0;
    char unusedBuffer[1];  // buffer must have size but is unused
    Status readStatus = Status::OP_OK;
    fileCount = 0;
    // Count files by reading each file entry until there is NO_MORE_FILES
    for (FwSizeType iter = 0; iter < loopLimit; ++iter) {
        readStatus = this->read(unusedBuffer, sizeof(unusedBuffer));
        if (readStatus == Status::NO_MORE_FILES) {
            break;
        } else if (readStatus != Status::OP_OK) {
            return Status::OTHER_ERROR;
        }
        count++;
    }
    fileCount = count;
    if (this->rewind() != Status::OP_OK) {
        return Status::OTHER_ERROR;
    }
    return Status::OP_OK;
}

template <typename T>
typename DirectoryCommon<T>::Status DirectoryCommon<T>::readDirectory(Fw::ExternalArray<Fw::String>& filenameArray,
                                                                      FwSizeType& filenameCount) {
    FW_ASSERT(filenameArray.getElements() != nullptr);
    FW_ASSERT(filenameArray.getSize() > 0);
    if (not this->m_is_open) {
        return Status::NOT_OPENED;
    }
    // Rewind to ensure we start reading from the beginning of the stream
    if (this->rewind() != Status::OP_OK) {
        return Status::OTHER_ERROR;
    }

    Status readStatus = Status::OP_OK;
    Status returnStatus = Status::OP_OK;
    FwSizeType index;
    filenameCount = 0;
    // Iterate through the directory and read the filenames into the array
    for (index = 0; index < filenameArray.getSize(); index++) {
        readStatus = this->read(filenameArray[index]);
        if (readStatus == Status::NO_MORE_FILES) {
            break;
        } else if (readStatus != Status::OP_OK) {
            return Status::OTHER_ERROR;
        }
    }
    filenameCount = index;

    if (this->rewind() != Status::OP_OK) {
        return Status::OTHER_ERROR;
    }

    return returnStatus;
}

template <typename T>
typename DirectoryCommon<T>::Status DirectoryCommon<T>::readDirectory(Fw::String filenameArray[],
                                                                      const FwSizeType filenameArraySize,
                                                                      FwSizeType& filenameCount) {
    Fw::ExternalArray<Fw::String> array(filenameArray, filenameArraySize);
    return this->readDirectory(array, filenameCount);
}

}  // namespace Os

#endif
