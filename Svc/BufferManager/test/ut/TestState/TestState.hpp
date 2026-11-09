// ======================================================================
// \title  TestState.hpp
// \brief  Shadow state model for BufferManager rule-based testing
//
// The shadow models only what is observable through the component's
// ports: the bin configuration, how many buffers of each bin are free,
// the buffers currently held by the test, and the counters reported in
// telemetry. It deliberately knows nothing about how the component lays
// out its memory or indexes its buffers.
// ======================================================================
#ifndef Svc_BufferManager_BufferManagerTestState_HPP
#define Svc_BufferManager_BufferManagerTestState_HPP

#include <vector>
#include "Fw/Buffer/Buffer.hpp"
#include "Svc/BufferManager/BufferManagerComponentImpl.hpp"

namespace Svc {

class BufferManagerTestState {
  public:
    //! A buffer obtained from the component and not yet returned
    struct HeldBuffer {
        Fw::Buffer buffer;         //!< The buffer as returned by bufferGetCallee
        U16 bin = 0;               //!< Shadow bin expected to have served it
        FwSizeType requested = 0;  //!< Size requested
        U8 pattern = 0;            //!< Byte pattern written over the requested size
    };

    //! Number of events the component emits before a throttled event goes quiet (checked against the model in the
    //! Tester)
    static const U32 EVENT_THROTTLE = 10;

  public:
    BufferManagerTestState();

    //! Reset the shadow to mirror a freshly set up component with the given bins
    void configure(const BufferManagerComponentImpl::BufferBins& bins);

    //! Sum of every configured payload size
    FwSizeType totalPayloadBytes() const;

    //! Random request size strictly between two adjacent bin sizes that can be served (0 if none exists)
    FwSizeType pickBetweenBinSize() const;

    //! True if the given buffer ID (low 16 bits of a context) belongs to a held buffer
    bool isHeldId(U32 id) const;

    // ----------------------------------------------------------------------
    // Queries used by rule preconditions and actions
    // ----------------------------------------------------------------------

    //! True if any bin has a free buffer
    bool anyFree() const;

    //! Largest buffer size among bins with a free buffer (0 if none)
    FwSizeType largestFreeSize() const;

    //! Largest configured bin size
    FwSizeType largestBinSize() const;

    //! Bin expected to serve a request of the given size per FPRIME-BM-002.
    //! Returns false if no free buffer can hold the request.
    bool expectedBin(FwSizeType size, U16& bin) const;

    //! Random request size that can currently be served (at least 1)
    FwSizeType pickServableSize() const;

    //! Random request size that cannot currently be served
    FwSizeType pickUnservableSize() const;

    //! Random index into the held buffers
    FwSizeType pickHeldIndex() const;

    //! True if the given buffer's [data, data + capacity) overlaps any held buffer
    bool overlapsHeld(const Fw::Buffer& buffer) const;

    // ----------------------------------------------------------------------
    // Updates mirroring component actions
    // ----------------------------------------------------------------------

    //! Record a successful allocation served from bin
    void recordGet(const Fw::Buffer& buffer, U16 bin, FwSizeType requested, U8 pattern);

    //! Record a failed allocation
    void recordNoBuffer();

    //! Record a rejected zero-sized request (counted in NoBuffs, event throttled)
    void recordZeroRequest();

    //! Forget every held buffer (the pool they came from was cleaned up underneath them)
    void dropHeld();

    //! Record the return of held buffer at index
    void recordReturn(FwSizeType index);

    //! Record the return of an empty buffer
    void recordEmptyReturn();

  public:
    // ----------------------------------------------------------------------
    // Shadow state
    // ----------------------------------------------------------------------

    BufferManagerComponentImpl::BufferBins shadow_bins;  //!< Bins staged for the next setup()
    bool shadow_isSetup;                                 //!< True between setup() and cleanup()
    bool shadow_everSetup;                               //!< True once setup() has been called

    FwSizeType shadow_binSize[BUFFERMGR_MAX_NUM_BINS];  //!< Buffer size per configured bin
    U16 shadow_binFree[BUFFERMGR_MAX_NUM_BINS];         //!< Free buffers per configured bin
    U16 shadow_numBins;                                 //!< Number of configured (non-empty) bins

    std::vector<HeldBuffer> shadow_held;  //!< Buffers obtained and not yet returned

    U32 shadow_totalBuffs;  //!< Expected TotalBuffs telemetry
    U32 shadow_currBuffs;   //!< Expected CurrBuffs telemetry
    U32 shadow_highWater;   //!< Expected HiBuffs telemetry
    U32 shadow_noBuffs;     //!< Expected NoBuffs telemetry
    U32 shadow_emptyBuffs;  //!< Expected EmptyBuffs telemetry

    U32 shadow_noBuffsEvents;     //!< NoBuffsAvailable events emitted so far (throttled)
    U32 shadow_zeroEvents;        //!< ZeroSizeRequest events emitted so far (throttled)
    U32 shadow_emptyBuffsEvents;  //!< NullEmptyBuffer events emitted so far (throttled)

    bool shadow_tlmSent;        //!< True once schedIn has written telemetry
    U32 shadow_lastTotalBuffs;  //!< Last TotalBuffs value written
    U32 shadow_lastCurrBuffs;   //!< Last CurrBuffs value written
    U32 shadow_lastHighWater;   //!< Last HiBuffs value written
    U32 shadow_lastNoBuffs;     //!< Last NoBuffs value written
    U32 shadow_lastEmptyBuffs;  //!< Last EmptyBuffs value written
};

}  // namespace Svc

#endif
