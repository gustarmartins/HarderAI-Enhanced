#ifndef LAB_FLOW_LANE_NATIVE_BINDING_H
#define LAB_FLOW_LANE_NATIVE_BINDING_H
#include "flow_lane_base_bias_v305.h"
typedef unsigned long long fln_u64;
typedef unsigned int fln_u32;
/* Actual ordinary final-target callback operands. This API is internal to the
 * owned 7BE418 prefix. saved_sp is the untouched retail 7BE068 stack frame;
 * callers outside that execution seam have no native allocation lifetime. */
typedef struct {
    fln_u64 main_base,route,saved_sp;
    fln_u32 center_bits[3],reserved;
} FlowLaneNativeEntry;
typedef struct {
    fln_u64 main_base,route,driver,physics,role,context,source,request;
    fln_u64 state,nav,producer,scene,system,race,group,manager,clock;
    fln_u64 selected,base_course,current,following,static_wrapper,dispatcher,course_data;
    fln_u32 tick,racer,offset_bits,role_bytes;
    FlowLaneBaseInput proposal;
    float up[3],motion[3],body_center[3],body_radius,rotation_radius;
} FlowLaneNativeFrame;
enum { FLN_INVALID=0,FLN_COLLECTED=1,FLN_DYNAMIC_OVERLAP=2,
       FLN_NATIVE_CALLER=3,FLN_NATIVE_OWNER=4,FLN_NATIVE_PHYSICS=5,
       FLN_NATIVE_ROUTE=6,FLN_NATIVE_COLLISION=7 };
/* Structural binding only, never write authorization. Root integration must
 * preserve the native job lifetime and, for identity builds, add its existing
 * storage/frame admission leases. No host receipt can replace those leases. */
fln_u32 lab_flow_lane_native_binding_collect(const FlowLaneNativeEntry *,FlowLaneNativeFrame *);
/* Checks the same additional-wrapper mask and current world AABBs as native
 * 7C3998/7C39B8 over the entire segment. No dispatcher/TLS scratch writes.
 * A moving object entering later remains the native avoider's responsibility.
 * 1=all currently eligible wrappers excluded,2=overlap,0=unqualified. */
fln_u32 lab_flow_lane_native_binding_dynamic(const FlowLaneNativeFrame *,
    const float a[3],const float b[3],float radius);
#endif
