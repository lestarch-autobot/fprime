// ======================================================================
// \title  BufferManagerTester.hpp
// \author tcanham
// \brief  hpp file for BufferManager test harness implementation class
//
// \copyright
// Copyright 2009-2015, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
//
// ======================================================================

#ifndef TESTER_HPP
#define TESTER_HPP

#include <STest/Pick/Pick.hpp>
#include "BufferManagerGTestBase.hpp"
#include "Fw/Types/MallocAllocator.hpp"
#include "Svc/BufferManager/BufferManagerComponentImpl.hpp"
#include "Svc/BufferManager/test/ut/TestState/TestState.hpp"
#include "TestUtils/RuleBasedTesting.hpp"

namespace Svc {

class BufferManagerTester : public BufferManagerGTestBase {
  public:
    //! Memory segment identifier passed to setup()
    static const FwEnumStoreType MEM_ID = 49;
    //! Manager identifier passed to setup()
    static const U16 MGR_ID = 32;

  public:
    BufferManagerTester();
    ~BufferManagerTester();

  public:
    // ----------------------------------------------------------------------
    // Directed tests
    // ----------------------------------------------------------------------

    //! White-box check of the contiguous [structs][payloads] memory layout
    void contiguousLayout();
    //! Setup, allocator contract, telemetry and buffer identity through the ports
    void setupBehavior();
    void oneBufferSize();
    void multBuffSize();
    void bufferSizeTrimmed();
    //! Bin selection for sizes between, at, and beyond the bin boundaries
    void bufferSizeSelection();
    //! Warning events throttle while the counters keep counting
    void eventThrottling();
    //! Buffers do not alias each other's memory
    void payloadIntegrity();
    //! Repeated cleanup and destruction release the pool exactly once
    void lifecycle();
    void setupSizeOverflowAsserts();
    //! Bins not ordered by non-decreasing size are rejected
    void setupBinOrderAsserts();
    //! Returned buffers with a bad context, pointer, size, or state assert
    void sendInAsserts();
    //! Port use and cleanup before setup() assert
    void useBeforeSetupAsserts();

  public:
    // ----------------------------------------------------------------------
    // Rule-based testing
    // ----------------------------------------------------------------------

    //! Set up the component and the shadow with the given bins
    void setupRules(const BufferManagerComponentImpl::BufferBins& bins);
    //! Set up with a single bin
    void setupRulesOneBin();
    //! Set up with the three-bin test configuration
    void setupRulesDefault();
    //! Set up with a random number of random, non-decreasing bins
    void setupRulesRandom();
    //! Return every held buffer and clean up the component
    void teardownRules();

    //! Rules for the bufferGetCallee port
    FW_RBT_DEFINE_RULE(BufferManagerTester, BufferGet, Ok);
    FW_RBT_DEFINE_RULE(BufferManagerTester, BufferGet, NoneAvailable);
    //! Rules for the bufferSendIn port
    FW_RBT_DEFINE_RULE(BufferManagerTester, BufferReturn, Ok);
    FW_RBT_DEFINE_RULE(BufferManagerTester, BufferReturn, Trimmed);
    FW_RBT_DEFINE_RULE(BufferManagerTester, BufferReturn, Empty);
    //! Rules for the schedIn port
    FW_RBT_DEFINE_RULE(BufferManagerTester, Sched, Telemetry);

    //! Shadow state for rule-based testing
    BufferManagerTestState shadow;

    static_assert(BufferManagerTestState::EVENT_THROTTLE ==
                      BufferManagerComponentImpl::EVENTID_NOBUFFSAVAILABLE_THROTTLE,
                  "shadow throttle must match the NoBuffsAvailable throttle in the model");
    static_assert(BufferManagerTestState::EVENT_THROTTLE ==
                      BufferManagerComponentImpl::EVENTID_NULLEMPTYBUFFER_THROTTLE,
                  "shadow throttle must match the NullEmptyBuffer throttle in the model");

  private:
    // ----------------------------------------------------------------------
    // Helpers
    // ----------------------------------------------------------------------

    //! Fill bins with the three-bin test configuration
    static void defaultBins(BufferManagerComponentImpl::BufferBins& bins);
    //! Request a buffer and check its size, capacity and manager ID
    Fw::Buffer getBuffer(FwSizeType size, FwSizeType expectedCapacity);
    //! Request a buffer that cannot be served and check the empty result and event
    void getNoBuffer(FwSizeType size);
    //! Return a buffer and check that no event was emitted
    void returnBuffer(Fw::Buffer& buffer);

    void connectPorts();
    void initComponents();

  private:
    BufferManagerComponentImpl component;
    Fw::MallocAllocator m_rulesAllocator;

    void textLogIn(const FwEventIdType id,          //!< The event ID
                   const Fw::Time& timeTag,         //!< The time
                   const Fw::LogSeverity severity,  //!< The severity
                   const Fw::TextLogString& text    //!< The event string
                   ) override;
};

}  // end namespace Svc

#endif
