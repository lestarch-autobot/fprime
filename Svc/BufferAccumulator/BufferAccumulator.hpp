// ======================================================================
// \title  BufferAccumulator.hpp
// \author bocchino
// \brief  BufferAccumulator interface
//
// \copyright
// Copyright (C) 2017 California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
//
// ======================================================================

#ifndef Svc_BufferAccumulator_HPP
#define Svc_BufferAccumulator_HPP

#include <Fw/Types/MemAllocator.hpp>

#include "Os/Queue.hpp"
#include "Svc/BufferAccumulator/BufferAccumulatorComponentAc.hpp"

namespace Svc {

// Forward declaration for UTs
namespace Accumulate {
class BufferAccumulatorTester;
}
namespace Drain {
class BufferAccumulatorTester;
}
namespace Errors {
class BufferAccumulatorTester;
}
namespace DrainLimit {
class BufferAccumulatorTester;
}

class BufferAccumulator final : public BufferAccumulatorComponentBase {
    friend class BufferAccumulatorTester;
    friend class Svc::Accumulate::BufferAccumulatorTester;
    friend class Svc::Drain::BufferAccumulatorTester;
    friend class Svc::Errors::BufferAccumulatorTester;
    friend class Svc::DrainLimit::BufferAccumulatorTester;

  private:
    // ----------------------------------------------------------------------
    // Types
    // ----------------------------------------------------------------------

    //! A BufferLogger file
    class ArrayFIFOBuffer {
      public:
        //! Construct an ArrayFIFOBuffer object
        ArrayFIFOBuffer();

        //! Destroy an ArrayFIFOBuffer File object
        ~ArrayFIFOBuffer();

        void init(Fw::Buffer* const elements,  //!< The array elements
                  FwSizeType capacity          //!< The capacity
        );

        //! Enqueue an index.
        //! Fails if the queue is full.
        //! \return Whether the operation succeeded
        bool enqueue(const Fw::Buffer& e  //!< The element to enqueue
        );

        //! Dequeue an index.
        //! Fails if the queue is empty.
        bool dequeue(Fw::Buffer& e  //!< The dequeued element
        );

        //! Get the size of the queue
        //! \return The size
        FwSizeType getSize() const;

        //! Get the capacity of the queue
        //! \return The capacity
        FwSizeType getCapacity() const;

      private:
        // ----------------------------------------------------------------------
        // Private member variables
        // ----------------------------------------------------------------------

        //! The memory for the elements
        Fw::Buffer* m_elements;

        //! The capacity of the queue
        FwSizeType m_capacity;

        //! The enqueue index
        FwSizeType m_enqueueIndex;

        //! The dequeue index
        FwSizeType m_dequeueIndex;

        //! The size of the queue
        FwSizeType m_size;
    };  // class ArrayFIFOBuffer

  public:
    // ----------------------------------------------------------------------
    // Construction, initialization, and destruction
    // ----------------------------------------------------------------------

    //! Construct BufferAccumulator instance
    //!
    BufferAccumulator(const char* const compName /*!< The component name*/
    );

    //! Destroy BufferAccumulator instance
    //!
    ~BufferAccumulator();

    // ----------------------------------------------------------------------
    // Public methods
    // ----------------------------------------------------------------------

    //! Give the class a memory buffer. Should be called after constructor
    //! and init, but before task is spawned. Asserts unless
    //! 1 <= initialDrainLimit <= maxNumBuffers; the consumer connected to
    //! bufferSendOutDrain must be able to queue maxNumBuffers buffers at once.
    void allocateQueue(
        FwEnumStoreType identifier,
        Fw::MemAllocator& allocator,
        FwSizeType maxNumBuffers,  //!< The maximum number of buffers
        BufferAccumulator_OpState initialMode = BufferAccumulator_OpState::ACCUMULATE,  //!< The initial operating mode
        U32 initialDrainLimit = 1  //!< The initial drain limit (1 .. maxNumBuffers)
    );

    //! Return allocated queue. Should be done during shutdown
    void deallocateQueue(Fw::MemAllocator& allocator);

    //! Report the initial drain limit and outstanding count once the ports are connected
    void preamble() override;

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for user-defined typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for bufferSendInFill
    //!
    void bufferSendInFill_handler(const FwIndexType portNum,  //!< The port number
                                  Fw::Buffer& buffer);

    //! Handler implementation for bufferSendInReturn
    //!
    void bufferSendInReturn_handler(const FwIndexType portNum,  //!< The port number
                                    Fw::Buffer& buffer);

    //! Handler implementation for pingIn
    //!
    void pingIn_handler(const FwIndexType portNum,  //!< The port number
                        U32 key                     //!< Value to return to pinger
    );

  private:
    // ----------------------------------------------------------------------
    // Command handler implementations
    // ----------------------------------------------------------------------

    //! Implementation for SetMode command handler
    //! Set the mode
    void BA_SetMode_cmdHandler(const FwOpcodeType opCode,             //!< The opcode
                               const U32 cmdSeq,                      //!< The command sequence number
                               const BufferAccumulator_OpState& mode  //!< The mode
    );

    //! Implementation for BA_DrainBuffers command handler
    //! Drain the commanded number of buffers
    void BA_DrainBuffers_cmdHandler(const FwOpcodeType opCode, /*!< The opcode*/
                                    const U32 cmdSeq,          /*!< The command sequence number*/
                                    U32 numToDrain,
                                    const BufferAccumulator_BlockMode& blockMode);

    //! Implementation for BA_SetDrainLimit command handler
    //! Set the maximum number of drained buffers that may be outstanding at once
    void BA_SetDrainLimit_cmdHandler(const FwOpcodeType opCode,  //!< The opcode
                                     const U32 cmdSeq,           //!< The command sequence number
                                     U32 maxOutstanding          //!< The new drain limit
    );

  private:
    // ----------------------------------------------------------------------
    // Private helper methods
    // ----------------------------------------------------------------------

    //! Whether buffers are to be sent downstream: DRAIN mode, or a partial drain in progress
    bool isDraining() const;

    //! Send stored buffers, oldest first, while draining and the outstanding count is below the
    //! drain limit; completes a partial drain whose target has been reached.
    //! reportStall: emit BA_DrainStalled if a partial drain would send but the FIFO is empty
    void drain(bool reportStall);

    //! Write the queue depth (on a fill, while draining, or when it changed) and the outstanding count (when it
    //! changed)
    void reportCounts(FwSizeType queuedBefore,  //!< The queue depth at handler entry
                      U32 outstandingBefore,    //!< The outstanding count at handler entry
                      bool isFill               //!< Whether the caller is the fill handler
    );

  private:
    // ----------------------------------------------------------------------
    // Private member variables
    // ----------------------------------------------------------------------

    //! The mode
    BufferAccumulator_OpState m_mode;

    //! Memory for the buffer array
    Fw::Buffer* m_bufferMemory;

    //! The FIFO queue of buffers
    ArrayFIFOBuffer m_bufferQueue;

    //! The maximum number of drained buffers that may be outstanding at once
    U32 m_drainLimit;

    //! The number of drained buffers sent and not yet returned
    U32 m_numOutstanding;

    //! Whether BA_UnexpectedReturn has been emitted since the last expected return
    bool m_unexpectedReturnReported;

    //! The number of QueueFull warnings sent since the last successful enqueue
    //! operation
    U32 m_numWarnings;

    //! The number of buffers drained in a partial drain command
    FwSizeType m_numDrained;

    //! The number of buffers TO drain in a partial drain command
    FwSizeType m_numToDrain;

    //! The DrainBuffers opcode to respond to
    FwOpcodeType m_opCode;

    //! The DrainBuffers cmdSeq to respond to
    U32 m_cmdSeq;

    //! The allocator ID
    FwEnumStoreType m_allocatorId;
};

}  // namespace Svc

#endif
