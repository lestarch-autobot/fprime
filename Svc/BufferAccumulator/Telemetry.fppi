@ The number of buffers queued
telemetry BA_NumQueuedBuffers: U32 id 0

@ The number of drained buffers sent on bufferSendOutDrain and not yet returned on bufferSendInReturn
telemetry BA_NumOutstandingBuffers: U32 id 1

@ The drain limit: the maximum number of drained buffers that may be outstanding at once
telemetry BA_DrainLimit: U32 id 2
