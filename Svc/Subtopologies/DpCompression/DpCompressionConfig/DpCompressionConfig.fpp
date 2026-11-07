module DpCompressionConfig {
    #Base ID for the DpCompression Subtopology, all components are offsets from this base ID
    constant BASE_ID = 0x04100000
    
    # Buffer management constants

    module ZLibBuffMgr {
        # compressionBufferStoreSize must be <= allocBufferStoreSize: BufferManager asserts on bins not in non-decreasing size order
        constant compressionBufferStoreSize  = 32*1024
        constant compressionBufferStoreCount = 2
        constant allocBufferStoreSize  = 269000
        constant allocBufferStoreCount = 2
        constant dpZLibCompressorBufferManagerId  = 301
    }
}

