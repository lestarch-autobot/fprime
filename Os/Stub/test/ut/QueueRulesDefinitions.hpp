#ifndef OS_STUB_TEST_UT_QUEUE_RULES_DEFINITIONS
#define OS_STUB_TEST_UT_QUEUE_RULES_DEFINITIONS
#include <Fw/FPrimeBasicTypes.hpp>
#include <functional>

using PriorityCompare = std::greater<FwQueuePriorityType>;
//! Largest message held by the fake queue of StubQueueTests.cpp
constexpr FwSizeType QUEUE_MESSAGE_SIZE_UPPER_BOUND = 1024;
constexpr FwSizeType QUEUE_DEPTH_UPPER_BOUND = 100;
constexpr bool TESTS_SUPPORT_BLOCKING = false;

#endif  // OS_STUB_TEST_UT_QUEUE_RULES_DEFINITIONS
