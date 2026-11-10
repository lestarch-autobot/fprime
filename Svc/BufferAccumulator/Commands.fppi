enum OpState : U8 {
  ACCUMULATE = 0
  DRAIN = 1
}

@ Set the mode
async command BA_SetMode(
                          mode: OpState
                        ) \
  opcode 0x00

enum BlockMode : U8 {
  NOBLOCK = 0
  BLOCK = 1
}

@ Drain the commanded number of buffers
async command BA_DrainBuffers(
                               numToDrain: U32
                               blockMode: BlockMode
                             ) \
  opcode 0x01

@ Set the drain limit: the maximum number of drained buffers that may be outstanding (sent on bufferSendOutDrain and not yet returned on bufferSendInReturn) at once. Applies to DRAIN mode and to BA_DrainBuffers. Accepted in any mode; 0 and values above the queue depth (maxNumBuffers given to allocateQueue) are rejected. The consumer connected to bufferSendOutDrain must be able to queue maxNumBuffers buffers in addition to its other concurrent queued inputs.
async command BA_SetDrainLimit(
                                maxOutstanding: U32 @< The new drain limit; 1 .. maxNumBuffers
                              ) \
  opcode 0x02
