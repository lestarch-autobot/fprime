// ======================================================================
// \title  SubtopologyTopologyDefs.hpp
// \brief subtopology definitions header
//
// ======================================================================
#ifndef FaultProtection_SubtopologyTopologyDefs_hpp
#define FaultProtection_SubtopologyTopologyDefs_hpp


namespace FaultProtection {
    struct SubtopologyState {
    };

    struct TopologyState {
        SubtopologyState fp;
    };
}
#endif // FaultProtection_SubtopologyTopologyDefs_hpp