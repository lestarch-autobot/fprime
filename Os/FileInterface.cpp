// ======================================================================
// \title Os/FileInterface.cpp
// \brief common function implementations for Os::FileInterface
// ======================================================================
#include <Fw/Types/Assert.hpp>
#include <Fw/Types/StringUtils.hpp>
#include <Os/FileInterface.hpp>
#include <algorithm>
#include <config/FppConstantsAc.hpp>
#include <limits>

namespace Os {

void FileInterface::markOpen(FileInterface::Mode mode) {
    FW_ASSERT((Mode::OPEN_NO_MODE < mode) && (mode < Mode::MAX_OPEN_MODE));
    this->m_mode = mode;
    // Reset any open CRC calculations
    this->m_hash.init();
}

void FileInterface::markClosed() {
    this->m_mode = Mode::OPEN_NO_MODE;
}

bool FileInterface::isOpen() const {
    FW_ASSERT((0 <= this->m_mode) && (this->m_mode < Mode::MAX_OPEN_MODE));
    return this->m_mode != Mode::OPEN_NO_MODE;
}

void FileInterface::assertValidPath(const CHAR* path) {
    FW_ASSERT(nullptr != path);
    const FwSizeType bound = static_cast<FwSizeType>(FileNameStringSize + 1);
    const FwSizeType string_len = static_cast<FwSizeType>(Fw::StringUtils::string_length(path, bound));
    FW_ASSERT(string_len < bound, static_cast<FwAssertArgType>(string_len), static_cast<FwAssertArgType>(bound));
}

FileInterface::Status FileInterface::open(const CHAR* filepath, FileInterface::Mode requested_mode) {
    return this->open(filepath, requested_mode, OverwriteType::NO_OVERWRITE);
}

FileInterface::Status FileInterface::open(const CHAR* filepath, FwSizeType length, FileInterface::Mode requested_mode) {
    return this->open(filepath, length, requested_mode, OverwriteType::NO_OVERWRITE);
}

FileInterface::Status FileInterface::open(const CHAR* filepath,
                                          FwSizeType length,
                                          FileInterface::Mode requested_mode,
                                          FileInterface::OverwriteType overwrite) {
    FW_ASSERT(nullptr != filepath);
    const FwSizeType string_len = static_cast<FwSizeType>(Fw::StringUtils::string_length(filepath, length));
    FW_ASSERT(string_len < length, static_cast<FwAssertArgType>(string_len), static_cast<FwAssertArgType>(length));
    return this->open(filepath, requested_mode, overwrite);
}

FileInterface::Status FileInterface::open(const Fw::ConstStringBase& path, FileInterface::Mode requested_mode) {
    return this->open(path.toChar(), static_cast<FwSizeType>(path.getCapacity()), requested_mode,
                      OverwriteType::NO_OVERWRITE);
}

FileInterface::Status FileInterface::open(const Fw::ConstStringBase& path,
                                          FileInterface::Mode requested_mode,
                                          FileInterface::OverwriteType overwrite) {
    return this->open(path.toChar(), static_cast<FwSizeType>(path.getCapacity()), requested_mode, overwrite);
}

FileInterface::Status FileInterface::seek_absolute(FwSizeType offset) {
    FileInterface::Status status = FileInterface::Status::OTHER_ERROR;
    // If the offset can be represented by a signed value, then we can perform a single seek
    if (static_cast<FwSizeType>(std::numeric_limits<FwSignedSizeType>::max()) >= offset) {
        // Check that the bounding above is correct
        FW_ASSERT(static_cast<FwSignedSizeType>(offset) >= 0);
        status = this->seek(static_cast<FwSignedSizeType>(offset), FileInterface::SeekType::ABSOLUTE);
    }
    // Otherwise, a full seek to any value represented by FwSizeType can be performed
    // by at most 3 seeks of a FwSignedSizeType. Two half seeks (rounded down) that are
    // strictly bounded by std::numeric_limits<FwSignedSizeType>::max() and one seek of
    // a possibile "odd" byte to ensure odds offsets do not introduce an off-by-one-error.
    // Thus we perform 3 seeks to guarantee that we can reach any position.
    else {
        FwSignedSizeType half_offset = static_cast<FwSignedSizeType>(offset >> 1);
        bool is_odd = (offset % 2) == 1;
        status = this->seek(half_offset, FileInterface::SeekType::ABSOLUTE);
        if (status == FileInterface::Status::OP_OK) {
            status = this->seek(half_offset, FileInterface::SeekType::RELATIVE);
        }
        if (status == FileInterface::Status::OP_OK) {
            status = this->seek((is_odd) ? 1 : 0, FileInterface::SeekType::RELATIVE);
        }
    }
    return status;
}

FileInterface::Status FileInterface::read(U8* buffer, FwSizeType& size) {
    return this->read(buffer, size, WaitType::WAIT);
}

FileInterface::Status FileInterface::write(const U8* buffer, FwSizeType& size) {
    return this->write(buffer, size, WaitType::WAIT);
}

FileInterface::Status FileInterface::calculateCrc(U32& crc) {
    FileInterface::Status status = FileInterface::Status::OP_OK;
    FwSizeType size = FW_FILE_CHUNK_SIZE;
    crc = 0;
    for (FwSizeType i = 0; i < std::numeric_limits<FwSizeType>::max(); i++) {
        status = this->incrementalCrc(size);
        // Break on eof or error
        if ((size != FW_FILE_CHUNK_SIZE) || (status != FileInterface::OP_OK)) {
            break;
        }
    }
    // When successful, finalize the CRC
    if (status == FileInterface::OP_OK) {
        status = this->finalizeCrc(crc);
    }
    return status;
}

FileInterface::Status FileInterface::incrementalCrc(FwSizeType& size) {
    FileInterface::Status status = FileInterface::Status::OP_OK;
    // Scratch buffer is local so that implementations selected by direct alias do not carry it per-object
    U8 crc_buffer[FW_FILE_CHUNK_SIZE] = {};
    FW_ASSERT(size <= FW_FILE_CHUNK_SIZE, FwAssertArgType(size));
    if (OPEN_NO_MODE == this->m_mode) {
        status = FileInterface::Status::NOT_OPENED;
    } else if (OPEN_READ != this->m_mode) {
        status = FileInterface::Status::INVALID_MODE;
    } else {
        // Read data without waiting for additional data to be available
        status = this->read(crc_buffer, size, FileInterface::WaitType::NO_WAIT);
        if (OP_OK == status) {
            FW_ASSERT(size <= FW_FILE_CHUNK_SIZE, FwAssertArgType(size));
            this->m_hash.update(crc_buffer, size);
        }
    }
    return status;
}

FileInterface::Status FileInterface::finalizeCrc(U32& crc) {
    FileInterface::Status status = FileInterface::Status::OP_OK;
    this->m_hash.finalize(crc);
    // Historically, the CRC calculation in File omitted the final 1's complement step. Utils::Hash performs that step
    // and as such, we must undo it before returning the value to ensure backwards compatibility.
    crc = ~crc;
    this->m_hash.init();
    return status;
}

FileInterface::Status FileInterface::readline(U8* buffer, FwSizeType& size, FileInterface::WaitType wait) {
    const FwSizeType requested_size = size;
    FW_ASSERT(buffer != nullptr);
    FW_ASSERT((0 <= this->m_mode) && (this->m_mode < Mode::MAX_OPEN_MODE));
    // Check that the file is open before attempting operation
    if (OPEN_NO_MODE == this->m_mode) {
        size = 0;
        return FileInterface::Status::NOT_OPENED;
    } else if (OPEN_READ != this->m_mode) {
        size = 0;
        return FileInterface::Status::INVALID_MODE;
    }
    FwSizeType original_location = 0;
    FileInterface::Status status = this->position(original_location);
    if (status != FileInterface::Status::OP_OK) {
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
        if (status != FileInterface::Status::OP_OK) {
            // Contract: on error, seek back to the original location
            size = 0;
            (void)this->seek_absolute(original_location);
            return status;
        }
        // EOF break out now
        if (read == 0) {
            size = i;
            return FileInterface::Status::OP_OK;
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
                return FileInterface::Status::OP_OK;
            }
        }
    }
    // Failed to find newline within data available
    // Contract: on error, seek back to the original location
    size = 0;
    (void)this->seek_absolute(original_location);
    return FileInterface::Status::OTHER_ERROR;
}
}  // namespace Os
