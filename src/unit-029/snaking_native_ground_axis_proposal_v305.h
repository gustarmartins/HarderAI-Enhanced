/* PRIVATE OFFLINE PROPOSAL v3. Owned record only; never write native axes.
 * Follow exact 182990 interpolation from the preceding qualified sample.
 * Collision publishes target/contact after normal production: a landing
 * sample can therefore retain the preceding airborne normal for one update.
 * All original actual/pending tangency, route and width forecasts remain. */
typedef struct {
    SnakeHopFrame frame;
    float actual[3],target[3],road[3],expected[3];
    u32 air,mode,flags,selector,contact,ds,ready,predicted,expected_valid,poison,landing,expected_world;
    float displacement,coefficient;
} GroundNativeAxis;
_Static_assert(sizeof(GroundNativeAxis)==192,"Native ground axis proposal ABI3");
EXPORT const u32 lab_snaking_ground_axis_proposal_config[4]={3,12,192,0};
EXPORT GroundNativeAxis lab_snaking_ground_axis_proposal_records[12];
#include "snaking_axis_history_diag_v305.h"
#include "snaking_normal_flag_scope_v305.h"

static u32 nga_bits(float x){union{float f;u32 u;}v={.f=x};return v.u;}
static GroundNativeAxis *nga_record(const SnakeHopFrame *frame){
    u64 p=(u64)frame,h=(u64)lab_snaking_hop_frames,d=(u64)lab_snaking_drift_frames;
    u64 offset=p-h;
    if(offset>=sizeof(lab_snaking_hop_frames))offset=p-d;
    if(offset>=sizeof(lab_snaking_hop_frames)||offset%88)return 0;
    return &lab_snaking_ground_axis_proposal_records[offset/88];
}
static void nga_reset(const SnakeHopFrame *frame){
    GroundNativeAxis *r=nga_record(frame);if(r){r->ready=0;r->poison=0;r->expected_valid=0;}
}
static u32 nga_identity(const GroundNativeAxis *r,const SnakeHopFrame *frame){
    if(r->frame.driver!=frame->driver||r->frame.producer!=frame->producer||
       r->frame.clock!=frame->clock||r->frame.course!=frame->course||
       r->frame.base_course!=frame->base_course||r->frame.physics!=frame->physics||
       r->frame.drift!=frame->drift||r->frame.start!=frame->start)return 0;
    for(u32 i=0;i<3;i++)if(nga_bits(r->frame.admission_up[i])!=nga_bits(frame->admission_up[i]))return 0;
    return 1;
}
static float nga_ground_factor(float x){
    if(x<=.2f)return .02f;
    if(x>=2.f)return .1f;
    volatile float shifted=x+(-.2f),product=shifted*.08f,scaled=product/1.8f;
    return scaled+.02f;
}
static u32 nga_interpolate(const float *actual,const float *target,float factor,float *next){
    for(u32 i=0;i<3;i++){
        volatile float delta=target[i]-actual[i],product=factor*delta;
        next[i]=actual[i]+product;
    }
    volatile float x2=next[0]*next[0],y2=next[1]*next[1],z2=next[2]*next[2],xy=x2+y2;
    float size=root(xy+z2);if(!finite(size)||size<=0.f)return 0;
    float inverse=1.f/size;for(u32 i=0;i<3;i++)next[i]=next[i]*inverse;
    return 1;
}
static u32 nga_target_equals(const void *physics,const float *target){
    for(u32 i=0;i<3;i++)if(w(physics,0x32c+4*i)!=nga_bits(target[i]))return 0;
    return 1;
}
#include "snaking_local_gravity_v305.h"
static u32 nga_world(const void *physics,u32 mode,u32 selector,u32 flags,float *target){
    if(selector==0){
        if(mode!=2&&(flags&0x1000u))return 0;
        target[0]=0.f;target[1]=1.f;target[2]=0.f;return 1;
    }
    return lsg_target(physics,target);
}
/* Private supplied-contact trial. Native18AAA4 ordinary branch publishes the
 * target; native189020..18905C copies that query normal into17C. Admit only
 * that exact matching result, with full contact and ordinary direct output.
 * No normal, contact, position or other native field is written here. */
EXPORT const u32 lab_snaking_ground_surface_config[4]={1,3,0x17c,0x32c};
#include "snaking_native_collision_target_v305.h"
static u32 ngs_native_ground_target(const void *physics){
    if(b(physics,0x200)!=1||b(physics,0x201)!=1||w(physics,0x26c)||b(physics,0x20a))return 0;
    u64 role=q(physics,0xf8);
    if(!ptr(role)||q((void *)role,0x28)!=(u64)physics)return 0;
    float displacement=f(physics,0x568);
    if(!finite(displacement)||displacement<0.f||(b((void *)role,0xd8)&&displacement<3.f))return 0;
    float normal[3];
    if(!hf_unit(physics,0x17c,normal,.999f,1.001f))return 0;
    float expected[3];if(!nct_expected(physics,role,expected))return 0;
    for(u32 i=0;i<3;i++)if(nga_bits(expected[i])!=w(physics,0x32c+4*i))return 0;
    return 1;
}

static u32 ngd_native_ground_target(const void *physics,NgdFailure *failure){
    if(ngd_test(b(physics,0x200)!=1,failure,81)||ngd_test(b(physics,0x201)!=1,failure,82)||ngd_test(w(physics,0x26c),failure,83)||ngd_test(b(physics,0x20a),failure,84))return 0;
    u64 role=q(physics,0xf8);
    if(ngd_test(!ptr(role),failure,85)||ngd_test(q((void *)role,0x28)!=(u64)physics,failure,86))return 0;
    float displacement=f(physics,0x568);
    if(ngd_test(!finite(displacement),failure,87)||ngd_test(displacement<0.f,failure,88)||ngd_test((b((void *)role,0xd8)&&displacement<3.f),failure,89))return 0;
    float normal[3];
    if(ngd_test(!hf_unit(physics,0x17c,normal,.999f,1.001f),failure,90))return 0;
    float expected[3];if(!nct_expected(physics,role,expected)){ngd_test(1,failure,90);return 0;}
    for(u32 i=0;i<3;i++)if(!ngd_equal(nga_bits(expected[i]),w(physics,0x32c+4*i),failure,91,i))return 0;
    return 1;
}

#include "snaking_retained_ground_target_v305.h"
#include "snaking_landing_prediction_v305.h"
#include "snaking_accepted_ground_target_v305.h"
#include "snaking_air_target_history_v305.h"
static void nga_qualify(const void *physics,const SnakeHopFrame *frame,const SnakeState *s,u32 ds){
    GroundNativeAxis *r=nga_record(frame);if(!r)return;
    NgdFailure failure={0};
    u32 ready=0,air=w(physics,0x26c),contact=b(physics,0x200)|(b(physics,0x201)<<1);
    u32 elapsed=frame->last-frame->start,mode=w(physics,0x230),selector=b(physics,0x27b);
    u64 role=q(physics,0xf8);
    /* Admission can be observed again at its own tick by the fixture/caller;
     * it carries no producer step to predict yet. Preserve only the empty
     * fresh record; duplicate updates after seeding remain strict below. */
    if(!elapsed&&!r->ready&&!r->poison)return;
    if(ngd_test(r->poison,&failure,1)||ngd_test(frame->valid!=1,&failure,2)||ngd_test(frame->reserved!=1,&failure,3)||ngd_test(s->phase!=1,&failure,4)||
       ngd_test(frame->last!=s->last,&failure,5)||ngd_test(frame->start!=s->start,&failure,6)||ngd_test(frame->physics!=(u64)physics,&failure,7)||
       ngd_test(!ptr(frame->drift),&failure,8)||ngd_test(q(physics,0x110)!=frame->drift,&failure,9)||ngd_test(w((void *)frame->drift,0x10)!=ds,&failure,10)||
       ngd_test((ds!=2&&ds!=(s->direction<0?4u:5u)),&failure,11)||ngd_test((ds==2&&elapsed>30),&failure,12)||
       ngd_test(!elapsed,&failure,13)||ngd_test(elapsed>=lab_snaking_config.leg_deadline,&failure,14)||ngd_test(air>19,&failure,15)||ngd_test(air>elapsed,&failure,16)||
       ngd_test(!hf_environment(physics),&failure,17)||ngd_test(f(physics,0x274)!=.05f*(float)air,&failure,18)||
       ngd_test((contact!=0&&contact!=1&&contact!=3),&failure,19)||ngd_test((contact!=3&&!air),&failure,42)||ngd_test((contact==3&&air),&failure,20)||ngd_test((ds!=2&&contact!=3),&failure,21)||
       ngd_test(!ptr(role),&failure,22)||ngd_test(q((void *)role,0x28)!=(u64)physics,&failure,23)||ngd_test(mode>2,&failure,24)||ngd_test(selector>1,&failure,25))goto invalid;
    u32 flags=w((void *)role,0x1cc);
    if(ngd_test(flags&0x00200010u,&failure,26))goto invalid;
    float actual[3],admission[3],target[3],next_target[3];
    if(ngd_test(!hf_unit(physics,0x320,actual,.999f,1.001f),&failure,27)||
       ngd_test(!hf_unit(frame,72,admission,.999f,1.001f),&failure,28)||
       ngd_test(!hf_unit(physics,0x32c,target,.999f,1.001f),&failure,29))goto invalid;
    u32 retained=contact==3&&ngs_retained_ground_target(physics,frame);
    NativeAcceptedGroundTarget accepted_query;
    u32 accepted=contact==3&&ngs_accepted_ground_target(physics,frame,&accepted_query);
    u32 predicted=0,landing=0;
    if(!r->ready){
        if(ngd_test(elapsed!=1,&failure,30)||ngd_test(!hf_same_plane(actual,admission),&failure,31)||ngd_test(!hf_same_plane(target,admission),&failure,32))goto invalid;
        for(u32 i=0;i<3;i++)r->road[i]=f(physics,0x32c+4*i);
    }else{
        if(ngd_test(!r->expected_valid,&failure,33)||ngd_test(!nga_identity(r,frame),&failure,34)||ngd_test(r->frame.last==0xffffffffu,&failure,35)||
           ngd_test(r->frame.last+1!=frame->last,&failure,36)||ngd_test(r->mode!=mode,&failure,37)||ngd_test(r->selector!=selector,&failure,38)||nfs_changed(r->flags,flags,&failure))goto invalid;
        float landed[3];
        u32 landed_exact=nlp_landed_prediction(physics,r,contact,landed,retained);
        float accepted_normal[3];
        u32 accepted_exact=accepted&&nap_prediction(physics,r,&accepted_query,ds,accepted_normal);
        for(u32 i=0;i<3;i++){
            if(!ngd_equal(nga_bits(accepted_exact?accepted_normal[i]:landed_exact?landed[i]:r->expected[i]),w(physics,0x320+4*i),&failure,70,i))goto invalid;
            if(r->contact==3&&!ngd_equal(nga_bits(r->road[i]),nga_bits(r->target[i]),&failure,71,i))goto invalid;
        }
        if(contact==3&&(r->contact==0||r->contact==1)){
            if(ngd_test(r->ds!=2,&failure,40)||ngd_test(r->landing,&failure,41)||(!retained&&!accepted&&!ngd_native_ground_target(physics,&failure)))goto invalid;
            landing=1;
        }else if(contact==r->contact||((contact==0||contact==1)&&(r->contact==0||r->contact==1||r->contact==3))){
            if(ngd_test((contact!=3&&(r->air+1!=air||r->landing||r->ds!=2)),&failure,42)||ngd_test((contact==3&&r->air),&failure,43))goto invalid;
            if(contact==3){
                if(!retained&&!accepted&&!ngd_native_ground_target(physics,&failure))goto invalid;
            }else if(r->expected_world){
                if(ngd_test(!nga_world(physics,mode,selector,flags,next_target),&failure,44)||ngd_test(!ath_prior_target(physics,r),&failure,45))goto invalid;
            }else if(ngd_test(!nga_target_equals(physics,r->road),&failure,46))goto invalid;
            landing=r->landing;
        }else {ngd_test(1,&failure,72);goto invalid;}
        predicted=1;
    }
    if(contact==3){
        if(!retained&&!accepted&&!ngd_native_ground_target(physics,&failure))goto invalid;
        for(u32 i=0;i<3;i++)r->road[i]=f(physics,0x32c+4*i);
    }
    for(u32 i=0;i<3;i++)next_target[i]=f(physics,0x32c+4*i);
    float displacement=f(physics,0x568);
    if(ngd_test(!finite(displacement),&failure,47)||ngd_test(displacement<0.f,&failure,48))goto invalid;
    float coefficient=nga_ground_factor(displacement);u32 world=0;
    /* Predict the next producer invocation. Contact201 determines whether
     * native1848A0 increments air or clears it before182990 selects its blend.
     * Current admission already bounds air<=19 and ground contact to air0. */
    if(!(contact&2u)&&air>=10){
        if(ngd_test(contact&2u,&failure,49)||ngd_test(!nga_world(physics,mode,selector,flags,next_target),&failure,44))goto invalid;
        coefficient=selector==0&&mode==2?.02f:.04f;world=1;
    }
    r->ready=0;r->expected_valid=0;
    /* Preserve raw producer operands. Unit validation above never replaces
     * native raw float bits in the exact interpolation equation. */
    for(u32 i=0;i<3;i++){
        r->actual[i]=f(physics,0x320+4*i);r->target[i]=f(physics,0x32c+4*i);
    }
    if(ngd_test(!nga_interpolate(r->actual,next_target,coefficient,r->expected),&failure,50))goto invalid;
    r->frame=*frame;r->air=air;r->mode=mode;r->flags=flags;r->selector=selector;
    r->contact=contact;r->ds=ds;r->predicted=predicted;r->landing=landing;
    r->expected_world=world;r->displacement=displacement;r->coefficient=coefficient;
    if(!ath_publish(r,next_target))goto invalid;
    r->expected_valid=1;ready=1;
invalid:
    if(!ready)ngd_publish(frame,r,&failure,air,contact,mode,selector,ds,elapsed);
    r->ready=ready;if(!ready){r->poison=1;r->expected_valid=0;}
}
static u32 ngs_owned_history(const void *physics,const SnakeHopFrame *frame){
    GroundNativeAxis *r=nga_record(frame);
    if(!r||!r->ready||!r->predicted||r->poison||frame->valid!=1||!nga_identity(r,frame)||
       r->frame.last!=frame->last||r->air!=w(physics,0x26c)||r->mode!=w(physics,0x230)||
       r->selector!=b(physics,0x27b)||r->contact!=(u32)(b(physics,0x200)|(b(physics,0x201)<<1))||
       r->ds!=w((void *)frame->drift,0x10))return 0;
    u64 role=q(physics,0xf8);
    if(!ptr(role)||q((void *)role,0x28)!=(u64)physics||((r->flags^w((void *)role,0x1cc))&lab_snaking_normal_flag_scope_config[1]))return 0;
    for(u32 i=0;i<3;i++)if(nga_bits(r->actual[i])!=w(physics,0x320+4*i)||
                            nga_bits(r->target[i])!=w(physics,0x32c+4*i))return 0;
    return 1;
}

#include "snaking_takeoff_history_v305.h"
static u32 nga_axis(const void *physics,const SnakeHopFrame *frame,const float *actual,const float *admission){
    return hf_same_plane(actual,admission)||ngs_owned_history(physics,frame);
}
static u32 ngs_target_axis(const void *physics,const SnakeHopFrame *frame,const float *target,const float *admission){
    if(hf_same_plane(target,admission))return 1;
    NativeAcceptedGroundTarget accepted_query;
    return ngs_owned_history(physics,frame)&&(ngs_native_ground_target(physics)||ngs_retained_ground_target(physics,frame)||
        ngs_accepted_ground_target(physics,frame,&accepted_query));
}

#include "snaking_air_boundary_v305.h"
