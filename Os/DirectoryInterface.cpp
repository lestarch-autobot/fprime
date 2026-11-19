// ======================================================================
// \title Os/DirectoryInterface.cpp
// \brief common function implementation for Os::DirectoryInterface
// ======================================================================
#include <Fw/Types/Assert.hpp>
#include <Os/Directory.hpp>
#include <limits>
#include <type_traits>

// The implementation selected as Os::Directory must derive from DirectoryInterface and owns the open state: the
// DirectoryInterface::isOpenImpl() default is only for implementations used through the link-time delegate. Checked
// here, where Os::Directory (pulled in through Os/Directory.hpp) is complete. The aliased class must declare the
// override itself: one inherited from an intermediate base is rejected too.
static_assert(std::is_base_of<Os::DirectoryInterface, Os::Directory>::value,
              "The implementation selected as Os::Directory must derive from Os::DirectoryInterface");
static_assert(std::is_same<decltype(&Os::Directory::isOpenImpl), bool (Os::Directory::*)() const>::value,
              "The class selected as Os::Directory must itself override DirectoryInterface::isOpenImpl()");

namespace Os {

bool DirectoryInterface::isOpenImpl() const {
    // Only reachable on an implementation used directly without overriding isOpenImpl(): the link-time delegate
    // tracks the open state itself and never asks the implementation
    FW_ASSERT(0);
    return false;
}

DirectoryInterface::Status DirectoryInterface::read(Fw::StringBase& filename) {
    if (not this->isOpen()) {
        return Status::NOT_OPENED;
    }
    return this->read(const_cast<char*>(filename.toChar()), filename.getCapacity());
}

DirectoryInterface::Status DirectoryInterface::readDirectory(Fw::ExternalArray<Fw::String>& filenameArray,
                                                             FwSizeType& filenameCount) {
    FW_ASSERT(filenameArray.getElements() != nullptr);
    FW_ASSERT(filenameArray.getSize() > 0);
    if (not this->isOpen()) {
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

DirectoryInterface::Status DirectoryInterface::readDirectory(Fw::String filenameArray[],
                                                             const FwSizeType filenameArraySize,
                                                             FwSizeType& filenameCount) {
    Fw::ExternalArray<Fw::String> array(filenameArray, filenameArraySize);
    return this->readDirectory(array, filenameCount);
}

DirectoryInterface::Status DirectoryInterface::getFileCount(FwSizeType& fileCount) {
    if (not this->isOpen()) {
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

}  // namespace Os
