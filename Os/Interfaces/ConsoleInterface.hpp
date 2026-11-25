// ======================================================================
// \title Os/Interfaces/ConsoleInterface.hpp
// \brief pure-virtual interface implemented by Os::Console implementations
// ======================================================================
#ifndef OS_INTERFACES_CONSOLEINTERFACE_HPP_
#define OS_INTERFACES_CONSOLEINTERFACE_HPP_

#include <Os/Interfaces/ConsoleUserInterface.hpp>

namespace Os {

// \brief Interface defining the properties of the console
class ConsoleInterface : public ConsoleUserInterface {
  public:
    //! \brief Default constructor
    ConsoleInterface() = default;

    //! \brief Default destructor
    virtual ~ConsoleInterface() = default;

    //! \brief write message to console
    //!
    //! Write a message to the console with a bounded size.
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
};

}  // namespace Os

#endif  // OS_INTERFACES_CONSOLEINTERFACE_HPP_
