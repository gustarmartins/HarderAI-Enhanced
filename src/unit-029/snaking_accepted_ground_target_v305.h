/* Current accepted-query provenance for continuous grounded interpolation.
 * Keep exact arithmetic and every original history/corridor/environment gate.
 * The original target may have been replaced after the preceding dispatch. */
#include "native_accepted_ground_target_v305.h"
EXPORT const u32 lab_snaking_accepted_ground_target_config[4]={1,0x189020,0x182990,1};
static u32 ngs_accepted_ground_target(const void *physics,const SnakeHopFrame *frame,
                                     NativeAcceptedGroundTarget *out){
    if(!frame||frame->valid!=1||frame->reserved!=1||frame->physics!=(u64)physics||
       b(physics,0x200)!=1||b(physics,0x201)!=1||w(physics,0x26c)||b(physics,0x20a))return 0;
    u64 role=q(physics,0xf8);
    if(!ptr(role)||q((void *)role,0x28)!=(u64)physics)return 0;
    float displacement=f(physics,0x568),normal[3],target[3];
    if(!finite(displacement)||displacement<3.f||
       !hf_unit(physics,0x17c,normal,.999f,1.001f)||
       !hf_unit(physics,0x32c,target,.999f,1.001f))return 0;
    return lab_native_ground_target_accepted(physics,frame->clock,frame->start,frame->last,out);
}
static u32 nap_prediction(const void *physics,const GroundNativeAxis *r,
                          const NativeAcceptedGroundTarget *query,u32 ds,float out[3]){
    if(!r->ready||!r->predicted||!r->expected_valid||r->poison||r->contact!=3||r->air||
       (ds!=4&&ds!=5)||(r->ds!=ds&&!(r->ds==2&&r->landing==1&&r->frame.last-r->frame.start<=30))||r->expected_world||
       !finite(r->displacement)||r->displacement<3.f||
       nga_bits(r->coefficient)!=nga_bits(.1f)||query->tick<r->frame.last)return 0;
    union {u32 u;float f;} travel={.u=query->displacement_bits};
    if(!finite(travel.f)||travel.f<3.f)return 0;
    float original[3],target[3];
    if(!nga_interpolate(r->actual,r->target,r->coefficient,original))return 0;
    for(u32 i=0;i<3;i++){
        if(nga_bits(original[i])!=nga_bits(r->expected[i])||
           query->actual[i]!=nga_bits(r->actual[i])||query->target[i]!=w(physics,0x32c+4*i))return 0;
        target[i]=f(physics,0x32c+4*i);
    }
    if(!nga_interpolate(r->actual,target,.1f,out))return 0;
    for(u32 i=0;i<3;i++)if(nga_bits(out[i])!=w(physics,0x320+4*i))return 0;
    return 1;
}
