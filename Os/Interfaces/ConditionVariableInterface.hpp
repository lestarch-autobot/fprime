// ======================================================================
// \title Os/Interfaces/ConditionVariableInterface.hpp
// \brief pure-virtual interface implemented by Os::ConditionVariable implementations
// ======================================================================
#ifndef OS_INTERFACES_CONDITIONVARIABLEINTERFACE_HPP_
#define OS_INTERFACES_CONDITIONVARIABLEINTERFACE_HPP_

#include <Os/Interfaces/ConditionVariableUserInterface.hpp>
#include <Os/Mutex.hpp>

namespace Os {

//! \brief interface for condition variable implementations
class ConditionVariableInterface : public ConditionVariableUserInterface {
  public:
    //! Default constructor
    ConditionVariableInterface() = default;
    //! Default destructor
    virtual ~ConditionVariableInterface() = default;

    //! \brief copy constructor is forbidden
    ConditionVariableInterface(const ConditionVariableInterface& other) = delete;

    //! \brief assignment operator is forbidden
    virtual ConditionVariableInterface& operator=(const ConditionVariableInterface& other) = delete;

    //! \brief wait on a condition variable
    //!
    //! Atomically unlock the provided mutex and block on the condition until a future `notify` or `notifyAll`.
    //!
    //! \param mutex: mutex to unlock as part of this operation
    //! \return status of the conditional wait
    virtual Status pend(Os::Mutex& mutex) = 0;

    //! \brief notify a single waiter on this condition variable
    virtual void notify() = 0;

    //! \brief notify all waiters on this condition variable
    virtual void notifyAll() = 0;

    //! \brief return the underlying condition variable handle (implementation specific).
    //! \return internal condition variable handle representation
    virtual ConditionVariableHandle* getHandle() = 0;
};

}  // namespace Os

#endif  // OS_INTERFACES_CONDITIONVARIABLEINTERFACE_HPP_
