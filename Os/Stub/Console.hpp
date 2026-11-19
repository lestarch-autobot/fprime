// ======================================================================
// \title Os/Stub/Console.hpp
// \brief stub implementation for Os::Console, header and test definitions
// ======================================================================
#ifndef OS_Stub_Console_HPP
#define OS_Stub_Console_HPP

#include <Os/Console.hpp>
#include <cstdio>

namespace Os {
namespace Stub {
namespace Console {

//! ConsoleHandle class definition for stub implementations.
//!
struct StubConsoleHandle : public ConsoleHandle {};

//! \brief stub implementation of Os::ConsoleInterface
//!
//! Stub implementation of `ConsoleInterface` handling stub console operations, usable as the link-time delegate
//! or directly as `Os::Console`.
//!
class StubConsole : public ConsoleInterface {
  public:
    //! \brief constructor
    //!
    StubConsole() = default;

    //! \brief copy constructor
    StubConsole(const StubConsole& other) = default;

    //! \brief default copy assignment
    StubConsole& operator=(const StubConsole& other) = default;

    //! \brief destructor
    //!
    ~StubConsole() override = default;

    // ------------------------------------
    // Functions overrides
    // ------------------------------------

    // Keep the common ConstStringBase overload visible alongside the override below
    using ConsoleInterface::writeMessage;

    //! \brief write message to console
    //!
    //! Discards the message: the stub console has no output destination.
    //!
    //! \param message: raw message to write
    //! \param size: size of the message to write to the console
    void writeMessage(const CHAR* message, const FwSizeType size) override;

    //! \brief returns the raw console handle
    //!
    //! Gets the raw console handle from the implementation. Note: users must include the implementation specific
    //! header to make any real use of this handle. Otherwise it will be as an opaque type.
    //!
    //! \return raw console handle
    //!
    ConsoleHandle* getHandle() override;

  private:
    //! Handle for StubConsole
    StubConsoleHandle m_handle;
};
}  // namespace Console
}  // namespace Stub
}  // namespace Os

#endif  // OS_Stub_Console_HPP
