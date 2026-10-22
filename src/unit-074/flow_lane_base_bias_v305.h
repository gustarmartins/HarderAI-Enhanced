#ifndef LAB_FLOW_LANE_BASE_BIAS_V305_H
#define LAB_FLOW_LANE_BASE_BIAS_V305_H
/* Isolated Big Blue operand generator. Produces a second candidate for the
 * existing travel-cost selector. No native memory, pointers or write authority.
 * Ground/contact, sweep, ownership and expected gain remain caller duties. */
typedef unsigned int flb_u32;
typedef struct {
    flb_u32 current_key,following_key,drift_state,reserved;
    float speed,original_offset,base_half_width;
    float native_center[3],native_side[3],base_center[3],position[3];
} FlowLaneBaseInput;
typedef struct {
    flb_u32 proposed,reason;
    float offset,target[3],shift_squared,base_distance_squared;
} FlowLaneBaseOutput;
enum { FLB_INVALID=1,FLB_REGION=2,FLB_DRIFT=3,FLB_SPEED=4,FLB_BASE=5,
       FLB_SIDE=6,FLB_CENTER=7,FLB_SHIFT=8,FLB_APPROACH=9 };
/* A returned proposal is NOT permission to store it into route+98. */
flb_u32 lab_flow_lane_base_proposal(const FlowLaneBaseInput *,FlowLaneBaseOutput *);
#endif
