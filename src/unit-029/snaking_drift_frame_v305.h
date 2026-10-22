/* Isolated explicit-drift continuation experiment. Own frame writes only.
 * Requires continuous qualified admission -> hop -> matching grounded drift.
 * Original successful corridor and hop supplement remain first in priority. */
EXPORT const struct {
    u32 abi,capacity,record_bytes,state_mask,prediction_ticks;
    float chassis_motion_alignment_squared;
} lab_snaking_drift_frame_config={1,12,88,48,4,.5f};
EXPORT SnakeHopFrame lab_snaking_drift_frames[12];

static void df_remember(SnakeHopFrame *to,const SnakeHopFrame *from){
    to->valid=0;
    if(from->valid!=1)return;
    *to=*from;to->pad=0;to->reserved=0;
}

/* Preserve the native R input opportunity over a short contact interruption.
 * This does not qualify a road: the complete existing route and vector checks
 * still run for every update. Do not grant new admissions while airborne. */
EXPORT const u32 lab_snaking_short_contact_config[5]={1,3,1,0,0};
static u32 df_contact(const void *physics,const SnakeHopFrame *frame){
    if(b(physics,0x200)!=1)return 0;
    if(b(physics,0x201)==1)return 1;
    u32 air=w(physics,0x26c);
    return b(physics,0x201)==0&&frame->reserved==1&&air>=1&&air<=3&&
           f(physics,0x274)==.05f*(float)air&&!w(physics,0x5d4)&&
           !w(physics,0x5d8)&&f(physics,0x3c8)==0.f;
}

static void df_touch(SnakeHopFrame *frame,const SnakeState *s,void *driver,
                     void *physics,void *drift,u32 now,u32 dt,u32 ds){
    if(frame->valid){
        u32 matching=s->direction<0?4u:5u;
        if(dt!=1||frame->last+1!=now||s->phase!=1||
           frame->driver!=s->driver||frame->producer!=s->producer||
           frame->clock!=s->clock||frame->start!=s->start||
           frame->physics!=(u64)physics||frame->drift!=(u64)drift||
           frame->course!=q(driver,0x80)||
           (ds!=2&&ds!=matching)||
           (ds==2&&frame->pad!=0&&frame->pad!=2)||
           (ds==matching&&(!frame->reserved||!df_contact(physics,frame))))
            frame->valid=0;
        else{
            if(ds==2)frame->reserved=1;
            frame->pad=ds;
        }
    }
    frame->last=now;
}

static u32 df_scope(const SnakeHopFrame *frame,const SnakeState *s,
                    const void *physics,const void *drift,u32 now,u32 ds){
    return frame->valid==1&&frame->reserved==1&&frame->pad==ds&&
           s->phase==1&&(ds==4||ds==5)&&ds==(s->direction<0?4u:5u)&&
           now>s->start&&now-s->start<lab_snaking_config.leg_deadline&&
           frame->last==now&&s->last==now&&
           frame->driver==s->driver&&frame->producer==s->producer&&
           frame->clock==s->clock&&frame->start==s->start&&
           frame->physics==(u64)physics&&frame->drift==(u64)drift&&
           q(physics,0x110)==(u64)drift&&df_contact(physics,frame);
}

static void df_qualify(SnakeHopFrame *frame,const SnakeState *s,void *driver,
                       void *physics,void *drift,void *route,u32 now,u32 ds){
    (void)s;(void)now;(void)ds;
    if(!frame->valid)return;
    u64 current_base=be_course((u64)lab_module_start-LAB_MAIN_TO_MODULE,driver,route);
    if(!current_base||current_base!=frame->base_course||
       q(driver,0x80)!=frame->course||q(physics,0x110)!=(u64)drift)frame->valid=0;
    nga_qualify(physics,frame,s,ds);
}

/* The body-up vector may pass its own motion tests while still tilting
 * the occupied road beyond the route plane limit. Prefer the existing
 * collision-qualified landing surface; all history, contact, motion and
 * route guards stay intact. No native axis or geometry is written. */
EXPORT const u32 lab_snaking_landed_surface_priority_config[4]={1,4,5,0};
static u32 hcs_landed(const void *physics,const SnakeHopFrame *frame,HopVectors *v);
static u32 df_vectors(const void *physics,const SnakeHopFrame *frame,HopVectors *v){
    if(!df_contact(physics,frame)||!(hcs_landed(physics,frame,v)||hf_vectors(physics,frame,v)||hrp_landed(physics,frame,v)))return 0;
    float heading[3];if(!hf_unit(physics,0x20,heading,.9f,1.1f))return 0;
    float actual=hf_dot(heading,v->motion),pending=hf_dot(heading,v->pending);
    /* The travel envelope replaces the chassis-forward route test only.
     * Refuse a spin/sideways body even when supplied travel stays forward. */
    float limit=lab_snaking_drift_frame_config.chassis_motion_alignment_squared;
    return actual>0&&pending>0&&actual*actual>=limit&&pending*pending>=limit;
}

#include "snaking_native_ground_axis_proposal_v305.h"
