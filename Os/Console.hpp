// ======================================================================
// \title Os/Console.hpp
// \brief common function definitions for Os::Console
// ======================================================================
#ifndef Os_Console_hpp_
#define Os_Console_hpp_

#include <Fw/FPrimeBasicTypes.hpp>
#include <Fw/Logger/Logger.hpp>
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

#endif
