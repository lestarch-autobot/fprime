module CdhCore {

    # Test deployment: tlmSend is Svc.TlmPacketizer, loaded with the Ref packet set and no omit list
    instance tlmSend: Svc.TlmPacketizer base id CdhCoreConfig.BASE_ID + 0x06000 \
        queue size CdhCoreConfig.QueueSizes.tlmSend \
        stack size CdhCoreConfig.StackSizes.tlmSend \
        priority CdhCoreConfig.Priorities.tlmSend \
        cpu CdhCoreConfig.CpuAffinities.tlmSend \
    {
        phase Fpp.ToCpp.Phases.configComponents """
        CdhCore::tlmSend.setPacketList(
            Ref::Ref_RefPacketsTlmPackets::packetList,
            Svc::IGNORE_OMIT_LIST,
            1
        );
        """
    }
}
