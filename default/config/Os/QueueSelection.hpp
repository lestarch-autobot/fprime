// ======================================================================
// \title config/QueueSelection.hpp
// \brief configured selection of the Os::Queue implementation
//
// Selects, at compile time, the implementation wrapped by Os::Queue: include the header defining the implementation and
// name it Os::Selected::Queue. Os/Queue.hpp defines Os::Queue as Os::QueueCommon<Os::Selected::Queue>, which holds the
// implementation by value and forwards to it without virtual dispatch.
//
// The implementation header must not include Os/Queue.hpp (it derives from Os/Interfaces/QueueInterface.hpp). The alias
// only changes the C++ type; the module providing the implementation must still be in the link.
// ======================================================================
#ifndef CONFIG_QUEUESELECTION_HPP
#define CONFIG_QUEUESELECTION_HPP

#include <Os/Generic/PriorityQueue.hpp>

namespace Os {
namespace Selected {
using Queue = Generic::PriorityQueue;
}  // namespace Selected
}  // namespace Os

#endif  // CONFIG_QUEUESELECTION_HPP
