// ======================================================================
// \title Os/Console.cpp
// \brief common function implementation for Os::Console
// ======================================================================
#include <Fw/Types/Assert.hpp>
#include <Os/Console.hpp>
#include <Os/Delegate.hpp>

namespace Os {
template <typename T>
ConsoleCommon<T>::ConsoleCommon() : ConsoleUserInterface(), Fw::Logger(), m_delegate() {}

template <typename T>
ConsoleCommon<T>::~ConsoleCommon() {}

template <typename T>
ConsoleCommon<T>::ConsoleCommon(const ConsoleCommon& other) : m_delegate(other.m_delegate) {}

template <typename T>
ConsoleCommon<T>& ConsoleCommon<T>::operator=(const ConsoleCommon& other) {
    if (this != &other) {
        this->m_delegate = other.m_delegate;
    }
    return *this;
}

template <typename T>
void ConsoleCommon<T>::writeMessage(const CHAR* message, const FwSizeType size) {
    FW_ASSERT(message != nullptr || size == 0);
    this->m_delegate.writeMessage(message, size);
}

template <typename T>
void ConsoleCommon<T>::writeMessage(const Fw::ConstStringBase& message) {
    this->writeMessage(message.toChar(), message.length());
}

template <typename T>
ConsoleHandle* ConsoleCommon<T>::getHandle() {
    return this->m_delegate.getHandle();
}

template <typename T>
void ConsoleCommon<T>::write(const CHAR* message, const FwSizeType size) {
    ConsoleCommon::getSingleton().writeMessage(message, size);
}

template <typename T>
void ConsoleCommon<T>::write(const Fw::ConstStringBase& message) {
    ConsoleCommon::getSingleton().writeMessage(message.toChar(), message.length());
}

template <typename T>
void ConsoleCommon<T>::init() {
    // Force trigger on the fly singleton setup
    (void)ConsoleCommon::getSingleton();
}

template <typename T>
ConsoleCommon<T>& ConsoleCommon<T>::getSingleton() {
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
// Instantiate the common class for the implementation selected in config/OsSelection.hpp
template class ConsoleCommon<Delegate::Selected<Console>::type>;

}  // namespace Os
