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

#ifndef Svc_BufferManagerTester_HPP
#define Svc_BufferManagerTester_HPP

#include <cstddef>
#include <cstdlib>
#include "BufferManagerGTestBase.hpp"
#include "Fw/Types/MemAllocator.hpp"
#include "Svc/BufferManager/BufferManagerComponentImpl.hpp"
#include "Svc/BufferManager/test/ut/TestState/TestState.hpp"
#include "TestUtils/RuleBasedTesting.hpp"

namespace Svc {

//! Instrumented allocator recording the latest allocation request and every deallocation. It honors any
//! power-of-two alignment (over-allocating and aligning within), so the alignment setup() asks for is observable
//! both as the request and in the memory handed back; setMisaligned() makes it break the alignment contract.
class TestAllocator : public Fw::MemAllocator {
  public:
    ~TestAllocator() override = default;
    void* allocate(const FwEnumStoreType identifier,
                   FwSizeType& size,
                   bool& recoverable,
                   FwSizeType alignment) override {
        this->m_reqId = identifier;
        this->m_reqSize = size;
        this->m_reqAlignment = alignment;
        recoverable = false;
        this->m_raw = ::malloc(size + alignment);
        if (this->m_raw == nullptr) {
            size = 0;
            this->m_mem = nullptr;
            return nullptr;
        }
        PlatformPointerCastType address = reinterpret_cast<PlatformPointerCastType>(this->m_raw);
        address = (address + alignment - 1) & ~static_cast<PlatformPointerCastType>(alignment - 1);
        this->m_mem = reinterpret_cast<void*>(address + (this->m_misaligned ? 1 : 0));
        return this->m_mem;
    }
    void deallocate(const FwEnumStoreType identifier, void* ptr) override {
        this->m_deallocCount++;
        this->m_deallocPtr = ptr;
        ::free(this->m_raw);
        this->m_raw = nullptr;
    }
    //! Hand back memory one byte past the aligned address: an allocator that does not honor the alignment
    void setMisaligned(bool misaligned) { this->m_misaligned = misaligned; }
    FwEnumStoreType getId() const { return this->m_reqId; }
    FwSizeType getSize() const { return this->m_reqSize; }
    FwSizeType getAlignment() const { return this->m_reqAlignment; }
    void* getMem() const { return this->m_mem; }
    U32 getDeallocCount() const { return this->m_deallocCount; }
    void* getDeallocPtr() const { return this->m_deallocPtr; }

  private:
    void* m_raw = nullptr;  //!< The malloc() block the aligned memory lies in
    bool m_misaligned = false;
    FwEnumStoreType m_reqId = 0;
    FwSizeType m_reqSize = 0;
    FwSizeType m_reqAlignment = 0;
    void* m_mem = nullptr;
    U32 m_deallocCount = 0;
    void* m_deallocPtr = nullptr;
};

class BufferManagerTester : public BufferManagerGTestBase {
  public:
    //! Memory segment identifier passed to setup()
    static const FwEnumStoreType MEM_ID = 49;
    //! Manager identifier passed to setup()
    static const U16 MGR_ID = 32;
    //! An arbitrary request size for rules where the value cannot matter
    static const FwSizeType ANY_REQUEST_SIZE = 10;
    //! Number of telemetry channels schedIn writes on its first call
    static const U32 NUM_TLM_CHANNELS = 5;
    //! Regex every FW_ASSERT death prints (nothing in the Tester asserts inside a death statement)
    static constexpr const char* UUT_ASSERT = "Assert: ";
    //! The pool alignment setup() uses when none is given: Fw::MemAllocator::allocate()'s own default
    static const FwSizeType DEFAULT_ALIGNMENT = alignof(std::max_align_t);
    //! A pool alignment stricter than the allocator default
    static const FwSizeType ABOVE_DEFAULT_ALIGNMENT = 4 * DEFAULT_ALIGNMENT;
    //! The loosest pool alignment, below that of anything setup() places in the pool
    static const FwSizeType MIN_ALIGNMENT = 1;

    //! The three-bin test configuration (see defaultBins())
    static const Fw::Buffer::SizeType BIN0_BUFFER_SIZE = 10;
    static const U16 BIN0_NUM_BUFFERS = 2;
    static const Fw::Buffer::SizeType BIN1_BUFFER_SIZE = 12;
    static const U16 BIN1_NUM_BUFFERS = 4;
    static const Fw::Buffer::SizeType BIN2_BUFFER_SIZE = 100;
    static const U16 BIN2_NUM_BUFFERS = 3;
    static const U16 NUM_BINS = 3;
    static constexpr Fw::Buffer::SizeType BIN_SIZES[NUM_BINS] = {BIN0_BUFFER_SIZE, BIN1_BUFFER_SIZE, BIN2_BUFFER_SIZE};
    static constexpr U16 BIN_COUNTS[NUM_BINS] = {BIN0_NUM_BUFFERS, BIN1_NUM_BUFFERS, BIN2_NUM_BUFFERS};
    static const U16 TOTAL_NUM_BUFFERS = BIN0_NUM_BUFFERS + BIN1_NUM_BUFFERS + BIN2_NUM_BUFFERS;

  public:
    BufferManagerTester();
    ~BufferManagerTester();

  public:
    // ----------------------------------------------------------------------
    // Directed tests
    // ----------------------------------------------------------------------

    //! Setup, allocator contract, telemetry and buffer identity through the ports
    void setupBehavior();
    //! The alignment given to setup() reaches the allocator and the first buffer handed out has it
    void setupAlignment();
    //! A single bin: exhaustion, return in any order, every buffer available again
    void singleBin();
    //! Three bins: requests of each bin's size drain that bin and the larger ones in order
    void multipleBins();
    //! A buffer's size is trimmed to the request and it returns to its slot
    void trimmedReturn();
    //! Bin selection for sizes between, at, and beyond the bin boundaries; a size-0 request is refused
    void bufferSizeSelection();
    //! Warning events throttle while the counters keep counting
    void eventThrottling();
    //! Buffers do not alias each other's memory
    void payloadIntegrity();
    //! Cleanup and destruction release the pool exactly once; use after cleanup, setup over a live pool and
    //! empty configurations assert; setup after cleanup restarts the pool and its high-water mark
    void lifecycle();
    //! Bin tables whose pool-size arithmetic would wrap are rejected
    void setupSizeOverflowAsserts();
    //! Bins not ordered by non-decreasing size are rejected; equal sizes and gaps are accepted
    void setupBinOrderAsserts();
    //! Alignments that are zero or not a power of two, and an allocator that does not honor the alignment, assert
    void setupAlignmentAsserts();
    //! Returned buffers with a bad context, pointer, size, or state assert
    void sendInAsserts();
    //! Port use and cleanup before setup() assert
    void useBeforeSetupAsserts();

  public:
    // ----------------------------------------------------------------------
    // Rule-based testing
    // ----------------------------------------------------------------------

    //! Stage bins (and the pool alignment to request) for the Lifecycle.Setup rule and apply it
    void setupRules(const BufferManagerComponentImpl::BufferBins& bins, FwSizeType alignment = DEFAULT_ALIGNMENT);
    //! Stage a single bin of one buffer and set up
    void setupRulesOneBuffer();
    //! Stage a single bin and set up
    void setupRulesOneBin();
    //! Stage the three-bin test configuration and set up
    void setupRulesDefault();
    //! Stage two equal-size bins, an empty bin, then a larger bin (a legal irregular table) and set up
    void setupRulesEqualAndGapped();
    //! Stage a random number of random, non-decreasing bins and a random pool alignment, and set up
    void setupRulesRandom();
    //! Return every held buffer and clean up the component (Lifecycle.Cleanup)
    void teardownRules();

    //! Rules for setup() and cleanup()
    FW_RBT_DEFINE_RULE(BufferManagerTester, Lifecycle, Setup);
    FW_RBT_DEFINE_RULE(BufferManagerTester, Lifecycle, Cleanup);
    FW_RBT_DEFINE_RULE(BufferManagerTester, Lifecycle, CleanupHolding);
    FW_RBT_DEFINE_RULE(BufferManagerTester, Lifecycle, NotSetupAsserts);
    FW_RBT_DEFINE_RULE(BufferManagerTester, Lifecycle, BinOrderAsserts);
    FW_RBT_DEFINE_RULE(BufferManagerTester, Lifecycle, SizeOverflowAsserts);
    FW_RBT_DEFINE_RULE(BufferManagerTester, Lifecycle, AlignmentAsserts);
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
    // Bin table builders
    // ----------------------------------------------------------------------

    //! The three-bin test configuration
    static BufferManagerComponentImpl::BufferBins defaultBins();
    //! A single bin of count buffers of the given size
    static BufferManagerComponentImpl::BufferBins oneBin(Fw::Buffer::SizeType size, U16 count);
    //! Bin 0 plus a second bin at the given index (bins between are left empty)
    static BufferManagerComponentImpl::BufferBins twoBins(Fw::Buffer::SizeType size0,
                                                          U16 count0,
                                                          U16 index1,
                                                          Fw::Buffer::SizeType size1,
                                                          U16 count1);
    //! Two equal-size bins, an empty bin, then a larger bin: a legal irregular configuration
    static BufferManagerComponentImpl::BufferBins equalAndGappedBins();

    // ----------------------------------------------------------------------
    // Port helpers (each clears the history it checks)
    // ----------------------------------------------------------------------

    //! Request a buffer and check its size, capacity and manager ID; no event is emitted
    Fw::Buffer getBuffer(FwSizeType size, FwSizeType expectedCapacity);
    //! Request every buffer of the three-bin table (defaultBins()) in bin order, each at its bin's size
    void getAllBuffers(Fw::Buffer (&buffers)[TOTAL_NUM_BUFFERS]);
    //! Request a buffer that cannot be served and check the empty result and the NoBuffsAvailable warning
    void getNoBuffer(FwSizeType size);
    //! Return a buffer and check that no event was emitted
    void returnBuffer(Fw::Buffer& buffer);
    //! Return an empty buffer and check the NullEmptyBuffer warning
    void returnEmptyBuffer(Fw::Buffer& empty);
    //! Run schedIn and check how many channels it wrote; the values are checked by the caller
    void checkTelemetry(U32 expectedChannels);
    //! setup() with the given table (and pool alignment) must assert
    void expectSetupDeath(const BufferManagerComponentImpl::BufferBins& bins, FwSizeType alignment = DEFAULT_ALIGNMENT);
    //! Request a buffer expected to be served from the shadow's bin and record it (shared by the BufferGet rules)
    void ruleGet(FwSizeType size);
    //! Hand back held buffer index after checking its pattern and record it (shared by the BufferReturn rules)
    void ruleReturn(FwSizeType index, Fw::Buffer& buffer);

    void connectPorts();
    void initComponents();

  private:
    //! Declared before the component: its destructor may deallocate through this allocator
    TestAllocator m_allocator;
    BufferManagerComponentImpl component;
};

}  // end namespace Svc

#endif
