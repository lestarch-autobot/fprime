module TestDictionaryCheck {

  instance rateGroup: Svc.PassiveRateGroup base id 0x100

  deployment topology TestDictionaryCheck {
    instance rateGroup

    # 3 memberships, 2 channels listed in packets, 3 omitted channels
    telemetry packets TestPackets {
      packet First id 1 group 1 {
        rateGroup.MaxCycleTime
        rateGroup.CycleTime
      }
      packet Second id 2 group 1 {
        rateGroup.MaxCycleTime
      }
    } omit {
      rateGroup.CycleCount
      rateGroup.PortCycleTime
      rateGroup.PortCycleTimeHWM
    }
  }
}
