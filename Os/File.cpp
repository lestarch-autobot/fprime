// ======================================================================
// \title Os/File.cpp
// \brief common function implementation for Os::File
// ======================================================================
#include <Fw/Types/Assert.hpp>
#include <Fw/Types/StringUtils.hpp>
#include <Os/Delegate.hpp>
#include <Os/File.hpp>
#include <algorithm>
#include <config/FppConstantsAc.hpp>

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
// Instantiate the common class for the implementation selected in config/OsSelection.hpp
template class FileCommon<Delegate::Selected<File>::type>;

}  // namespace Os
