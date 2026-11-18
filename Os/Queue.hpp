// ======================================================================
// \title Os/Queue.hpp
// \brief public Os::Queue interface and alias
//
// This header aggregates all definitions needed to use Os::Queue:
// the interface, the configured alias, and the concrete implementation type.
//
// The includes below must appear in the order given: each depends on names
// defined by the one before it, so reordering them will fail to compile.
// The ordering constraints are:
//
//   1. config/OsSelection.hpp (CFG) defines the Os::Queue type alias by
//      forward-declaring a link-time delegate (DelegateQueue) or directly
//      aliasing a concrete implementation (e.g. Os::Generic::PriorityQueue).
//      Must not include Os OSAL headers (they aren't yet defined).
//
//   2. Os/QueueInterface.hpp (IF) includes CFG first, then defines
//      QueueHandle, QueueInterface, and QueueRegistry.
//
//   3. OS_QUEUE_HEADER (IMPL) is defined by CFG and points to the concrete
//      implementation header. If using delegation, this points to
//      Os/DelegateQueue.hpp. If using compile-time selection, it points
//      directly to an implementation (e.g. Os/Generic/PriorityQueue.hpp).
//
// QueueInterface.hpp must precede OS_QUEUE_HEADER here,
// and CFG must never include either of them (that would form a cycle).
// ======================================================================
#ifndef Os_Queue_hpp_
#define Os_Queue_hpp_

#include "Os/QueueInterface.hpp"

#include OS_QUEUE_HEADER

#endif  // Os_Queue_hpp_
