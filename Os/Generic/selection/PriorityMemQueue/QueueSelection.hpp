// ======================================================================
// \title config/QueueSelection.hpp
// \brief selection of Generic::Queue::PriorityMemQueue as the Os::Queue implementation
//
// Supplied as one of the CONFIGURATION_HEADERS of the implementation module: the build system places this file at
// config/QueueSelection.hpp when that implementation is chosen (CHOOSES_IMPLEMENTATIONS in register_fprime_config).
// Os/Queue.hpp then defines Os::Queue as Os::QueueCommon<Os::Selected::Queue>.
// ======================================================================
#ifndef CONFIG_QUEUESELECTION_HPP
#define CONFIG_QUEUESELECTION_HPP

#include <Os/Generic/PriorityMemQueue.hpp>

namespace Os {
namespace Selected {
using Queue = Generic::Queue::PriorityMemQueue;
}  // namespace Selected
}  // namespace Os

#endif  // CONFIG_QUEUESELECTION_HPP
