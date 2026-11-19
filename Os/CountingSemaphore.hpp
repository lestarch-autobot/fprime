// ======================================================================
// \title Os/CountingSemaphore.hpp
// \brief public Os::CountingSemaphore interface and alias
//
// This header aggregates all definitions needed to use Os::CountingSemaphore:
// the interface, the configured alias, and the concrete implementation type.
//
// The includes below must appear in the order given: each depends on names
// defined by the one before it, so reordering them will fail to compile.
// The ordering constraints are:
//
//   1. config/OsSelection.hpp (CFG) defines the Os::CountingSemaphore type
//      alias by forward-declaring a link-time delegate
//      (DelegateCountingSemaphore) or directly aliasing a concrete
//      implementation (e.g. Os::Posix::Semaphore::PosixCountingSemaphore).
//      Must not include Os OSAL headers (they aren't yet defined).
//
//   2. Os/CountingSemaphoreInterface.hpp (IF) includes CFG first, then defines
//      CountingSemaphoreHandle and CountingSemaphoreInterface.
//
//   3. OS_COUNTING_SEMAPHORE_HEADER (IMPL) is defined by CFG and points to the
//      concrete implementation header. If using delegation, this points to
//      Os/DelegateCountingSemaphore.hpp. If using compile-time selection, it
//      points directly to an implementation (e.g. Os/Posix/CountingSemaphore.hpp).
//
// CountingSemaphoreInterface.hpp must precede OS_COUNTING_SEMAPHORE_HEADER
// here, and CFG must never include either of them (that would form a cycle).
// ======================================================================
#ifndef OS_COUNTING_SEMAPHORE_HPP_
#define OS_COUNTING_SEMAPHORE_HPP_

#include <type_traits>
#include "Os/CountingSemaphoreInterface.hpp"

// Validate that OS_COUNTING_SEMAPHORE_HEADER was defined by config/OsSelection.hpp
#ifndef OS_COUNTING_SEMAPHORE_HEADER
#error "OS_COUNTING_SEMAPHORE_HEADER must be defined in config/OsSelection.hpp"
#endif

#include OS_COUNTING_SEMAPHORE_HEADER

// Checks common to every selection of Os::CountingSemaphore (link-time delegate or compile-time alias)
static_assert(std::is_base_of<Os::CountingSemaphoreInterface, Os::CountingSemaphore>::value,
              "Os::CountingSemaphore must derive from Os::CountingSemaphoreInterface");
static_assert(std::is_constructible<Os::CountingSemaphore, U32>::value,
              "Os::CountingSemaphore must be constructible from a U32 initial count");
static_assert(!std::is_copy_constructible<Os::CountingSemaphore>::value &&
                  !std::is_copy_assignable<Os::CountingSemaphore>::value,
              "Os::CountingSemaphore must not be copyable");

#endif  // OS_COUNTING_SEMAPHORE_HPP_
