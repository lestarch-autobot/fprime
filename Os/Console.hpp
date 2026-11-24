// ======================================================================
// \title Os/Console.hpp
// \brief common function definitions for Os::Console
// ======================================================================
#ifndef Os_Console_hpp_
#define Os_Console_hpp_

#include <Fw/FPrimeBasicTypes.hpp>
#include <Fw/Logger/Logger.hpp>
#include <Fw/Types/Assert.hpp>
#include <Os/Console.hpp>
#include <Os/Os.hpp>
#include "config/OsSelection.hpp"  // defines the Os::Console alias and OS_CONSOLE_HEADER; do not remove

namespace Os {
//! \brief Base class for storing implementation specific handle information
struct ConsoleHandle {};

//! \brief types and constants shared by users of Os::Console and by its implementations
struct ConsoleUserInterface {};

template <typename T>
class ConsoleCommon : public ConsoleUserInterface, public Fw::Logger {
  public:
    //! \brief Default constructor
    ConsoleCommon();

    //! \brief Default destructor
    ~ConsoleCommon();

    //! \brief copy constructor that copies the internal representation
    ConsoleCommon(const ConsoleCommon& other);

    //! \brief assignment operator that copies the internal representation
    ConsoleCommon& operator=(const ConsoleCommon& other);

    //! \brief write message to console
    //!
    //! Write a message to the console with a bounded size. This will delegate to the implementation defined write
    //! method.
    //!
    //! \param message: raw message to write
    //! \param size: size of the message to write to the console
    void writeMessage(const CHAR* message, const FwSizeType size);

    //! \brief write message to console
    //!
    //! Write a message to the console as stored as a ConstStringBase type
    //!
    //! \param message: raw message to write (ConstStringBase)
    void writeMessage(const Fw::ConstStringBase& message);

    //! \brief returns the raw console handle
    //!
    //! Gets the raw console handle from the implementation. Note: users must include the implementation specific
    //! header to make any real use of this handle. Otherwise it will be as an opaque type.
    //!
    //! \return raw console handle
    //!
    ConsoleHandle* getHandle();

    //! \brief write message to console
    //!
    //! Write a message to the console as stored as a ConstStringBase type
    //!
    //! \param message: raw message to write (ConstStringBase)
    static void write(const Fw::ConstStringBase& message);

    //! \brief write message to the global console
    //!
    //! Write a message to the console with a bounded size. This will delegate to the global singleton
    //! implementation.
    //!
    //! \param message: raw message to write
    //! \param size: size of the message to write to the console
    static void write(const CHAR* message, const FwSizeType size);

    //! \brief initialize singleton
    static void init();

    //! \brief get a reference to singleton
    //! \return reference to singleton
    static ConsoleCommon& getSingleton();

  private:
    // This section is used to store the implementation-defined console handle. To Os::Console and fprime, this type
    // is opaque and thus normal allocation cannot be done. Instead, we allow the implementor to store then handle
    // in the byte-array here and set `handle` to that address for storage.
    T m_delegate;  //!< Delegate for the real implementation
};

}  // namespace Os

// Validate that OS_CONSOLE_HEADER was defined by config/OsSelection.hpp
#ifndef OS_CONSOLE_HEADER
#error "OS_CONSOLE_HEADER must be defined in config/OsSelection.hpp"
#endif

#include OS_CONSOLE_HEADER

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
}  // namespace Os

#endif
