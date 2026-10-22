#ifndef LAB_FLOW_LANE_SEALED_QUERY_V305_H
#define LAB_FLOW_LANE_SEALED_QUERY_V305_H
#include "flow_lane_native_flow_query_v305.h"
#define FLOW_SEAL_SPANS 4096
#define FLOW_SEAL_BYTES 262144
typedef struct {fln_u64 address;fln_u32 size,offset;} FlowLaneSealedSpan;
typedef struct {
    FlowLaneNativeFrame frame;
    fln_u64 course_words[10];
    fln_u32 ready,span_count,bytes,reserved;
    FlowLaneSealedSpan spans[FLOW_SEAL_SPANS];
    fln_u64 data[FLOW_SEAL_BYTES/8];
} FlowLaneFlowSeal;
/* One native owner invocation, exclusive module-owned scratch. Open validates
 * the same course graph as the ordinary query, and captures all its borrowed
 * course bytes. Every query compares those bytes before AND after actual
 * retail execution, parent regions before children. No cross-tick reuse.
 * Caller retains actual native job/allocation lifetime throughout, and must
 * close successfully before committing a decision. No native object writes. */
fln_u32 lab_flow_lane_seal_open(const FlowLaneNativeFrame *,FlowLaneFlowSeal *);
fln_u32 lab_flow_lane_sealed_query(FlowLaneFlowSeal *,const float [3],FlowLaneFlowResult *);
fln_u32 lab_flow_lane_seal_close(FlowLaneFlowSeal *);
#endif
