/*
 * TlmPacketizerComponentImplCfg.hpp
 *
 *  Created on: Dec 10, 2017
 *      Author: tim
 */

// \copyright
// Copyright 2009-2015, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.

#ifndef SVC_TLMPACKETIZER_TLMPACKETIZERCOMPONENTIMPLCFG_HPP_
#define SVC_TLMPACKETIZER_TLMPACKETIZERCOMPONENTIMPLCFG_HPP_

#include <Fw/FPrimeBasicTypes.hpp>

namespace Svc {

//! Maximum number of channels that the packetizer can handle. Must be >= number of channels listed in packets,
//! plus the omitted channels unless IGNORE_OMIT_LIST is passed to setPacketList
static const FwChanIdType MAX_PACKETIZER_CHANNELS = 200;

//! Maximum number of memberships (channel entries summed over all packets of the packet list). Must be >= 1
static const FwSizeType MAX_PACKETIZER_MEMBERSHIPS = MAX_PACKETIZER_CHANNELS;

//! Maximum number of missing channels to track and report
static const FwChanIdType TLMPACKETIZER_MAX_MISSING_TLM_CHECK = 25;

}  // namespace Svc

#endif /* SVC_TLMPACKETIZER_TLMPACKETIZERCOMPONENTIMPLCFG_HPP_ */
