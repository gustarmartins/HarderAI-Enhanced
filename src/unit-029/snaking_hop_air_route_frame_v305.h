/* Isolated owned-hop supplement, native qualified route-plane reference v3. The target at +32c may be native world-up
 * after eleven airborne updates (182a28/182c3c). It is an orientation target,
 * not necessarily the occupied route plane. Never store any native field.
 * Retain the qualified admission plane AND actual rotation/actual/pending
 * travel guards; every original paired route/width/forecast guard still runs.
 */
EXPORT const struct {u32 abi,physical_state,min_air_ticks,max_air_ticks;float plane_squared;}
lab_snaking_hop_air_route_frame_config={3,2,11,19,.0025f};

#include "snaking_hop_road_projection_v305.h"

static u32 har_vectors(const void *physics,const SnakeHopFrame *frame,HopVectors *v){
    if(hf_vectors(physics,frame,v))return 1;
    if(hrp_landed(physics,frame,v))return 1;
    /* Caller hf_scope also checks continuous ownership/tick, phase, current
     * course/base identity and lifetime. No exception survives contact or ds4/5. */
    if(frame->valid!=1||frame->physics!=(u64)physics||
       q(physics,0x110)!=frame->drift||!ptr(frame->drift)||
       w((void *)frame->drift,0x10)!=2||!nth_air_contact(physics,frame)||
       !hf_environment(physics))return 0;
    u32 air=w(physics,0x26c),elapsed=frame->last-frame->start;
    if(air<11||air>19||elapsed>30||air>elapsed||b(physics,0x27b)>1||
       f(physics,0x274)!=.05f*(float)air)return 0;
    u64 role=q(physics,0xf8);if(!ptr(role)||q((void *)role,0x28)!=(u64)physics)return 0;
    u32 flags=w((void *)role,0x1cc),mode=w(physics,0x230);
    /* Bit21 selects another gravity branch; bit4 changes the normal path.
     * Ordinary native world-up reset: material2, or no bit12 hold. The outer
     * native-intent gate retains all its other special-state exclusions. */
    if((flags&0x00200010u)||mode>2)return 0;
    if(b(physics,0x27b)==0){
        if((mode!=2&&(flags&0x1000u))||w(physics,0x32c)||
           w(physics,0x330)!=0x3f800000u||w(physics,0x334))return 0;
    }else{
        /* Native selector1 negates gravity27c into target32c after air>=11.
         * Only exact world-down gravity is supported here. Keep signed-zero
         * correspondence, so this cannot admit another gravity direction or
         * an independently mismatching world-up target. Bit12 does not gate
         * this native selector1 branch (retail182a8c..182aac).
         */
        if(!lsg_hop_target(physics,frame))return 0;
    }
    float chassis_up[3],heading[3];
    if(!hf_unit(frame,72,v->target_up,.999f,1.001f)||
       !hf_unit(physics,0x14,chassis_up,.9f,1.1f)||
       !hf_unit(physics,0x20,heading,.9f,1.1f)||!hf_motion_plane(heading,chassis_up)||
       !hf_unit(physics,0x320,v->up,.999f,1.001f)||
       !hf_unit(physics,0x2fc,v->motion,.9f,1.1f)||
       !hf_unit(physics,0x308,v->pending,.9f,1.1f)||
       !nga_axis(physics,frame,v->up,v->target_up))return 0;
    if(!hf_motion_plane(v->motion,v->up)||!hf_motion_plane(v->motion,v->target_up)||
       !hf_motion_plane(v->pending,v->up)||!hf_motion_plane(v->pending,v->target_up)){
        if(!hrp_vectors(physics,frame,v))return 0;
    }
    /* Native actual320 rotates toward the airborne target. It remains a body
     * coherence/tangency check above; the future road envelope uses the
     * retained ground-admission plane. Actual/pending travel vectors still
     * drive both alignment and four-update 3D width forecasts unchanged.
     * This is limited to the branch-qualified airborne exception. */
    for(u32 i=0;i<3;i++)v->up[i]=v->target_up[i];
    return 1;
}
