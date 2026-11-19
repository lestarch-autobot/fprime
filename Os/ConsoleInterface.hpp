// ======================================================================
// \title Os/ConsoleInterface.hpp
// \brief Os::ConsoleHandle and Os::ConsoleInterface definitions
//
// Users include Os/Console.hpp, which completes the Os::Console type. Implementation headers include this file.
// ======================================================================
#ifndef OS_CONSOLEINTERFACE_HPP_
#define OS_CONSOLEINTERFACE_HPP_

#include <Fw/FPrimeBasicTypes.hpp>
#include <Fw/Logger/Logger.hpp>
#include <Os/Os.hpp>
#include "config/OsSelection.hpp"  // defines Os::Console alias and OS_CONSOLE_HEADER consumed by Os/Console.hpp; do not remove

// A project's config/OsSelection.hpp must choose the Console: the Os::Console alias and OS_CONSOLE_HEADER together.
// The preprocessor cannot see the alias: a "'Console' does not name a type" error below means the alias is missing.
#ifndef OS_CONSOLE_HEADER
#error "config/OsSelection.hpp must select the Console: define the Os::Console alias and OS_CONSOLE_HEADER"
#endif

namespace Os {
//! \brief Base class for storing implementation specific handle information
struct ConsoleHandle {};

// \brief Interface defining the properties of the console
class ConsoleInterface : public Fw::Logger {
  public:
    //! \brief Default constructor
    ConsoleInterface() = default;

    //! \brief Default destructor
    virtual ~ConsoleInterface() = default;

    //! \brief write message to console
    //!
    //! Write a message to the console with a bounded size.
    //!
    //! Implementations must assert that `message` is not a nullptr unless `size` is 0. Implementations must also
    //! bring the `Fw::ConstStringBase` overload back into scope with `using ConsoleInterface::writeMessage;`
    //!
    //! \param message: raw message to write
    //! \param size: size of the message to write to the console
    virtual void writeMessage(const CHAR* message, const FwSizeType size) = 0;

    //! \brief returns the raw console handle
    //!
    //! Gets the raw console handle from the implementation. Note: users must include the implementation specific
    //! header to make any real use of this handle. Otherwise it will be as an opaque type.
    //!
    //! \return raw console handle
    //!
    virtual ConsoleHandle* getHandle() = 0;

    //! \brief provide a pointer to a console delegate object
    //!
    //! This function must return a pointer to a `ConsoleInterface` object that contains the real implementation of
    //! the console functions as defined by the implementor.  This function must do several things to be considered
    //! correctly implemented:
    //!
    //! 1. Assert that their implementation fits within FW_HANDLE_MAX_SIZE.
    //!    e.g. `static_assert(sizeof(PosixConsole) <= sizeof(Os::ConsoleHandleStorage),
    //!        "FW_HANDLE_MAX_SIZE too small");`
    //! 2. Assert that their implementation aligns within FW_HANDLE_ALIGNMENT.
    //!    e.g. `static_assert((FW_HANDLE_ALIGNMENT % alignof(PosixConsole)) == 0, "Bad handle alignment");`
    //! 3. If to_copy is null, placement new their implementation into `aligned_placement_new_memory`
    //!    e.g. `ConsoleInterface* interface = new (aligned_placement_new_memory) PosixConsole;`
    //! 4. If to_copy is non-null, placement new using copy constructor their implementation into
    //!    `aligned_placement_new_memory`
    //!    e.g. `ConsoleInterface* interface = new (aligned_placement_new_memory) PosixConsole(*to_copy);`
    //! 5. Return the result of the placement new
    //!    e.g. `return interface;`
    //!
    //! \return result of placement new, must be equivalent to `aligned_placement_new_memory`
    //!
    static ConsoleInterface* getDelegate(ConsoleHandleStorage& aligned_placement_new_memory,
                                         const ConsoleInterface* to_copy = nullptr);

    // ------------------------------------------------------------
    // Common functions built on top of the implementation-specific functions
    // ------------------------------------------------------------

    //! \brief write message to console
    //!
    //! Write a message to the console as stored as a ConstStringBase type. This fulfills the `Fw::Logger` interface
    //! by forwarding to the implementation-specific `writeMessage(const CHAR*, FwSizeType)`.
    //!
    //! \param message: raw message to write (ConstStringBase)
    void writeMessage(const Fw::ConstStringBase& message) override;

    //! \brief write message to the global console
    //!
    //! Write a message stored as a ConstStringBase type to the global singleton console.
    //!
    //! \param message: raw message to write (ConstStringBase)
    static void write(const Fw::ConstStringBase& message);

    //! \brief write message to the global console
    //!
    //! Write a message to the console with a bounded size. This will write to the global singleton console.
    //!
    //! \param message: raw message to write
    //! \param size: size of the message to write to the console
    static void write(const CHAR* message, const FwSizeType size);

    //! \brief initialize singleton
    static void init();

    //! \brief get a reference to singleton
    //!
    //! The singleton is an instance of the configured `Os::Console` and is registered as the `Fw::Logger` on first
    //! access.
    //!
    //! \return reference to singleton
    static Console& getSingleton();
};
}  // namespace Os

#endif  // OS_CONSOLEINTERFACE_HPP_
