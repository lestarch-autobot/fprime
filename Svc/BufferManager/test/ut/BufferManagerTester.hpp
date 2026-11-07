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

//! Instrumented allocator recording the single allocation and every deallocation
class TestAllocator : public Fw::MemAllocator {
  public:
    TestAllocator() : m_reqId(0), m_reqSize(0), m_mem(nullptr), m_deallocCount(0), m_deallocPtr(nullptr) {}
    virtual ~TestAllocator() {}
    void* allocate(const FwEnumStoreType identifier,
                   FwSizeType& size,
                   bool& recoverable,
                   FwSizeType alignment) override {
        this->m_reqId = identifier;
        this->m_reqSize = size;
        this->m_mem = this->m_alloc.allocate(identifier, size, recoverable);
        return this->m_mem;
    }
    void deallocate(const FwEnumStoreType identifier, void* ptr) override {
        this->m_deallocCount++;
        this->m_deallocPtr = ptr;
        this->m_alloc.deallocate(identifier, ptr);
    }
    FwEnumStoreType getId() { return this->m_reqId; }
    FwSizeType getSize() { return this->m_reqSize; }
    void* getMem() { return this->m_mem; }
    U32 getDeallocCount() { return this->m_deallocCount; }
    void* getDeallocPtr() { return this->m_deallocPtr; }

  private:
    Fw::MallocAllocator m_alloc;
    FwEnumStoreType m_reqId;
    FwSizeType m_reqSize;
    void* m_mem;
    U32 m_deallocCount;
    void* m_deallocPtr;
};

class BufferManagerTester : public BufferManagerGTestBase {
  public:
    //! Memory segment identifier passed to setup()
    static const FwEnumStoreType MEM_ID = 49;
    //! Manager identifier passed to setup()
    static const U16 MGR_ID = 32;
    //! An arbitrary request size for rules where the value cannot matter
    static const FwSizeType ANY_REQUEST_SIZE = 10;
    //! The three-bin test configuration (see defaultBins())
    static const Fw::Buffer::SizeType BIN0_BUFFER_SIZE = 10;
    static const U16 BIN0_NUM_BUFFERS = 2;
    static const Fw::Buffer::SizeType BIN1_BUFFER_SIZE = 12;
    static const U16 BIN1_NUM_BUFFERS = 4;
    static const Fw::Buffer::SizeType BIN2_BUFFER_SIZE = 100;
    static const U16 BIN2_NUM_BUFFERS = 3;
    static const U16 TOTAL_NUM_BUFFERS = BIN0_NUM_BUFFERS + BIN1_NUM_BUFFERS + BIN2_NUM_BUFFERS;

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

    //! Stage bins for the Lifecycle.Setup rule and apply it
    void setupRules(const BufferManagerComponentImpl::BufferBins& bins);
    //! Stage a single bin of one buffer and set up
    void setupRulesOneBuffer();
    //! Stage a single bin and set up
    void setupRulesOneBin();
    //! Stage the three-bin test configuration and set up
    void setupRulesDefault();
    //! Stage a random number of random, non-decreasing bins and set up
    void setupRulesRandom();
    //! Return every held buffer and clean up the component (Lifecycle.Cleanup)
    void teardownRules();

    //! Rules for setup() and cleanup()
    FW_RBT_DEFINE_RULE(BufferManagerTester, Lifecycle, Setup);
    FW_RBT_DEFINE_RULE(BufferManagerTester, Lifecycle, Cleanup);
    FW_RBT_DEFINE_RULE(BufferManagerTester, Lifecycle, NotSetupAsserts);
    FW_RBT_DEFINE_RULE(BufferManagerTester, Lifecycle, BinOrderAsserts);
    FW_RBT_DEFINE_RULE(BufferManagerTester, Lifecycle, SizeOverflowAsserts);
    FW_RBT_DEFINE_RULE(BufferManagerTester, Lifecycle, EmptyBinsAsserts);
    FW_RBT_DEFINE_RULE(BufferManagerTester, Lifecycle, SetupTwiceAsserts);
    //! Rules for the bufferGetCallee port
    FW_RBT_DEFINE_RULE(BufferManagerTester, BufferGet, Ok);
    FW_RBT_DEFINE_RULE(BufferManagerTester, BufferGet, Between);
    FW_RBT_DEFINE_RULE(BufferManagerTester, BufferGet, Zero);
    FW_RBT_DEFINE_RULE(BufferManagerTester, BufferGet, NoneAvailable);
    //! Rules for the bufferSendIn port
    FW_RBT_DEFINE_RULE(BufferManagerTester, BufferReturn, Ok);
    FW_RBT_DEFINE_RULE(BufferManagerTester, BufferReturn, Trimmed);
    FW_RBT_DEFINE_RULE(BufferManagerTester, BufferReturn, Empty);
    FW_RBT_DEFINE_RULE(BufferManagerTester, BufferReturn, Asserts);
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
    static_assert(BufferManagerTestState::EVENT_THROTTLE ==
                      BufferManagerComponentImpl::EVENTID_ZEROSIZEREQUEST_THROTTLE,
                  "shadow throttle must match the ZeroSizeRequest throttle in the model");

  private:
    // ----------------------------------------------------------------------
    // Helpers
    // ----------------------------------------------------------------------

    //! Fill bins with the three-bin test configuration
    static void defaultBins(BufferManagerComponentImpl::BufferBins& bins);
    //! Request a buffer expected to be served from the shadow's bin and record it (shared by the BufferGet rules)
    void ruleGet(FwSizeType size);
    //! Hand back held buffer index after checking its pattern and record it (shared by the BufferReturn rules)
    void ruleReturn(FwSizeType index, Fw::Buffer& buffer);
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
    TestAllocator m_rulesAllocator;

    void textLogIn(const FwEventIdType id,          //!< The event ID
                   const Fw::Time& timeTag,         //!< The time
                   const Fw::LogSeverity severity,  //!< The severity
                   const Fw::TextLogString& text    //!< The event string
                   ) override;
};

}  // end namespace Svc

#endif
