// ======================================================================
// \title Os/Console.cpp
// \brief common function implementation for Os::Console
// ======================================================================
#include <Fw/Types/Assert.hpp>
#include <Os/Console.hpp>

namespace Os {
ConsoleCommon::ConsoleCommon()
    : ConsoleInterface(),
      Fw::Logger(),
      m_handle_storage(),
      m_delegate(*ConsoleInterface::getDelegate(m_handle_storage)) {}

ConsoleCommon::~ConsoleCommon() {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<ConsoleInterface*>(&this->m_handle_storage[0]));
    m_delegate.~ConsoleInterface();
}

ConsoleCommon::ConsoleCommon(const ConsoleCommon& other)
    : m_handle_storage(), m_delegate(*ConsoleCommon::getDelegate(m_handle_storage, &other.m_delegate)) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<ConsoleCommon*>(&this->m_handle_storage[0]));
}

ConsoleCommon& ConsoleCommon::operator=(const ConsoleCommon& other) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<ConsoleCommon*>(&this->m_handle_storage[0]));
    if (this != &other) {
        this->m_delegate = *ConsoleInterface::getDelegate(m_handle_storage, &other.m_delegate);
    }
    return *this;
}

void ConsoleCommon::writeMessage(const CHAR* message, const FwSizeType size) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<ConsoleInterface*>(&this->m_handle_storage));
    FW_ASSERT(message != nullptr || size == 0);
    this->m_delegate.writeMessage(message, size);
}

void ConsoleCommon::writeMessage(const Fw::ConstStringBase& message) {
    this->writeMessage(message.toChar(), message.length());
}

ConsoleHandle* ConsoleCommon::getHandle() {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<ConsoleInterface*>(&this->m_handle_storage));
    return this->m_delegate.getHandle();
}

void ConsoleCommon::write(const CHAR* message, const FwSizeType size) {
    ConsoleCommon::getSingleton().writeMessage(message, size);
}

void ConsoleCommon::write(const Fw::ConstStringBase& message) {
    ConsoleCommon::getSingleton().writeMessage(message.toChar(), message.length());
}

void ConsoleCommon::init() {
    // Force trigger on the fly singleton setup
    (void)ConsoleCommon::getSingleton();
}

ConsoleCommon& ConsoleCommon::getSingleton() {
    static ConsoleCommon s_singleton;
    // Registration happens once: re-registering on every access would silently replace a
    // logger the project registered after the console singleton was first used
    static bool s_registered = false;
    if (not s_registered) {
        s_registered = true;
        Fw::Logger::registerLogger(&s_singleton);
    }
    return s_singleton;
}
}  // namespace Os
