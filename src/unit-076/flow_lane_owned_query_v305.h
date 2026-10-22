#ifndef LAB_FLOW_LANE_OWNED_QUERY_V305_H
#define LAB_FLOW_LANE_OWNED_QUERY_V305_H
#include "flow_lane_sealed_query_v305.h"
typedef struct {
    FlowLaneFlowSeal seal;
    fln_u64 data[FLOW_SEAL_BYTES/8];
    fln_u64 area_table,path_rows,zero;
    fln_u32 area_count,path_count,ready,reserved;
} FlowLaneOwnedFlow;
/* Exclusive callback-owned snapshot. Open validates/copies the live graph and
 * relocates every pointer used by native containment/interpolation. Queries
 * read only this immutable copy (plus retail code/constants and their inputs).
 * They return snapshot results, even if live data changes in the meantime.
 * Close MUST succeed under the retained owner/allocation scope before any
 * decision is committed. Close performs the full live parent-before-child
 * comparison and invalidates the snapshot on every result. */
fln_u32 lab_flow_owned_open(const FlowLaneNativeFrame *,FlowLaneOwnedFlow *);
fln_u32 lab_flow_owned_query(const FlowLaneOwnedFlow *,const float [3],FlowLaneFlowResult *);
fln_u32 lab_flow_owned_close(FlowLaneOwnedFlow *);
#endif
