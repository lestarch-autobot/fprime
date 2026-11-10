@ The Buffer Accumulator instance accepted and enqueued a buffer. To avoid uncontrolled sending of events, this event occurs only when the previous buffer received caused a QueueFull error.
event BA_BufferAccepted \
  severity activity high \
  id 0x00 \
  format "Buffer accepted"

@ The Buffer Accumulator instance received a buffer when its queue was full. To avoid uncontrolled sending of events, this event occurs only when the previous buffer received did not cause a QueueFull error.
event BA_QueueFull \
  severity warning high \
  id 0x01 \
  format "Queue full"

@ Got DrainBuffers command while executing DrainBuffers command
event BA_StillDraining(
                        numDrained: U32
                        numToDrain: U32
                      ) \
  severity warning high \
  id 0x02 \
  format "Still draining {} of {}"

@ Got DrainBuffers command while in DRAIN mode
event BA_AlreadyDraining \
  severity warning high \
  id 0x03 \
  format "Already in DRAIN mode"

@ Ran out of buffers while executing DrainBuffers command
event BA_DrainStalled(
                       numDrained: U32
                       numToDrain: U32
                     ) \
  severity warning high \
  id 0x04 \
  format "Drain stalling - only drained {} of {}"

@ Finished DrainBuffers command
event BA_PartialDrainDone(
                           numDrained: U32
                         ) \
  severity activity high \
  id 0x05 \
  format "Partial drain of {} finished"

@ Not enough buffers to complete requested drain, and NOBLOCK was set; will only drain what we have
event BA_NonBlockDrain(
                        numWillDrain: U32
                        numReqDrain: U32
                      ) \
  severity warning low \
  id 0x06 \
  format "Only have {}; requested drain of {}"

@ BA_SetDrainLimit accepted; the drain limit is now maxOutstanding
event BA_DrainLimitSet(
                        maxOutstanding: U32 @< The new drain limit
                      ) \
  severity activity high \
  id 0x07 \
  format "Drain limit set to {}"

@ BA_SetDrainLimit rejected: the requested drain limit is 0 or exceeds the queue depth; the drain limit is unchanged
event BA_InvalidDrainLimit(
                            maxOutstanding: U32 @< The rejected value
                            maxAllowed: U32 @< The largest accepted value (the queue depth, maxNumBuffers)
                          ) \
  severity warning high \
  id 0x08 \
  format "Invalid drain limit {}; must be in 1..{}"

@ A buffer arrived on bufferSendInReturn while no drained buffers were outstanding; it was forwarded on bufferSendOutReturn. To avoid uncontrolled sending of events, this event occurs only when the previous returned buffer did not also arrive with no buffers outstanding.
event BA_UnexpectedReturn \
  severity warning high \
  id 0x09 \
  format "Buffer returned with no buffers outstanding"
