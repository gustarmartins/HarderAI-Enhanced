/* Private opt-in hop continuation. Original admission/corridor stays intact.
 * These helpers only store their own per-racer frame. Native geometry, motion,
 * charge, state and packet remain untouched. A route envelope is not KCL. */
EXPORT const struct {
    u32 abi,capacity,record_bytes,physical_state,max_elapsed,prediction_ticks;
    float plane_squared,alignment_squared,axis_norm_min,axis_norm_max;
} lab_snaking_hop_frame_config={2,12,88,2,30,4,.0025f,.95f,.999f,1.001f};

typedef struct {
    u64 driver,producer,clock,course,base_course,physics,drift;
    u32 start,last,valid,pad;
    float admission_up[3];u32 reserved;
} SnakeHopFrame;
_Static_assert(sizeof(SnakeHopFrame)==88,"Hop frame ABI2");
_Static_assert(__builtin_offsetof(SnakeHopFrame,admission_up)==72,"Hop admission plane offset");
EXPORT SnakeHopFrame lab_snaking_hop_frames[12];
static void nga_reset(const SnakeHopFrame *frame);
static void nga_qualify(const void *physics,const SnakeHopFrame *frame,const SnakeState *s,u32 ds);
static u32 nga_axis(const void *physics,const SnakeHopFrame *frame,const float *actual,const float *admission);
static u32 ngs_target_axis(const void *physics,const SnakeHopFrame *frame,const float *target,const float *admission);

typedef struct {float up[3],target_up[3],motion[3],pending[3];} HopVectors;
static u32 hrp_landed(const void *,const SnakeHopFrame *,HopVectors *);
static float hf_dot(const float *a,const float *z){return a[0]*z[0]+a[1]*z[1]+a[2]*z[2];}
static u32 hf_unit(const void *p,u32 offset,float *v,float low,float high){
    float n=0;
    for(u32 i=0;i<3;i++){v[i]=f(p,offset+4*i);if(!finite(v[i]))return 0;n+=v[i]*v[i];}
    if(!finite(n)||n<low||n>high)return 0;
    n=root(n);for(u32 i=0;i<3;i++)v[i]/=n;
    return 1;
}
static u32 hf_same_plane(const float *a,const float *z){
    float d=hf_dot(a,z);
    return finite(d)&&d>0&&d*d>=1.f-lab_snaking_hop_frame_config.plane_squared;
}
static u32 hf_motion_plane(const float *axis,const float *normal){
    float d=hf_dot(axis,normal);
    return finite(d)&&d*d<=lab_snaking_hop_frame_config.plane_squared;
}
static u32 hf_environment(const void *physics){
    float branch=f(physics,0x274);
    return !w(physics,0x5d4)&&!w(physics,0x5d8)&&f(physics,0x3c8)==0.f&&
           finite(branch)&&branch>=0.f&&branch<1.f&&
           b(physics,0x200)<=1&&b(physics,0x201)<=1;
}
static u32 hf_vectors(const void *physics,const SnakeHopFrame *frame,HopVectors *v){
    float chassis_up[3],chassis_heading[3],admission_up[3];
    if(!hf_environment(physics)||
       !hf_unit(frame,72,admission_up,.999f,1.001f)||
       !hf_unit(physics,0x14,chassis_up,.9f,1.1f)||
       !hf_unit(physics,0x20,chassis_heading,.9f,1.1f)||
       !hf_motion_plane(chassis_heading,chassis_up)||
       !hf_unit(physics,0x320,v->up,.999f,1.001f)||
       !hf_unit(physics,0x32c,v->target_up,.999f,1.001f)||
       !hf_unit(physics,0x2fc,v->motion,.9f,1.1f)||
       !hf_unit(physics,0x308,v->pending,.9f,1.1f)||
       !nga_axis(physics,frame,v->up,admission_up)||
       !ngs_target_axis(physics,frame,v->target_up,admission_up))return 0;
    return hf_motion_plane(v->motion,v->up)&&hf_motion_plane(v->motion,v->target_up)&&
           hf_motion_plane(v->pending,v->up)&&hf_motion_plane(v->pending,v->target_up);
}
/* Existing owned lock serializes these private records. Missing or duplicate
 * updates revoke the supplement; they never cancel a legacy-valid maneuver. */
static void hf_touch(SnakeHopFrame *frame,const SnakeState *s,void *driver,
                     void *physics,void *drift,u32 now,u32 dt,u32 ds){
    /* Cheap retirement precedes native-intent gates. No helper call or course
     * walk belongs here; vetoed input must keep the original early exit. */
    if(frame->valid&&(ds!=2||dt!=1||frame->last+1!=now||s->phase!=1||
       frame->driver!=s->driver||frame->producer!=s->producer||
       frame->clock!=s->clock||frame->start!=s->start||
       frame->physics!=(u64)physics||frame->drift!=(u64)drift||
       frame->course!=q(driver,0x80)))frame->valid=0;
    frame->last=now;
}
static void hf_remember(SnakeHopFrame *frame,const SnakeState *s,
                        void *driver,void *physics,void *drift,void *route,u32 now){
    nga_reset(frame);
    frame->valid=0;
    if(s->phase!=1||s->start!=now||!b(physics,0x201)||
       !hf_unit(physics,0x14,frame->admission_up,.9f,1.1f))return;
    HopVectors vectors;if(!hf_vectors(physics,frame,&vectors))return;
    u64 base=be_course((u64)lab_module_start-LAB_MAIN_TO_MODULE,driver,route);
    u64 course=q(driver,0x80);
    if(!base||!ptr(course))return;
    frame->driver=s->driver;frame->producer=s->producer;frame->clock=s->clock;
    frame->start=now;frame->last=now;frame->course=course;frame->base_course=base;
    frame->physics=(u64)physics;frame->drift=(u64)drift;
    frame->pad=0;frame->reserved=0;frame->valid=1;
}
static u32 hf_scope(const SnakeHopFrame *frame,const SnakeState *s,
                    const void *physics,const void *drift,u32 now,u32 ds){
    return frame->valid==1&&s->phase==1&&ds==2&&now>s->start&&
           now-s->start<=30&&frame->last==now&&s->last==now&&
           frame->driver==s->driver&&frame->producer==s->producer&&
           frame->clock==s->clock&&frame->start==s->start&&
           frame->physics==(u64)physics&&frame->drift==(u64)drift&&
           q(physics,0x110)==(u64)drift;
}
/* Run only after the original native-intent, phase and deadline gates.
 * Observe the qualified base before the ORIGINAL active corridor, including
 * its successful ticks. A -> B -> A cannot revive an observed invalidation. */
static void hf_qualify(SnakeHopFrame *frame,const SnakeState *s,void *driver,
                       void *physics,void *drift,void *route,u32 now,u32 ds){
    if(!hf_scope(frame,s,physics,drift,now,ds))return;
    u64 current_base=be_course((u64)lab_module_start-LAB_MAIN_TO_MODULE,driver,route);
    if(!current_base||current_base!=frame->base_course||
       q(driver,0x80)!=frame->course||q(physics,0x110)!=(u64)drift)frame->valid=0;
}
static u32 hf_intent(const void *physics,const void *a,const void *z,
                    const HopVectors *v,u32 occupied,
                    float *delta,float *length2,float *projection){
    if(!be_intent(physics,a,z,v->up,1.f,.0025f,occupied,delta,length2,projection))return 0;
    float normal=hf_dot(delta,v->target_up);
    return finite(normal)&&normal*normal<=.0025f*(*length2);
}
/* Both actual and pending travel must qualify every selected AND base edge.
 * Chassis yaw alone is never substituted with an arbitrary passing vector. */
static u32 hf_edge(const float *delta,float length2,const HopVectors *v){
    if(!finite(length2)||length2<64.f)return 0;
    float a=hf_dot(delta,v->motion),z=hf_dot(delta,v->pending);
    float n=hf_dot(delta,v->up),t=hf_dot(delta,v->target_up);
    return finite(a)&&finite(z)&&finite(n)&&finite(t)&&a>0&&z>0&&
           a*a>=lab_snaking_config.alignment_squared*length2&&
           z*z>=lab_snaking_config.alignment_squared*length2&&
           n*n<=.0025f*length2&&t*t<=.0025f*length2;
}
/* Four-update local prediction stays inside the ACTUALLY occupied base edge.
 * The original separate full-horizon route walk remains mandatory. Prediction
 * never clips position/projection at a node or invents selected-view width. */
EXPORT const u32 lab_snaking_projection_operands_config[4]={1,24,20,16};
static u32 hpo_refused(u32 *detail,u32 stage,u32 axis,u32 endpoint){
    *detail=stage|(axis<<4)|(endpoint<<5);return 0;
}

static u32 hf_projection(const void *physics,const void *a,const float *delta,
                         float length2,float width,float speed,const HopVectors *v,u32 *__restrict detail,float *__restrict observed){
    *detail=0;for(u32 i=0;i<4;i++)observed[i]=0.f;
    float offset[3],projection=0;
    for(u32 i=0;i<3;i++){
        offset[i]=f(physics,0x2c+4*i)-f(a,0xc+4*i);
        observed[i]=offset[i];
        if(!finite(offset[i]))return hpo_refused(detail,1,0,0);projection+=offset[i]*delta[i];
    }
    float clearance=width-lab_snaking_config.margin-2.f*speed;
    float travel=4.f*speed;
    if(!finite(projection)||projection<0||projection>length2||clearance<=0)return hpo_refused(detail,2,0,0);
    for(u32 mode=0;mode<2;mode++){
        const float *axis=mode?v->pending:v->motion;
        float advance=hf_dot(axis,delta),end=projection+travel*advance;
        if(!finite(end)||advance<=0||end>=length2)return hpo_refused(detail,3,mode,0);
        for(u32 endpoint=0;endpoint<2;endpoint++){
            float along=(endpoint?end:projection)/length2,error=0;
            for(u32 i=0;i<3;i++){
                float e=offset[i]+(endpoint?travel*axis[i]:0.f)-along*delta[i];error+=e*e;
            }
            observed[3]=error;
            if(!finite(error)||error>=clearance*clearance)return hpo_refused(detail,4,mode,endpoint);
        }
    }
    return 1;
}
