// ======================================================================
// \title Os/Directory.cpp
// \brief common function implementation for Os::Directory
// ======================================================================
#include <Fw/Types/Assert.hpp>
#include <Os/Delegate.hpp>
#include <Os/Directory.hpp>

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

// Instantiate the common class for the implementation selected in config/OsSelection.hpp
template class DirectoryCommon<Delegate::Selected<Directory>::type>;

}  // namespace Os
