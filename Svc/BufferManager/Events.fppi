@ The BufferManager was unable to allocate a requested buffer
event NoBuffsAvailable(
                        $size: FwSizeType @< The requested size
                      ) \
  severity warning high \
  id 0x00 \
  format "No available buffers of size {}" \
  throttle 10

@ The buffer manager received a null pointer and zero-sized buffer as a return. Probably undetected failed buffer allocation
event NullEmptyBuffer \
  severity warning high \
  id 0x01 \
  format "Received null pointer and zero size buffer" \
  throttle 10

@ The buffer manager received a request for a zero-sized buffer and returned an empty buffer
event ZeroSizeRequest \
  severity warning high \
  id 0x02 \
  format "Zero size buffer request rejected" \
  throttle 10
