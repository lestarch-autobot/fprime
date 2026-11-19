// ======================================================================
// \title Os/DelegateFile.cpp
// \brief implementation of Os::DelegateFile (link-time delegating Os::File)
// ======================================================================
#include <Fw/Types/Assert.hpp>
#include <Os/DelegateFile.hpp>

namespace Os {

DelegateFile::DelegateFile() : m_handle_storage(), m_delegate(*FileInterface::getDelegate(m_handle_storage)) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<FileInterface*>(&this->m_handle_storage[0]));
}

DelegateFile::~DelegateFile() {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<FileInterface*>(&this->m_handle_storage[0]));
    if (this->m_mode != OPEN_NO_MODE) {
        this->close();
    }
    m_delegate.~FileInterface();
}

DelegateFile::DelegateFile(const DelegateFile& other)
    : FileInterface(other),
      m_handle_storage(),
      m_delegate(*FileInterface::getDelegate(m_handle_storage, &other.m_delegate)) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<FileInterface*>(&this->m_handle_storage[0]));
}

DelegateFile& DelegateFile::operator=(const DelegateFile& other) {
    if (this != &other) {
        // The delegate below is constructed over the storage of the existing one. Any file this
        // object currently holds must be closed first or its handle is orphaned permanently.
        if (this->m_mode != OPEN_NO_MODE) {
            this->close();
        }
        this->m_delegate.~FileInterface();
        FileInterface::operator=(other);
        (void)FileInterface::getDelegate(m_handle_storage, &other.m_delegate);
        FW_ASSERT(&this->m_delegate == reinterpret_cast<FileInterface*>(&this->m_handle_storage[0]));
    }
    return *this;
}

DelegateFile::Status DelegateFile::open(const CHAR* filepath,
                                        DelegateFile::Mode requested_mode,
                                        DelegateFile::OverwriteType overwrite) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<FileInterface*>(&this->m_handle_storage[0]));
    FileInterface::assertValidPath(filepath);
    FW_ASSERT(DelegateFile::Mode::OPEN_NO_MODE < requested_mode && DelegateFile::Mode::MAX_OPEN_MODE > requested_mode);
    FW_ASSERT((0 <= this->m_mode) && (this->m_mode < Mode::MAX_OPEN_MODE));
    FW_ASSERT((0 <= overwrite) && (overwrite < OverwriteType::MAX_OVERWRITE_TYPE));
    // Check for already opened file
    if (this->isOpen()) {
        return DelegateFile::Status::INVALID_MODE;
    }
    DelegateFile::Status status = this->m_delegate.open(filepath, requested_mode, overwrite);
    if (status == DelegateFile::Status::OP_OK) {
        // Tracks the mode and resets any open CRC calculations
        this->markOpen(requested_mode);
    }
    return status;
}

void DelegateFile::close() {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<FileInterface*>(&this->m_handle_storage[0]));
    FW_ASSERT((0 <= this->m_mode) && (this->m_mode < Mode::MAX_OPEN_MODE));
    this->m_delegate.close();
    this->markClosed();
}

DelegateFile::Status DelegateFile::size(FwSizeType& size_result) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<FileInterface*>(&this->m_handle_storage[0]));
    FW_ASSERT((0 <= this->m_mode) && (this->m_mode < Mode::MAX_OPEN_MODE));
    if (OPEN_NO_MODE == this->m_mode) {
        return DelegateFile::Status::NOT_OPENED;
    }
    return this->m_delegate.size(size_result);
}

DelegateFile::Status DelegateFile::position(FwSizeType& position_result) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<FileInterface*>(&this->m_handle_storage[0]));
    FW_ASSERT((0 <= this->m_mode) && (this->m_mode < Mode::MAX_OPEN_MODE));
    // Check that the file is open before attempting operation
    if (OPEN_NO_MODE == this->m_mode) {
        return DelegateFile::Status::NOT_OPENED;
    }
    return this->m_delegate.position(position_result);
}

DelegateFile::Status DelegateFile::preallocate(FwSizeType offset, FwSizeType length) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<FileInterface*>(&this->m_handle_storage[0]));
    FW_ASSERT((0 <= this->m_mode) && (this->m_mode < Mode::MAX_OPEN_MODE));
    // Check that the file is open before attempting operation
    if (OPEN_NO_MODE == this->m_mode) {
        return DelegateFile::Status::NOT_OPENED;
    } else if (OPEN_READ == this->m_mode) {
        return DelegateFile::Status::INVALID_MODE;
    }
    return this->m_delegate.preallocate(offset, length);
}

DelegateFile::Status DelegateFile::seek(FwSignedSizeType offset, DelegateFile::SeekType seekType) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<FileInterface*>(&this->m_handle_storage[0]));
    FW_ASSERT((0 <= seekType) && (seekType < SeekType::MAX_SEEK_TYPE));
    // Cannot do a seek with a negative offset in absolute mode
    FW_ASSERT((seekType == DelegateFile::SeekType::RELATIVE) || (offset >= 0));
    FW_ASSERT((0 <= this->m_mode) && (this->m_mode < Mode::MAX_OPEN_MODE));
    // Check that the file is open before attempting operation
    if (OPEN_NO_MODE == this->m_mode) {
        return DelegateFile::Status::NOT_OPENED;
    }
    return this->m_delegate.seek(offset, seekType);
}

DelegateFile::Status DelegateFile::flush() {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<FileInterface*>(&this->m_handle_storage[0]));
    FW_ASSERT((0 <= this->m_mode) && (this->m_mode < Mode::MAX_OPEN_MODE));
    // Check that the file is open before attempting operation
    if (OPEN_NO_MODE == this->m_mode) {
        return DelegateFile::Status::NOT_OPENED;
    } else if (OPEN_READ == this->m_mode) {
        return DelegateFile::Status::INVALID_MODE;
    }
    return this->m_delegate.flush();
}

DelegateFile::Status DelegateFile::read(U8* buffer, FwSizeType& size, DelegateFile::WaitType wait) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<FileInterface*>(&this->m_handle_storage[0]));
    FW_ASSERT(buffer != nullptr);
    FW_ASSERT((0 <= this->m_mode) && (this->m_mode < Mode::MAX_OPEN_MODE));
    // Check that the file is open before attempting operation
    if (OPEN_NO_MODE == this->m_mode) {
        size = 0;
        return DelegateFile::Status::NOT_OPENED;
    } else if (OPEN_READ != this->m_mode) {
        size = 0;
        return DelegateFile::Status::INVALID_MODE;
    }
    return this->m_delegate.read(buffer, size, wait);
}

DelegateFile::Status DelegateFile::write(const U8* buffer, FwSizeType& size, DelegateFile::WaitType wait) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<FileInterface*>(&this->m_handle_storage[0]));
    FW_ASSERT(buffer != nullptr);
    FW_ASSERT((0 <= this->m_mode) && (this->m_mode < Mode::MAX_OPEN_MODE));
    // Check that the file is open before attempting operation
    if (OPEN_NO_MODE == this->m_mode) {
        size = 0;
        return DelegateFile::Status::NOT_OPENED;
    } else if (OPEN_READ == this->m_mode) {
        size = 0;
        return DelegateFile::Status::INVALID_MODE;
    }
    return this->m_delegate.write(buffer, size, wait);
}

FileHandle* DelegateFile::getHandle() {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<FileInterface*>(&this->m_handle_storage[0]));
    return this->m_delegate.getHandle();
}

}  // namespace Os
