module DataProductsConfig {
    #Base ID for the DataProducts Subtopology, all components are offsets from this base ID
    constant BASE_ID = 0x04000000

    module QueueSizes {
        constant dpCat    = 10
        constant dpMgr  = 10
        @ dpBufferAccumulator may have up to BufferAccumulator.maxNumBuffers buffers in flight to dpWriter at once (BA_SetDrainLimit is capped there); this queue must hold all of them plus dpWriter's other queued inputs (dpWriterQueueMargin)
        constant dpWriter   = BufferAccumulator.maxNumBuffers + dpWriterQueueMargin
        @ Messages other than buffers that can be pending on dpWriter's queue at once: 1 schedIn tick (re-posted via the internal schedInInternal port) + an assumed ceiling of 3 concurrently queued commands (dpWriter has one async command, CLEAR_EVENT_THROTTLE; CmdDispatcher does not bound in-flight commands per component - projects issuing more concurrently must raise this margin)
        constant dpWriterQueueMargin = 4
        constant dpBufferAccumulator = 10
        constant dpBufferManager   = 10
    }


    module StackSizes {
        constant dpCat    = 64 * 1024
        constant dpMgr  = 64 * 1024
        constant dpWriter   = 64 * 1024
        constant dpBufferAccumulator = 64 * 1024
        constant dpBufferManager   = 64 * 1024
    }

    module Priorities {
        constant dpCat    = 24
        constant dpMgr  = 23
        constant dpWriter   = 22
        constant dpBufferAccumulator = 21
        # dpBufferManager is passive; this value is kept for projects that use
        # the shared DataProducts config constants.
        constant dpBufferManager  = 21
    }

    module CpuAffinities {
        constant dpCat    = Os.TASK_DEFAULT
        constant dpMgr  = Os.TASK_DEFAULT
        constant dpWriter   = Os.TASK_DEFAULT
        constant dpBufferAccumulator = Os.TASK_DEFAULT
        # dpBufferManager is intentionally omitted because it is passive.
    }

    module BufferAccumulator {
        constant allocatorId   = 301
        constant maxNumBuffers = 10
        @ Initial drain limit: maximum number of buffers in flight to dpWriter at once (1 .. maxNumBuffers)
        constant initialDrainLimit = 1
    }

    # Buffer management constants
    module BuffMgr {
        constant dpBufferStoreSize  = 10000
        constant dpBufferStoreCount = 10
        constant dpBufferManagerId  = 300
    }

    # Directory and file paths
    module Paths {
        constant dpDir    = "./DpCat"
        constant dpState  = "./DpCat/DpState.dat"
    }
}
