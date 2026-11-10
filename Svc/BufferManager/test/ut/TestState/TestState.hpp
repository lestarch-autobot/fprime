// ======================================================================
// \title  TestState.hpp
// \brief  Shadow state model for BufferManager rule-based testing
//
// Models only what the ports expose: the bin table, free counts per bin, the buffers held by the test
// and the telemetry counters. It knows nothing of how the component lays out its pool; buffer identity
// is read only from the Fw::Buffer context (encoding below).
// ======================================================================
#ifndef Svc_BufferManagerTestState_HPP
#define Svc_BufferManagerTestState_HPP

#include <cstddef>
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
    // Buffer context encoding: manager ID in the high 16 bits, buffer ID (dense, 0..TotalBuffs-1) in the low 16
    // bits. SDD 3.6.2 names the two fields; the split is the component's port contract.
    // ----------------------------------------------------------------------

    static U32 contextOf(U16 mgrId, U32 bufferId);
    static U32 managerIdOf(const Fw::Buffer& buffer);
    static U32 bufferIdOf(const Fw::Buffer& buffer);

    //! True if value is a non-zero power of two (what setup() accepts as an alignment)
    static bool isPowerOfTwo(FwSizeType value);
    //! True if the buffer's data starts on a multiple of alignment
    static bool isAligned(const Fw::Buffer& buffer, FwSizeType alignment);

  public:
    // ----------------------------------------------------------------------
    // Updates mirroring component actions
    // ----------------------------------------------------------------------

    //! Stage the bin table and the pool alignment the next setup() will use
    void stageBins(const BufferManagerComponentImpl::BufferBins& bins, FwSizeType poolAlignment);

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

  private:
    //! True if bin (1..numBins-1) has a free buffer and is more than one byte larger than the bin before it
    bool hasGapBelow(U16 bin) const;

  public:
    // ----------------------------------------------------------------------
    // Shadow state
    // ----------------------------------------------------------------------

    BufferManagerComponentImpl::BufferBins table = {};  //!< Bin table staged for the next setup()
    FwSizeType alignment = alignof(std::max_align_t);   //!< Pool alignment staged for the next setup() (setup()'s
                                                        //!< own default until stageBins() says otherwise)
    bool isSetup = false;                               //!< True between setup() and cleanup()
    bool everSetup = false;                             //!< True once setup() has been called

    FwSizeType binSize[BUFFERMGR_MAX_NUM_BINS] = {};  //!< Buffer size per configured bin
    U16 binFree[BUFFERMGR_MAX_NUM_BINS] = {};         //!< Free buffers per configured bin
    U16 numBins = 0;                                  //!< Number of configured (non-empty) bins

    std::vector<HeldBuffer> held;  //!< Buffers obtained and not yet returned

    U32 totalBuffs = 0;  //!< Expected TotalBuffs telemetry
    U32 currBuffs = 0;   //!< Expected CurrBuffs telemetry
    U32 highWater = 0;   //!< Expected HiBuffs telemetry
    U32 noBuffs = 0;     //!< Expected NoBuffs telemetry
    U32 emptyBuffs = 0;  //!< Expected EmptyBuffs telemetry

    U32 noBuffsEvents = 0;     //!< NoBuffsAvailable events emitted so far (throttled)
    U32 zeroEvents = 0;        //!< ZeroSizeRequest events emitted so far (throttled)
    U32 emptyBuffsEvents = 0;  //!< NullEmptyBuffer events emitted so far (throttled)

    bool tlmSent = false;    //!< True once schedIn has written telemetry
    U32 lastTotalBuffs = 0;  //!< Last TotalBuffs value written
    U32 lastCurrBuffs = 0;   //!< Last CurrBuffs value written
    U32 lastHighWater = 0;   //!< Last HiBuffs value written
    U32 lastNoBuffs = 0;     //!< Last NoBuffs value written
    U32 lastEmptyBuffs = 0;  //!< Last EmptyBuffs value written
};

}  // namespace Svc

#endif
