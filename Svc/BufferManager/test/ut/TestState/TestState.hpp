// ======================================================================
// \title  TestState.hpp
// \brief  Shadow state model for BufferManager rule-based testing
//
// Models only what the ports expose: the bin table, free counts per bin, the buffers held by the test
// and the telemetry counters. It knows nothing of how the component lays out or indexes its pool.
// ======================================================================
#ifndef Svc_BufferManagerTestState_HPP
#define Svc_BufferManagerTestState_HPP

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

    // ----------------------------------------------------------------------
    // Buffer context encoding (SDD 3.6.2): manager ID in the high 16 bits, buffer ID in the low 16 bits
    // ----------------------------------------------------------------------

    static U32 contextOf(U16 mgrId, U32 bufferId);
    static U32 managerIdOf(const Fw::Buffer& buffer);
    static U32 bufferIdOf(const Fw::Buffer& buffer);

  public:
    BufferManagerTestState();

    // ----------------------------------------------------------------------
    // Updates mirroring component actions
    // ----------------------------------------------------------------------

    //! Stage the bin table the next setup() will use
    void stageBins(const BufferManagerComponentImpl::BufferBins& table);

    //! Mirror setup() with the staged bins: the per-pool state restarts while the lifetime counters, event
    //! throttles and last telemetry written persist
    void configure();

    //! Mirror cleanup(): the pool and every buffer out of it are gone; CurrBuffs restarts at the next setup()
    void recordCleanup();

    //! Record a successful allocation served from bin
    void recordGet(const Fw::Buffer& buffer, U16 bin, FwSizeType requested, U8 pattern);

    //! Record a failed allocation
    void recordNoBuffer();

    //! Record a rejected zero-sized request (counted in NoBuffs, event throttled)
    void recordZeroRequest();

    //! Record the return of held buffer at index
    void recordReturn(FwSizeType index);

    //! Record the return of an empty buffer
    void recordEmptyReturn();

    //! Record the values schedIn just wrote, for the next update-on-change check
    void recordTelemetry();

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

    //! True if some bin with a free buffer is more than one byte larger than the bin before it
    bool hasBetweenBinSize() const;

    //! Random request size strictly between two adjacent bin sizes that can be served (requires hasBetweenBinSize())
    FwSizeType pickBetweenBinSize() const;

    //! Random request size that can currently be served (at least 1)
    FwSizeType pickServableSize() const;

    //! Random request size that cannot currently be served
    FwSizeType pickUnservableSize() const;

    //! Random index into the held buffers
    FwSizeType pickHeldIndex() const;

    //! True if the given buffer ID belongs to a held buffer
    bool isHeldId(U32 id) const;

    //! True if the given buffer's [data, data + capacity) overlaps any held buffer
    bool overlapsHeld(const Fw::Buffer& buffer) const;

  public:
    // ----------------------------------------------------------------------
    // Shadow state
    // ----------------------------------------------------------------------

    BufferManagerComponentImpl::BufferBins bins;  //!< Bins staged for the next setup()
    bool isSetup;                                 //!< True between setup() and cleanup()
    bool everSetup;                               //!< True once setup() has been called

    FwSizeType binSize[BUFFERMGR_MAX_NUM_BINS];  //!< Buffer size per configured bin
    U16 binFree[BUFFERMGR_MAX_NUM_BINS];         //!< Free buffers per configured bin
    U16 numBins;                                 //!< Number of configured (non-empty) bins

    std::vector<HeldBuffer> held;  //!< Buffers obtained and not yet returned

    U32 totalBuffs;  //!< Expected TotalBuffs telemetry
    U32 currBuffs;   //!< Expected CurrBuffs telemetry
    U32 highWater;   //!< Expected HiBuffs telemetry
    U32 noBuffs;     //!< Expected NoBuffs telemetry
    U32 emptyBuffs;  //!< Expected EmptyBuffs telemetry

    U32 noBuffsEvents;     //!< NoBuffsAvailable events emitted so far (throttled)
    U32 zeroEvents;        //!< ZeroSizeRequest events emitted so far (throttled)
    U32 emptyBuffsEvents;  //!< NullEmptyBuffer events emitted so far (throttled)

    bool tlmSent;        //!< True once schedIn has written telemetry
    U32 lastTotalBuffs;  //!< Last TotalBuffs value written
    U32 lastCurrBuffs;   //!< Last CurrBuffs value written
    U32 lastHighWater;   //!< Last HiBuffs value written
    U32 lastNoBuffs;     //!< Last NoBuffs value written
    U32 lastEmptyBuffs;  //!< Last EmptyBuffs value written
};

}  // namespace Svc

#endif
