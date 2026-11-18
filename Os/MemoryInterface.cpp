// ======================================================================
// \title Os/MemoryInterface.cpp
// \brief common convenience-wrapper implementation for Os::Memory
// ======================================================================
#include <Os/Memory.hpp>

namespace Os {

void MemoryInterface::init() {
    (void)Os::Memory::getSingleton();
}

Memory& MemoryInterface::getSingleton() {
    static Memory _singleton;
    return _singleton;
}

MemoryInterface::Status MemoryInterface::getUsage(Usage& memory) {
    return Os::Memory::getSingleton()._getUsage(memory);
}

}  // namespace Os
