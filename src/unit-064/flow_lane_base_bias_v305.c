#include "flow_lane_base_bias_v305.h"
typedef union {float f;flb_u32 u;} BFloat;
_Static_assert(sizeof(FlowLaneBaseInput)==76,"base bias input ABI");
_Static_assert(sizeof(FlowLaneBaseOutput)==32,"base bias output ABI");
static flb_u32 finite(float x){BFloat b={.f=x};return (b.u&0x7fffffffu)<0x7f800000u;}
static float absf(float x){return x<0?-x:x;}
static float root(float x){float z;__asm__ volatile("fsqrt %s0,%s1":"=w"(z):"w"(x));return z;}
static float target(float center,float offset,float side){
    float product;__asm__ volatile("fmul %s0,%s1,%s2":"=w"(product):"w"(offset),"w"(side));
    return center+product; /* preserve the native separate multiply/add rounding */
}
flb_u32 lab_flow_lane_base_proposal(const FlowLaneBaseInput *i,FlowLaneBaseOutput *out){
    if(!out)return 0;
    FlowLaneBaseOutput o={0};o.reason=FLB_INVALID;
    if(!i){*out=o;return 0;}
    o.offset=i->original_offset;
    if(!finite(i->speed)||!finite(i->original_offset)||!finite(i->base_half_width)){*out=o;return 0;}
    for(flb_u32 j=0;j<3;j++)if(!finite(i->native_center[j])||!finite(i->native_side[j])||
        !finite(i->base_center[j])||!finite(i->position[j])){*out=o;return 0;}
    if(i->reserved){*out=o;return 0;}
    for(flb_u32 j=0;j<3;j++)o.target[j]=target(i->native_center[j],i->original_offset,i->native_side[j]);
    if(i->current_key!=(39u<<16)||i->following_key!=(40u<<16)){o.reason=FLB_REGION;goto done;}
    if(i->drift_state){o.reason=FLB_DRIFT;goto done;}
    /* Actual captured approach retains >=2 forecast ticks through speed12;
     * speed16 falls below that margin. This refuses a lane proposal; it never
     * alters or caps the CPU's speed/AI.RATIO. The caller still runs its cost gate. */
    if(i->speed<6.f||i->speed>12.f){o.reason=FLB_SPEED;goto done;}
    const float anchor[3]={-2984.534423828125f,6103.86962890625f,46.56821060180664f};
    if(absf(i->base_half_width-131.09117126464844f)>.01f){o.reason=FLB_BASE;goto done;}
    for(flb_u32 j=0;j<3;j++)if(absf(i->base_center[j]-anchor[j])>.01f){o.reason=FLB_BASE;goto done;}
    float side2=0,base_dot=0,center2=0;
    for(flb_u32 j=0;j<3;j++){
        float d=i->base_center[j]-i->native_center[j];
        side2+=i->native_side[j]*i->native_side[j];base_dot+=d*i->native_side[j];center2+=d*d;
    }
    if(!finite(side2)||side2<.81f||side2>1.21f){o.reason=FLB_SIDE;goto done;}
    /* This narrow first candidate uses the observed native side orientation.
     * A reversed side is a different query case, never a fixed keep-right rule. */
    const float reference[3]={.969059243f,-.035756626f,-.244224584f};
    float alignment=0;
    for(flb_u32 j=0;j<3;j++)alignment+=i->native_side[j]*reference[j];
    if(alignment<=0||alignment*alignment<.98f*side2){o.reason=FLB_SIDE;goto done;}
    float width2=i->base_half_width*i->base_half_width;
    if(center2>.25f*width2){o.reason=FLB_CENTER;goto done;}
    float desired=base_dot/side2+.1f*i->base_half_width/root(side2);
    float shift=desired-i->original_offset,shift2=shift*shift*side2;
    if(!finite(desired)||shift2<1.f||shift2>.1225f*width2){o.reason=FLB_SHIFT;goto done;}
    float native2=0,new2=0,heading_dot=0,new_target[3],from_base2=0;
    for(flb_u32 j=0;j<3;j++){
        new_target[j]=target(i->native_center[j],desired,i->native_side[j]);
        float a=o.target[j]-i->position[j],b=new_target[j]-i->position[j];
        native2+=a*a;new2+=b*b;heading_dot+=a*b;
        float d=new_target[j]-i->base_center[j];from_base2+=d*d;
    }
    if(!finite(native2)||!finite(new2)||!finite(heading_dot)||!finite(from_base2)||
       native2<10000.f||new2<10000.f||heading_dot<=0||
       heading_dot*heading_dot<.98f*native2*new2||from_base2>.0256f*width2){o.reason=FLB_APPROACH;goto done;}
    o.proposed=1;o.reason=0;o.offset=desired;o.shift_squared=shift2;o.base_distance_squared=from_base2;
    for(flb_u32 j=0;j<3;j++)o.target[j]=new_target[j];
done:
    *out=o;return o.proposed;
}
