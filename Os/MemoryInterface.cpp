// ======================================================================
// \title Os/MemoryInterface.cpp
// \brief common function implementations for Os::Memory
// ======================================================================
#include "Os/Memory.hpp"

namespace Os {

void MemoryInterface::init() {
    (void)MemoryInterface::getSingleton();
}

Memory& MemoryInterface::getSingleton() {
    static Memory _singleton;
    return _singleton;
}

MemoryInterface::Status MemoryInterface::getUsage(Usage& memory_usage) {
    return MemoryInterface::getSingleton()._getUsage(memory_usage);
}

}  // namespace Os
