// ======================================================================
// \title  BufferManagerComponentImpl.hpp
// \author tcanham
// \brief  hpp file for BufferManager component implementation class
//
// \copyright
// Copyright 2009-2015, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
//
// ======================================================================

#ifndef BufferManager_HPP
#define BufferManager_HPP

#include <Fw/Types/MemAllocator.hpp>
#include <cstddef>
#include "Svc/BufferManager/BufferManagerComponentAc.hpp"
#include "config/BufferManagerComponentImplCfg.hpp"

namespace Svc {

// To use the class, instantiate an instance of the BufferBins struct below. This
// table specifies N buffers of M size per bin. Up to MAX_NUM_BINS bins can be specified.
// The table is copied when setup() is called, so it does not need to be retained after
// the call.
//
// The rules for specifying bins:
// 1. For each bin (BufferBins.bins[n]), specify the size of the buffers (bufferSize) in the
//    bin and how many buffers for that bin (numBuffers).
// 2. Bins with a non-zero numBuffers must be ordered by non-decreasing bufferSize (empty bins
//    may appear anywhere); setup() asserts otherwise. When receiving a request for a buffer, the
//    component will search for the first buffer from the bins that is equal to or greater
//    than the requested size, starting at the beginning of the table.
// 3. Any unused bins should have numBuffers set to 0.
// 4. A single bin can be specified if a single size is needed.
//
// If a buffer is requested that can't be found among available buffers, the call will
//    return an Fw::Buffer with a size of zero. It is expected that the user will notice
//    and have the appropriate response for the design. A request for a zero-sized buffer
//    is rejected the same way (empty Fw::Buffer plus a warning event) without consuming a
//    buffer. If an empty buffer is returned to the BufferManager instance, a warning event
//    will be issued but no other action will be taken.
//
// setup() takes an AllocationMode selecting how the pool is obtained from the allocator:
// SINGLE_AGGREGATED_ALLOCATION (the default) makes one allocate() call for the buffer payloads, packed in
//    bin order, followed by the tracking structs; INDIVIDUAL_ALLOCATION makes one allocate() call for the
//    tracking structs and then one per buffer payload, each at the requested alignment, for allocators that
//    place or guard each buffer separately. Every allocation is made under the same memory identifier and
//    released by cleanup().
//
// Buffer manager will assert under the following conditions:
// 1. A returned buffer has the incorrect manager ID.
// 2. A returned buffer has an incorrect buffer ID.
// 3. A returned buffer is returned with a correct buffer ID, but it isn't already allocated.
// 4. A returned buffer has an indicated size larger than originally allocated.
// 5. A returned buffer has a pointer different than the one originally allocated.
// 6. The non-empty bins (numBuffers != 0) passed to setup() are not ordered by non-decreasing bufferSize.
// 7. The bins passed to setup() contain no buffers at all.
// 8. setup() is called while the instance is already set up (call cleanup() first).
// 9. A bin with a non-zero numBuffers has a bufferSize of zero.
// 10. The alignment passed to setup() is zero or not a power of two.
// 11. The allocator returns memory not aligned as requested: the aggregated pool at the requested alignment (or
//     that of the buffer tracking structures if larger); in INDIVIDUAL_ALLOCATION each payload at the requested
//     alignment and the tracking structures at their own.
// 12. The allocator returns no memory, or less than requested, for any allocation.
//
// cleanup() does not require every buffer to have been returned: buffers still out become invalid
//    with the pool they came from and must not be used or returned afterwards. After cleanup(), setup()
//    may be called again (with the same or another allocator and bin table); the new pool is released
//    by the next cleanup(). CurrBuffs/HiBuffs restart with the new pool while NoBuffs/EmptyBuffs keep
//    counting over the lifetime of the instance.
//
// Note that a pointer to the Fw::MemAllocator used in setup() is stored for later memory cleanup.
// The instance of the allocator must persist beyond calling the cleanup() function or the
// destructor of BufferManager if cleanup() is not called. If a project-specific manual memory
// allocator is not needed, Fw::MallocAllocator can be used when the default alignment suffices: it
// provides malloc's alignment whatever is requested, so setup() asserts for larger requests.

class BufferManagerComponentImpl final : public BufferManagerComponentBase {
    friend class BufferManagerTester;

  public:
    // ----------------------------------------------------------------------
    // Construction, initialization, and destruction
    // ----------------------------------------------------------------------

    //! Construct object BufferManager
    //!
    BufferManagerComponentImpl(const char* const compName /*!< The component name*/
    );

    // Defines a buffer bin
    struct BufferBin {
        Fw::Buffer::SizeType bufferSize;  //!< size of the buffers in this bin. Set to zero for unused bins.
        U16 numBuffers;                   //!< number of buffers in this bin. Set to zero for unused bins.
    };

    // Set of bins for the BufferManager
    struct BufferBins {
        BufferBin bins[BUFFERMGR_MAX_NUM_BINS];  //!< set of bins to define buffers
    };

    //! How setup() obtains the pool from the allocator
    enum AllocationMode {
        SINGLE_AGGREGATED_ALLOCATION,  //!< One allocate(): the payloads packed in bin order, then the tracking structs
        INDIVIDUAL_ALLOCATION  //!< One allocate() for the tracking structs, then one per payload at the alignment
    };

    //! set up configuration

    void setup(U16 mgrID,                    //!< ID of manager for buffer checking
               FwEnumStoreType memID,        //!< Memory segment identifier
               Fw::MemAllocator& allocator,  //!< memory allocator. MUST be persistent for later deallocation.
                                             //!  MUST persist past destructor if cleanup() not called explicitly.
               const BufferBins& bins,       //!< Set of user bins
               FwSizeType alignment = alignof(std::max_align_t),  //!< Payload alignment, a non-zero power of two.
                                                                  //!  SINGLE_AGGREGATED_ALLOCATION: the pool is
                                                                  //!  requested at least at this (or the tracking
                                                                  //!  structs' alignment if larger) and the first
                                                                  //!  payload starts at it. INDIVIDUAL_ALLOCATION:
                                                                  //!  every payload is requested at it. Default:
                                                                  //!  the allocator's own default (maximum
                                                                  //!  alignment defined by C++).
               AllocationMode allocationMode = SINGLE_AGGREGATED_ALLOCATION  //!< How the pool is obtained from
                                                                             //!  the allocator
    );

    void cleanup();  // Free memory prior to end of program if desired. Otherwise,
                     // will be deleted in destructor

    //! Destroy object BufferManager
    //!
    ~BufferManagerComponentImpl();

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for user-defined typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for bufferSendIn
    //!
    void bufferSendIn_handler(const FwIndexType portNum, /*!< The port number*/
                              Fw::Buffer& fwBuffer);

    //! Handler implementation for bufferGetCallee
    //!
    Fw::Buffer bufferGetCallee_handler(const FwIndexType portNum, /*!< The port number*/
                                       Fw::Buffer::SizeType size);

    //! Handler implementation for schedIn
    //!
    void schedIn_handler(const FwIndexType portNum, /*!< The port number*/
                         U32 context                /*!< The call order*/
    );

    // ----------------------------------------------------------------------
    // Allocation regimes, called by setup() once the bins are validated and counted
    // ----------------------------------------------------------------------

    //! Request size bytes at alignment from the allocator and assert the result (non-null, size and alignment honored)
    void* checkedAllocate(Fw::MemAllocator& allocator, FwSizeType size, FwSizeType alignment);

    //! SINGLE_AGGREGATED_ALLOCATION: one allocation holding the payloads (payloadBytes, packed in bin order) followed
    //! by the tracking structs; sets m_memory, m_buffers and each struct's payload memory and size
    void allocateAggregated(Fw::MemAllocator& allocator, FwSizeType alignment, FwSizeType payloadBytes);

    //! INDIVIDUAL_ALLOCATION: one allocation for the tracking structs, then one per payload at alignment; sets
    //! m_memory, m_buffers and each struct's payload memory and size
    void allocateIndividual(Fw::MemAllocator& allocator, FwSizeType alignment);

    bool m_setup;    //!< flag to indicate component has been setup
    bool m_cleaned;  //!< flag to indicate memory has been cleaned up
    U16 m_mgrId;     //!< stored manager ID for buffer checking

    BufferBins m_bufferBins;  //!< copy of bins supplied by user

    struct AllocatedBuffer {
        Fw::Buffer buff;            //!< Buffer class to give to user
        U8* memory;                 //!< pointer to memory buffer
        Fw::Buffer::SizeType size;  //!< size of the buffer
        bool allocated;             //!< this buffer has been allocated
    };

    void* m_memory;                   //!< the aggregated allocation (payloads then tracking structs), or in
                                      //!< INDIVIDUAL_ALLOCATION the tracking structs' allocation
    AllocatedBuffer* m_buffers;       //!< tracking structs, inside m_memory (after the payloads when aggregated)
    Fw::MemAllocator* m_allocator;    //!< allocator for memory
    FwEnumStoreType m_memId;          //!< identifier for allocator
    U16 m_numStructs;                 //!< number of allocated structs
    AllocationMode m_allocationMode;  //!< how the current pool was obtained, hence how cleanup() releases it

    // stats
    U32 m_highWater;   //!< high watermark for allocations
    U32 m_currBuffs;   //!< number of currently allocated buffers
    U32 m_noBuffs;     //!< number of failures to allocate a buffer
    U32 m_emptyBuffs;  //!< number of empty buffers returned
};

}  // end namespace Svc

#endif
