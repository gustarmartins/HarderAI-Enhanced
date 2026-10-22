#ifndef LAB_FLOW_LANE_NATIVE_BINDING_CONTACT_H
#define LAB_FLOW_LANE_NATIVE_BINDING_CONTACT_H
#include "flow_lane_native_binding_mesh.h"
typedef struct {
    FlowLaneMeshModel model;
    unsigned prism,reserved;
    float center_a[3],center_b[3],radius;
} FlowLaneContactSpanInput;
typedef struct {
    unsigned status,response_bit,category,reserved;
    double begin,end,rounding_bound;
} FlowLaneContactSpan;
/* Sufficient whole-segment interval for the actual ordinary interior branch
 * of7C6C18. Uses stored native prism planes and categories, never reconstructed
 * shared vertices. Edge/vertex cases remain unknown rather than fabricated
 * contact. A conservative binary32 roundoff margin shrinks the interval.
 * Caller owns immutable model storage and real current sphere/trajectory. */
unsigned lab_flow_lane_native_contact_span(const FlowLaneContactSpanInput *,FlowLaneContactSpan *);
#endif
