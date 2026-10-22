#ifndef LAB_FLOW_LANE_NATIVE_EDGE_CERTIFICATE_V305_H
#define LAB_FLOW_LANE_NATIVE_EDGE_CERTIFICATE_V305_H
#include "flow_lane_decode_cache_v305.h"
#include "flow_lane_native_binding_contact.h"
typedef struct {
    unsigned status,reason,branches,response_bit;
    float depth_minimum,depth_maximum;
} FlowLaneEdgeCertificate;
enum { FLEC_INPUT=1,FLEC_PLANES=2,FLEC_CATEGORY=3,FLEC_EDGE=4,FLEC_VERTEX=5 };
/* Sufficient continuous ordinary7C6C18 admission, including all feasible
 * native interior/edge/vertex branches. No mesh-gap epsilon, radius inflation
 * or invented penetration. Unknown interval branches refuse. Native model
 * lifetime, trajectory ownership and obstacle clearance remain separate. */
unsigned lab_flow_lane_native_edge_certificate(const FlowLaneContactSpanInput *,FlowLaneEdgeCertificate *,FlowLaneDecodeCache *);
#endif
