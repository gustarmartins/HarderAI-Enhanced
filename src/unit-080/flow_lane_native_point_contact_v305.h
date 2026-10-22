#ifndef LAB_FLOW_LANE_NATIVE_POINT_CONTACT_V305_H
#define LAB_FLOW_LANE_NATIVE_POINT_CONTACT_V305_H
#include "flow_lane_native_binding_mesh.h"
enum { FLPC_REJECTED=0,FLPC_INTERIOR=1,FLPC_EDGE=2,FLPC_VERTEX=3 };
typedef struct {
    unsigned hit,category,branch,response_bit;
    float corrected_depth,raw_depth;
} FlowLanePointContact;
/* Exact ordinary single-prism7C6C18 float arithmetic for flags0. No material
 * reinterpretation, previous-position mode, native calls or query-state writes.
 * Return0=invalid input,1=evaluated. Only hit=true has usable priority depth.
 * The caller holds native model/arrays/category lifetime throughout the call.
 * Build with floating-point contraction and math errno disabled. */
unsigned lab_flow_lane_point_contact(const FlowLaneMeshModel *,unsigned prism_index,
    const float center[3],float radius,FlowLanePointContact *);
#endif
