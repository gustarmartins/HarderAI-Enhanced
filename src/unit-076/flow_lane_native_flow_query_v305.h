#ifndef LAB_FLOW_LANE_NATIVE_FLOW_QUERY_V305_H
#define LAB_FLOW_LANE_NATIVE_FLOW_QUERY_V305_H
#include "flow_lane_native_binding.h"
typedef struct {
    fln_u32 status,accepted;
    int mode;
    fln_u32 reserved;
    float vector[3];
    fln_u32 reserved2;
} FlowLaneFlowResult;
enum { FLF_INVALID=0,FLF_EVALUATED=1,FLF_COURSE=2,FLF_CHANGED=3,FLF_OUTPUT=4 };
/* Internal ordinary-final-target API. The caller retains the actual native
 * owner job and immutable course allocations for this entire call. Pointer
 * validation cannot confer allocation lifetime. Uses retail 80C65C and its
 * actual getters; base0 is supported for the isolated retail-image fixture.
 * This is prospective Area/PullPath flow, not physical floor admission.
 * Only the explicit 32-byte output and local stack are written. Unsupported
 * course structures refuse. Output must not alias the frame/course/input. */
fln_u32 lab_flow_lane_native_flow_query(const FlowLaneNativeFrame *,
    const float position[3],FlowLaneFlowResult *);
#endif
