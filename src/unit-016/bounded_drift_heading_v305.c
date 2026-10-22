/* Opt-in prototype: correct an existing owned packet10 after native exits.
 * Primary input, R, countersteer, native state/lifetime and charge stay native.
 * Geometry is a short path-envelope prediction, never a KCL safety proof.
 */
typedef unsigned char u8;
typedef unsigned int u32;
typedef signed int i32;
typedef unsigned long long u64;
#define EXPORT __attribute__((visibility("default")))
#include "bounded_drift_heading_observation_v305.h"
extern char lab_module_start[];
extern const u8 lab_snaking_state[]; /* Existing88-byte records, borrowed only. */
EXPORT const struct {
    u32 abi,scope,horizon;
    float gain,lookahead,selected_max,packet_max,delta_max,margin,clearance_fraction,max_speed;
} lab_bounded_drift_heading_config={1,1+2*LAB_DRIFT_HEADING_SNAKING,4,
    .055f,100.f,.015f,.03f,.01f,24.f,.05f,40.f};

static u64 q(const void *p,u32 o){return *(const u64 *)((const u8 *)p+o);}
static u32 w(const void *p,u32 o){return *(const u32 *)((const u8 *)p+o);}
static u8 b(const void *p,u32 o){return *((const u8 *)p+o);}
static float f(const void *p,u32 o){return *(const float *)((const u8 *)p+o);}
static u32 ptr(u64 p){return p>=0x10000&&!(p&7)&&p<(1ull<<39)-0x2000;}
static u32 finite(float v){union {float f;u32 u;}x={.f=v};return (x.u&0x7fffffffu)<0x7f800000u;}
static float absf(float v){return v<0?-v:v;}
static float root(float v){float r;__asm__("fsqrt %s0,%s1":"=w"(r):"w"(v));return r;}
static float clamp(float v,float limit){return v>limit?limit:(v< -limit?-limit:v);}
static float dot(const float *a,const float *z){return a[0]*z[0]+a[1]*z[1]+a[2]*z[2];}
#include "native_base_envelope_v305.inc"

/* Public scalar boundary for exact-artifact sweeps. clear_slack is the space
 * LEFT after both native movement projections and the original24-unit margin.
 * The5% allocation and small-angle displacement budget are authored policy. */
EXPORT float lab_bounded_drift_heading_value(float original,float error,float correction,
                                            float clear_slack,float speed){
    if(!finite(original)||!finite(error)||!finite(correction)||!finite(clear_slack)||
       !finite(speed)||speed<=0||speed>lab_bounded_drift_heading_config.max_speed||
       clear_slack<=0||absf(original)>lab_bounded_drift_heading_config.packet_max||
       absf(correction)>.08f)return original;
    float h=(float)lab_bounded_drift_heading_config.horizon;
    float delta=lab_bounded_drift_heading_config.clearance_fraction*clear_slack/
                (speed*h*(h+1.f)*.5f);
    if(delta>lab_bounded_drift_heading_config.delta_max)delta=lab_bounded_drift_heading_config.delta_max;
    float desired=clamp(error*lab_bounded_drift_heading_config.gain,
                        lab_bounded_drift_heading_config.selected_max);
    float candidate=clamp(desired-correction,lab_bounded_drift_heading_config.packet_max);
    candidate=original+clamp(candidate-original,delta);
    if(!finite(candidate)||absf(desired-(candidate+correction))>=absf(desired-(original+correction)))return original;
    return candidate;
}

#include "snaking_heading_compensation_v305.h"

static u32 unit(const void *p,u32 offset,float *out){
    float n=0;
    for(u32 i=0;i<3;i++){out[i]=f(p,offset+4*i);if(!finite(out[i]))return 0;n+=out[i]*out[i];}
    if(!finite(n)||n<.9f||n>1.1f)return 0;
    n=root(n);for(u32 i=0;i<3;i++)out[i]/=n;return 1;
}

/* 183194 passes physics320, not physics14, to the native rotation consumer.
 * Refuse an incoherent or nonunit actual rotation frame. Normalize only this
 * local geometric copy; never repair or overwrite the native axis. */
static u32 rotation_frame(const void *physics,float *up){
    float normal[3],n=0;
    if(!unit(physics,0x14,normal))return 0;
    for(u32 i=0;i<3;i++){
        up[i]=f(physics,0x320+4*i);if(!finite(up[i]))return 0;n+=up[i]*up[i];
    }
    if(!finite(n)||n<.999f||n>1.001f)return 0;
    n=root(n);for(u32 i=0;i<3;i++)up[i]/=n;
    return dot(normal,up)>=.9995f;
}

static u32 span(const void *physics,u64 a,u64 z,const float *up,
                float *delta,float *offset,float *length2,float *projection){
    float n=0,p=0;
    for(u32 i=0;i<3;i++){
        float start=f((void *)a,0xc+4*i),end=f((void *)z,0xc+4*i),position=f(physics,0x2c+4*i);
        if(!finite(start)||!finite(end)||!finite(position))return 0;
        delta[i]=end-start;offset[i]=position-start;n+=delta[i]*delta[i];p+=offset[i]*delta[i];
    }
    float vertical=dot(delta,up);
    if(!finite(n)||n<64||!finite(p)||vertical*vertical>.0025f*n)return 0;
    *length2=n;*projection=p;return 1;
}

static u32 envelope(const void *physics,u64 a,u64 z,const float *up,
                    const float *motion,const float *pending,float speed,float *slack,u32 owned_grounded){
    float d[3],o[3],n,p,radius=10000.f;
    if(!span(physics,a,z,up,d,o,&n,&p)||p<0||p>n)return 0;
    for(u32 i=0;i<3;i++){
        float left=f((void *)a,0x24+4*i),right=f((void *)z,0x24+4*i);
        if(!finite(left)||!finite(right)||left<=0||right<=0||left>10000||right>10000)return 0;
        if(left<radius)radius=left;if(right<radius)radius=right;
    }
    float length=root(n),clearance=radius-lab_bounded_drift_heading_config.margin;
    float travel=speed*(float)lab_bounded_drift_heading_config.horizon;
    if(clearance<=0)return 0;
    *slack=clearance;
    for(u32 mode=0;mode<2;mode++){
        const float *axis=mode?pending:motion;float advance=dot(axis,d),vertical=dot(axis,up);
        if(advance<=0||(!owned_grounded&&advance*advance<.95f*n)||vertical*vertical>.0025f)return 0;
        float end=p+travel*advance;
        if(!finite(end)||end+lab_bounded_drift_heading_config.margin*length>=n)return 0;
        for(u32 endpoint=0;endpoint<2;endpoint++){
            float projection=(endpoint?end:p)/n,error=0;
            for(u32 i=0;i<3;i++){
                float e=o[i]+(endpoint?travel*axis[i]:0)-projection*d[i];error+=e*e;
            }
            if(!finite(error)||error>=clearance*clearance)return 0;
            float free=clearance-root(error);if(free<*slack)*slack=free;
        }
    }
    return 1;
}

#include "snaking_cached_predecessor_v305.h"
#include "drift_heading_node_v305.h"
#include "snaking_route_goal_v305.h"
#include "snaking_previous_base_heading_v305.h"

static u64 clock_for(u64 base){
    u64 root=q((void *)base,0x12f6388);if(!ptr(root))return 0;
    u64 single=q((void *)root,0);if(!ptr(single))return 0;
    u64 link=q((void *)single,0x20);if(!ptr(link))return 0;
    u64 scene=q((void *)link,8);if(!ptr(scene)||!(b((void *)scene,0x180)&4))return 0;
    u64 system=q((void *)scene,0x1b0);if(!ptr(system))return 0;
    u64 clocks=q((void *)system,0x218);if(!ptr(clocks))return 0;
    u64 clock=q((void *)clocks,0x50);return ptr(clock)&&w((void *)clock,0x38)==6?clock:0;
}

static u32 scope(const void *producer,u64 driver,u64 clock,u32 racer,u32 drift_state){
    if(b(producer,0x41)==1)return 1;
    if(b(producer,0x41)!=0)return 0;
    const u8 *owner=lab_snaking_state+88*racer;
    return q(owner,0)==driver&&q(owner,8)==(u64)producer&&q(owner,16)==clock&&
           !w(owner,32)&&w(owner,36)==w((void *)clock,0x44)&&w(owner,40)==1&&
           (int)w(owner,52)==(drift_state==4?-1:1);
}

EXPORT void lab_bounded_drift_heading_dispatch(void *state){
    u64 base=(u64)lab_module_start-LAB_MAIN_TO_MODULE;
    if((base&4095)||base>=(1ull<<39)-0x1400000||!ptr((u64)state))return;
    u64 driver=q(state,0x88),packet=q(state,0xc0),nav=q(state,0xb0);
    if(!ptr(driver)||!ptr(packet)||!ptr(nav)||q((void *)driver,0x50)!=(u64)state||
       q((void *)nav,0x28)!=(u64)state||q((void *)nav,0x58)!=driver||q((void *)nav,0x78)!=packet)return;
    u64 producer=q((void *)nav,0x70),source=q((void *)nav,0x60),target=q((void *)nav,0x80),other=q((void *)nav,0x68);
    if(!ptr(producer)||!ptr(source)||!ptr(target)||!ptr(other)||
       q((void *)producer,0x70)!=driver||q((void *)producer,0x80)!=source||
       q((void *)producer,0x98)!=packet||q((void *)producer,0xa0)!=target||
       q((void *)other,0x70)!=driver||b((void *)other,0x41)||b((void *)target,0x28)||
       b((void *)source,0x80))return;
    u64 route=q((void *)producer,0x88),adapter=q((void *)driver,0x88);
    if(!ptr(route)||!ptr(adapter)||q((void *)source,0x58)!=driver||q((void *)source,0x60)!=route)return;
    u64 role=q((void *)adapter,0x48),physics=q((void *)adapter,0x50);
    if(!ptr(role)||!ptr(physics)||q((void *)role,0x28)!=physics||q((void *)physics,0xf8)!=role||
       b((void *)role,0xd2)!=1||b((void *)role,0xd3)!=1||b((void *)role,0xe1)!=1||b((void *)role,0xe7)||
       (w((void *)role,0x1cc)&0x01204018u)||b((void *)physics,0x200)>1||b((void *)physics,0x201)>1||
       w((void *)physics,0x5d4)||f((void *)physics,0x3c8)!=0.f)return;
    /* Only the reviewed additive physical branch qualifies; do not assume the
     * same map across physics274>=1, transition states or nonfinite values. */
    float physical_branch=f((void *)physics,0x274);
    float physical_turn=f((void *)physics,0x3ac);
    if(!finite(physical_branch)||physical_branch<0||physical_branch>=1||
       !finite(physical_turn)||absf(physical_turn)>.08f)return;
    u32 racer=w((void *)role,0xa8);u64 drift=q((void *)physics,0x110),ai=q((void *)driver,0x90);
    if(racer>=12||!ptr(drift)||!ptr(ai)||q((void *)drift,8)!=role||
       q((void *)physics,0x100)!=ai||q((void *)ai,0x10)!=role)return;
    u32 ds=w((void *)drift,0x10),flags=w((void *)packet,4);
    if((ds!=2&&ds!=4&&ds!=5)||(flags&0xbu)!=9u)return;
    u32 heading_state=ds==2?((i32)w(lab_snaking_state+88*racer,52)<0?4u:5u):ds;
    float actual=f((void *)role,0x17c),pending_axis=f((void *)packet,8);
    if(heading_state==4){actual=-actual;pending_axis=-pending_axis;}
    if(!finite(actual)||!finite(pending_axis)||actual<.7f||actual>1.f||pending_axis<.7f)return;
    for(u32 i=0;i<3;i++){
        float value=f((void *)route,0x9c+4*i);
        if(!finite(value)||f((void *)target,4*i)!=value||f((void *)target,0xc+4*i)!=value)return;
    }
    float target_direction[3];
    if(!unit((void *)target,0x18,target_direction)||!finite(f((void *)target,0x24)))return;
    u64 native_base=be_course(base,(void *)driver,(void *)route),clock=clock_for(base);
    if(!native_base||!clock||!scope((void *)producer,driver,clock,racer,heading_state))return;
    if(ds==2){if(!srg_hop_scope((void *)producer,driver,physics,drift,clock,racer,native_base))return;}
    else if(!b((void *)physics,0x200)||!b((void *)physics,0x201))return;
    u64 selected=q((void *)driver,0x80),a=q((void *)route,0x60),z=q((void *)route,0x58);
    if(!be_span(a,0x374)||!be_span(z,0x374)||a==z||
       be_point((void *)selected,w((void *)a,0x8c))!=a||be_point((void *)selected,w((void *)z,0x8c))!=z||
       !be_edge((void *)a,w((void *)z,0x8c)))return;
    float up[3],motion[3],pending[3],d[3],o[3],n,p;
    if(!rotation_frame((void *)physics,up)||!unit((void *)physics,0x2fc,motion)||
       !unit((void *)physics,0x308,pending)||!span((void *)physics,a,z,up,d,o,&n,&p))return;
    if(p<0){
        u64 older=q((void *)route,0x68);
        if(!be_span(older,0x374)||older==a||older==z||
           be_point((void *)selected,w((void *)older,0x8c))!=older||!be_edge((void *)older,w((void *)a,0x8c)))return;
        z=a;a=older;if(!span((void *)physics,a,z,up,d,o,&n,&p))return;
    }
    u64 physical_prior=0,prior_cache[5]={0};
    if(p<0){
        extern u32 lab_snaking_ground_recovery_heading(void *,void *,void *,void *,u32,u32);
        if(ds==2||b((void *)producer,0x41)!=0||
           !lab_snaking_ground_recovery_heading((void *)driver,(void *)physics,(void *)producer,
                                               (void *)route,racer,w((void *)clock,0x44)))return;
        for(u32 k=0;k<5;k++)prior_cache[k]=q((void *)route,0x50+8*k);
        physical_prior=scp_earlier((void *)route,(void *)selected,a,z,prior_cache[1]);
        if(!physical_prior)return;
        z=a;a=physical_prior;
        if(!span((void *)physics,a,z,up,d,o,&n,&p))return;
    }
    if(p<0||p>n)return;
    float speed=f((void *)physics,0x37c);
    if(!finite(speed)||speed<=0)return;
    for(u32 off=0x424;off<=0x430;off+=4){float cap=f((void *)physics,off);if(!finite(cap)||cap<0)return;if(cap>speed)speed=cap;}
    if(speed>lab_bounded_drift_heading_config.max_speed)return;
    float selected_advance=dot(motion,d),pending_advance=dot(pending,d),length=root(n);
    float reserve=speed*lab_bounded_drift_heading_config.horizon+lab_bounded_drift_heading_config.margin;
    u32 owned_grounded=0;
    extern u32 lab_snaking_ground_recovery_heading(void *,void *,void *,void *,u32,u32);
    if(selected_advance<=0||pending_advance<=0)return;
    u32 needs_node=p+reserve*length>=n,node_future=0;
    float node_toward[3];HeadingNodeProof node_proof;
    if(selected_advance*selected_advance<.95f*n||pending_advance*pending_advance<.95f*n){
        /* Borrow only this tick's matching admitted custom owner. The local
         * actual/pending tube and bounded steering budget remain below. */
        if(b((void *)producer,0x41)!=0||
           !lab_snaking_ground_recovery_heading((void *)driver,(void *)physics,(void *)producer,
                                               (void *)route,racer,w((void *)clock,0x44)))return;
        owned_grounded=1;
    }
    u64 ba=be_point((void *)native_base,w((void *)a,0x8c)),bz=be_point((void *)native_base,w((void *)z,0x8c));
    float slack;
    if(!ba||!bz||!be_edge((void *)ba,w((void *)bz,0x8c)))return;
    u32 previous_covered=0;HeadingNodeProof previous_proof;
    u32 covered=!needs_node&&envelope((void *)physics,ba,bz,up,motion,pending,speed,&slack,owned_grounded);
    if(covered&&p+lab_bounded_drift_heading_config.lookahead*length>=n){
        u32 node_owned=owned_grounded;
        if(!node_owned&&b((void *)producer,0x41)==0&&
           lab_snaking_ground_recovery_heading((void *)driver,(void *)physics,(void *)producer,
                                              (void *)route,racer,w((void *)clock,0x44)))node_owned=1;
        float node_slack;
        if(hn_continue((void *)driver,(void *)physics,(void *)route,selected,native_base,
                       a,z,ba,bz,up,motion,pending,speed,node_owned,
                       &node_slack,node_toward,&node_proof)){
            slack=node_slack;node_future=1;
        }
    }
    if(!covered){
        if(!owned_grounded&&b((void *)producer,0x41)==0&&
           lab_snaking_ground_recovery_heading((void *)driver,(void *)physics,(void *)producer,
                                              (void *)route,racer,w((void *)clock,0x44))){
            owned_grounded=1;
            covered=!needs_node&&envelope((void *)physics,ba,bz,up,motion,pending,speed,&slack,1);
        }
        if(!covered&&!needs_node&&owned_grounded&&b((void *)producer,0x41)==0&&
           pbh_covered((void *)driver,(void *)physics,(void *)route,selected,native_base,
                       a,z,ba,bz,up,motion,pending,speed,&slack,&previous_proof)){
            covered=1;previous_covered=1;
        }
        if(!covered){
            if(!hn_continue((void *)driver,(void *)physics,(void *)route,selected,native_base,
                            a,z,ba,bz,up,motion,pending,speed,owned_grounded,
                            &slack,node_toward,&node_proof))return;
            node_future=1;
        }
    }
    float ahead=lab_bounded_drift_heading_config.lookahead;
    if(ahead>length-p/length)ahead=length-p/length;
    float toward[3],vertical=0;
    for(u32 i=0;i<3;i++){toward[i]=d[i]*(p/n+ahead/length)-o[i];vertical+=toward[i]*up[i];}
    for(u32 i=0;i<3;i++)toward[i]-=vertical*up[i];
    if(node_future)for(u32 i=0;i<3;i++)toward[i]=node_toward[i];
    if(!node_future&&b((void *)producer,0x41)==0&&!srg_aim(d,up,toward))return;
    float cross[3]={motion[1]*toward[2]-motion[2]*toward[1],
                    motion[2]*toward[0]-motion[0]*toward[2],motion[0]*toward[1]-motion[1]*toward[0]};
    float forward=dot(motion,toward),side=dot(cross,up);
    if(!finite(forward)||!finite(side)||forward<=0)return;
    float error=((float (*)(float,float))(base+0xb4e148))(side,forward);
    float correction=f((void *)drift,0x54)-f((void *)drift,0x38)-f((void *)drift,0x48);
    float original=f((void *)packet,0x10);
    float result=lab_bounded_drift_heading_value(original,error,correction,slack,speed);
    /* scope() already proved this producer's matching owned phase1 episode. */
    if(b((void *)producer,0x41)==0)
        result=lab_snaking_heading_compensation_value(original,error,correction,slack,speed);
    /* Recheck the immediate owner and original request before the only write. */
    if(physical_prior){
        for(u32 k=0;k<5;k++)if(q((void *)route,0x50+8*k)!=prior_cache[k])return;
        if(scp_earlier((void *)route,(void *)selected,prior_cache[3],prior_cache[2],
                       prior_cache[1])!=physical_prior)return;
    }
    if(node_future&&!hn_same(&node_proof,(void *)driver,(void *)route))return;
    if(previous_covered&&!hn_same(&previous_proof,(void *)driver,(void *)route))return;
    if(!finite(result)||result==original||q(state,0xc0)!=packet||q((void *)nav,0x78)!=packet||
       q((void *)nav,0x70)!=producer||w((void *)packet,4)!=flags||f((void *)packet,0x10)!=original||
       w((void *)drift,0x10)!=ds||!scope((void *)producer,driver,clock,racer,heading_state)||
       (ds==2&&!srg_hop_scope((void *)producer,driver,physics,drift,clock,racer,native_base)))return;
    *(float *)((u8 *)packet+0x10)=result;
    observe_heading(racer,driver,producer,physics,clock,(u64)state,packet,
        w((void *)clock,0x44),b((void *)producer,0x41)==1?1u:2u,ds,
        original,result,error,correction,slack,speed);
}
