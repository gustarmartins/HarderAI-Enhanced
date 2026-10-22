/* Experimental near-blue grace at the native route-history exit only.
 * No native memory writes, charge/timer changes or injected steering. The
 * caller retains every earlier native exit and the later backward-dot exit.
 * Course point envelopes are a conservative heuristic, not collision proof.
 */
typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long long u64;
#define EXPORT __attribute__((visibility("default")))

EXPORT const struct {
    u32 abi;
    u8 slot_updates[12];
    float margin, alignment_squared, max_speed;
    u32 max_counter;
} lab_drift_completion_config = {
    1, {9,9,9,9,9,9,9,9,9,9,9,9}, 24.f, .9025f, 100.f, 60000
};

typedef struct {
    u64 producer,drift;
    u32 first,last,limit,initialized;
    u32 lock,offered,held,reason;
} Budget;
EXPORT Budget lab_drift_completion_budget[12];
EXPORT const u32 lab_completion_deadline_config[2]={1,9};
EXPORT const u32 lab_drift_tier_finish_config[4]={1,3,9,0x2000};
EXPORT const u32 lab_blue_finish_rescue_config[4]={1,24,9,0};

static u64 q(const void *p,u32 o){return *(const u64 *)((const u8 *)p+o);}
static u32 w(const void *p,u32 o){return *(const u32 *)((const u8 *)p+o);}
static u8 b(const void *p,u32 o){return *((const u8 *)p+o);}
static float f(const void *p,u32 o){return *(const float *)((const u8 *)p+o);}
static u32 ptr(u64 p){return p>=0x10000 && !(p&7) && p<(1ull<<39)-0x1000;}
static u32 finite(float v){union{float f;u32 u;}x={.f=v};return (x.u&0x7fffffffu)<0x7f800000u;}
static u32 done(Budget *s,u32 result,u32 reason){
    s->reason=reason;s->held+=result;
    __atomic_store_n(&s->lock,0,__ATOMIC_RELEASE);return result;
}

static u32 simple_point(const void *p){
    u32 a=w(p,0x30),c=w(p,0x34);u8 selected=b(p,0x2e4);
    return a<=10 && c<=10 && selected<=1 && !b(p,0x370) && (selected?c:a)==1;
}

/* Experimental short-horizon envelope. Native movement directions, not kart
 * facing, determine projected travel. This is NOT a KCL collision oracle.
 * Reevaluate on every completion invocation; the shared nine-update limit stays.
 */
EXPORT const struct {u32 abi,max_ticks;float margin,max_turn;} lab_directional_drift_config={1,9,24.f,.08f};
static float root_positive(float v){float r;__asm__("fsqrt %s0,%s1":"=w"(r):"w"(v));return r;}

/* Native history60 -> target58 encloses the current approach. */
EXPORT const u32 lab_completion_approach_config[4]={1,0x60,0x58,0x50};
EXPORT const struct {u32 abi,up_offset;float plane2,norm_tolerance;} lab_completion_road_frame_config={1,0x14,.01f,.1f};
extern char lab_module_start[];
#include "time_trial_driving_scope_v305.h"
EXPORT const u32 lab_time_trial_completion_scope_config[4]={1,1,0,0};
/* Native-base route provenance for the bounded private clearance helpers.
 * Centers/extents are not KCL boundaries. Caller-owned selected route intent
 * remains separate; no pointer here carries a native lifetime lease. */
static u32 be_span(u64 p,u64 n){
    return ptr(p)&&n&&n<=(1ull<<39)-p;
}

static u64 be_point(void *course,u32 key){
    if(!be_span((u64)course,0x28)||key==0xffffffffu)return 0;
    u32 group=key&0xffffu,index=key>>16,count=w(course,0x10),capacity=w(course,0x18);
    u64 groups=q(course,0x20);
    if(!count||count>capacity||capacity>65536||group>=count||
       !be_span(groups,48ull*capacity))return 0;
    void *row=(void *)(groups+48ull*group);
    u32 n=w(row,0x10),cap=w(row,0x18);u64 items=q(row,0x20);
    if(!n||n>cap||cap>65536||index>=n||!be_span(items,8ull*cap))return 0;
    u64 result=q((void *)items,8*index);
    if(!be_span(result,0x374)||w((void *)result,0x8c)!=key)return 0;
    /* Processed negative extents remain valid data. Clearance callers decide
     * which paired view supplies their positive radius. */
    for(u32 i=0;i<3;i++)if(!finite(f((void *)result,0xc+4*i))||
                            !finite(f((void *)result,0x24+4*i)))return 0;
    return result;
}

static u32 be_edge(void *from,u32 destkey){
    if(!be_span((u64)from,0x374)||destkey==0xffffffffu)return 0;
    u32 next=w(from,0x30),previous=w(from,0x34);u8 selected=b(from,0x2e4);
    if(next>10||previous>10||selected>1||b(from,0x370)||
       (selected?previous:next)!=1)return 0;
    return w(from,selected?0x60:0x38)==destkey;
}

static u64 be_course(u64 mainbase,void *driver,void *route){
    if((mainbase&4095)||mainbase>(1ull<<39)-0x1300000||
       !be_span((u64)driver,0xd1)||!be_span((u64)route,0x70))return 0;
    u64 table=q(driver,0),selected=q(driver,0x80);
    if(!be_span(table,0x190)||!be_span(selected,0x28)||
       q((void *)table,0x148)!=mainbase+0x72364||
       q((void *)table,0x178)!=mainbase+0x72384||
       q((void *)table,0x188)!=mainbase+0x71ea4||
       q(route,0x40)!=(u64)driver||q(route,0x48)!=(u64)driver+0x80)return 0;
    u64 root=q((void *)mainbase,0x12f6388);if(!be_span(root,8))return 0;
    u64 single=q((void *)root,0);if(!be_span(single,0x28))return 0;
    u64 link=q((void *)single,0x20);if(!be_span(link,0x10))return 0;
    u64 scene=q((void *)link,8);if(!be_span(scene,0x1b8)||!(b((void *)scene,0x180)&4))return 0;
    u64 system=q((void *)scene,0x1b0);if(!be_span(system,0x240))return 0;
    u64 race=q((void *)system,0x238);if(!be_span(race,0xb8))return 0;
    u64 group=q((void *)race,0xb0);if(!be_span(group,0x58)||
       q((void *)group,0x28)!=race||q(driver,0x60)!=group)return 0;
    u64 manager=q((void *)group,0x50);if(!ptr(manager))return 0;
    u8 special=b(driver,0xc4),variant=b(driver,0xd0);
    u64 alternate=0,resource=0,common=0,field=0;
    if(variant){
        if(!be_span(manager,0x59b0))return 0;
        alternate=q((void *)manager,0x59a8);
        if(alternate&&!be_span(alternate,0x28))return 0;
    }
    if(special&1){
        if(!be_span(manager,0x59b0))return 0;
        field=manager+(alternate?0x59a8:0x59a0);
    }else{
        resource=q((void *)system,0x220);if(!be_span(resource,0x198))return 0;
        if(alternate){
            if(!be_span(resource,0x9a8))return 0;
            field=resource+0x9a0;
        }else{
            common=q((void *)resource,0x190);if(!be_span(common,0xf0))return 0;
            field=common+0xe8;
        }
    }
    u64 expected=q((void *)field,0);if(!be_span(expected,0x28))return 0;
    /* The qualified native getter already runs in 5C210's width predicate.
     * Preflight every branch-dependent dereference before invoking it. */
    u64 resolved=((u64 (*)(void *))(mainbase+0x7bf5c4))(driver);
    if(resolved!=expected)return 0;
    /* Detect observed owner/selector changes around the native call. This is
     * structural readback, not an assertion of concurrent lifetime safety. */
    if(q((void *)mainbase,0x12f6388)!=root||q((void *)root,0)!=single||
       q((void *)single,0x20)!=link||q((void *)link,8)!=scene||
       !(b((void *)scene,0x180)&4)||q((void *)scene,0x1b0)!=system||
       q((void *)system,0x238)!=race||q((void *)race,0xb0)!=group||
       q((void *)group,0x28)!=race||q((void *)group,0x50)!=manager||
       q(driver,0x60)!=group||q(driver,0)!=table||q(driver,0x80)!=selected||
       q(route,0x40)!=(u64)driver||q(route,0x48)!=(u64)driver+0x80||
       b(driver,0xc4)!=special||b(driver,0xd0)!=variant||
       q((void *)table,0x148)!=mainbase+0x72364||
       q((void *)table,0x178)!=mainbase+0x72384||
       q((void *)table,0x188)!=mainbase+0x71ea4||q((void *)field,0)!=expected)return 0;
    if(variant&&q((void *)manager,0x59a8)!=alternate)return 0;
    if(!(special&1)&&(q((void *)system,0x220)!=resource||
       (!alternate&&q((void *)resource,0x190)!=common)))return 0;
    return resolved;
}

/* Preserve the selected route's longitudinal and near-planar intent.
 * Lateral containment is evaluated separately around native base centers. */
static u32 be_intent(const void *physics,const void *a,const void *z,
                     const float *up,float up2,float plane2,u32 occupied,
                     float *delta,float *length2,float *projection){
    float n=0,p=0,vertical=0;
    for(u32 i=0;i<3;i++){
        float start=f(a,0xc+4*i),end=f(z,0xc+4*i),position=f(physics,0x2c+4*i);
        if(!finite(start)||!finite(end)||!finite(position))return 0;
        delta[i]=end-start;n+=delta[i]*delta[i];
        p+=(position-start)*delta[i];vertical+=delta[i]*up[i];
    }
    if(!finite(n)||n<64||!finite(p)||vertical*vertical>plane2*n*up2||
       (occupied&&(p<0||p>n)))return 0;
    *length2=n;*projection=p;return 1;
}

EXPORT const u32 lab_completion_base_envelope_config[4]={1,0x7bf5c4,0x10,0x18};
EXPORT const u32 lab_completion_occupied_segment_config[4]={1,0x60,0x58,0x68};
#include "blue_finish_segment_bridge_v305.h"
static u32 corridor(void *producer,const void *physics,const void *route,u32 ticks,u32 tier){
    if(!ticks||ticks>(tier?lab_directional_drift_config.max_ticks:lab_blue_finish_rescue_config[1])||w(physics,0x5d4))return 0;
    u64 previous=q(route,0x60),next=q(route,0x58);
    if(!ptr(previous)||!ptr(next)||previous==next)return 0;
    const void *a=(const void *)previous,*z=(const void *)next;
    if(!simple_point(a)||!simple_point(z))return 0;
    u64 driver=q(producer,0x70);
    u64 native_base=be_course((u64)lab_module_start-LAB_MAIN_TO_MODULE,(void *)driver,(void *)route);
    if(!native_base)return 0;
    u64 selected=q((void *)driver,0x80);
    if(be_point((void *)selected,w(a,0x8c))!=previous||be_point((void *)selected,w(z,0x8c))!=next||
       !be_edge((void *)a,w(z,0x8c)))return 0;
    float up[3],up2=0;
    for(u32 i=0;i<3;i++){up[i]=f(physics,0x14+4*i);if(!finite(up[i]))return 0;up2+=up[i]*up[i];}
    if(up2<.9f||up2>1.1f)return 0;
    float selected_delta[3],selected_length2,selected_projection;
    /* A native cached history point can still be ahead of the kart. Inspect
     * the projection without relaxing finite, length or road-plane checks. */
    if(!be_intent(physics,a,z,up,up2,.01f,0,selected_delta,&selected_length2,&selected_projection))return 0;
    if(selected_projection<0){
        u64 older=q(route,0x68);
        if(!be_span(older,0x374)||older==previous||older==next)return 0;
        const void *candidate=(const void *)older;
        if(!simple_point(candidate)||be_point((void *)selected,w(candidate,0x8c))!=older||
           !be_edge((void *)candidate,w(a,0x8c)))return 0;
        /* The cache must describe a real adjacent selected-course edge and
         * contain the position. Being older alone does not qualify it. */
        if(!be_intent(physics,candidate,a,up,up2,.01f,1,
                      selected_delta,&selected_length2,&selected_projection))return 0;
        next=previous;previous=older;z=a;a=candidate;
    }else if(selected_projection>selected_length2)return 0;
    u64 ba=be_point((void *)native_base,w(a,0x8c)),bz=be_point((void *)native_base,w(z,0x8c));
    if(!ba||!bz||!be_edge((void *)ba,w((void *)bz,0x8c)))return 0;
    float selected_length=root_positive(selected_length2);
    a=(const void *)ba;z=(const void *)bz;
    float radius=10000.f,delta[3],offset[3],length2=0,projection=0;
    for(u32 i=0;i<3;i++){
        float ra=f(a,0x24+4*i),rz=f(z,0x24+4*i);
        float start=f(a,0xc+4*i),end=f(z,0xc+4*i),position=f(physics,0x2c+4*i);
        if(!finite(ra)||!finite(rz)||ra<=0||rz<=0||ra>10000||rz>10000||
           !finite(start)||!finite(end)||!finite(position))return 0;
        if(ra<radius)radius=ra;if(rz<radius)radius=rz;
        delta[i]=end-start;offset[i]=position-start;
        length2+=delta[i]*delta[i];projection+=offset[i]*delta[i];
    }
    float normal_delta=0;for(u32 i=0;i<3;i++)normal_delta+=delta[i]*up[i];
    if(!finite(length2)||length2<64||!finite(projection)||projection<0||projection>length2||
       normal_delta*normal_delta>.01f*length2*up2)return 0;
    float speed=f(physics,0x37c),turn=f(physics,0x3ac);
    if(!finite(speed)||speed<=0||!finite(turn)||turn>lab_directional_drift_config.max_turn||
       turn< -lab_directional_drift_config.max_turn)return 0;
    for(u32 o=0x424;o<=0x430;o+=4){
        float cap=f(physics,o);if(!finite(cap)||cap<0)return 0;if(cap>speed)speed=cap;
    }
    if(speed>lab_drift_completion_config.max_speed)return 0;
    float travel=speed*ticks;
    /* Authored uncertainty reserve for turns during the short hold. It is a
     * prediction assumption, not a proven upper bound on subsequent steering. */
    float reserve=lab_directional_drift_config.margin+
        speed*lab_directional_drift_config.max_turn*ticks*(ticks+1)*.5f;
    float clearance=radius-reserve;
    if(!finite(clearance)||clearance<=0)return 0;
    float length=root_positive(length2);
    for(u32 mode=0;mode<2;mode++){
        float motion[3],norm2=0;
        for(u32 i=0;i<3;i++){
            motion[i]=f(physics,(mode?0x308:0x2fc)+4*i);
            if(!finite(motion[i]))return 0;norm2+=motion[i]*motion[i];
        }
        float normal_motion=0;for(u32 i=0;i<3;i++)normal_motion+=motion[i]*up[i];
        if(!finite(norm2)||norm2<.81f||norm2>1.21f||normal_motion*normal_motion>.01f*norm2*up2)return 0;
        float norm=root_positive(norm2),advance=0;
        for(u32 i=0;i<3;i++){motion[i]/=norm;advance+=motion[i]*delta[i];}
        if(!finite(advance)||advance<=0)return 0;
        float selected_advance=0;
        for(u32 i=0;i<3;i++)selected_advance+=motion[i]*selected_delta[i];
        if(!finite(selected_advance)||selected_advance<=0||
           selected_projection+travel*selected_advance+reserve*selected_length>=selected_length2)return 0;
        float end_projection=projection+travel*advance;
        if(!finite(end_projection)||end_projection+reserve*length>=length2)return 0;
        /* A straight projected segment has convex squared distance from the
         * route line. Checking both ends bounds that segment, not a whole race. */
        for(u32 endpoint=0;endpoint<2;endpoint++){
            float t=(endpoint?end_projection:projection)/length2,residual2=0;
            for(u32 i=0;i<3;i++){
                float residual=offset[i]+(endpoint?travel*motion[i]:0)-t*delta[i];
                residual2+=residual*residual;
            }
            if(!finite(residual2)||residual2>=clearance*clearance)return 0;
        }
    }
    return 1;
}

static u32 bfb_corridor(void *producer,const void *physics,const void *route,u32 ticks,u32 tier){
    if(tier)return 0;
    if(!ticks||ticks>(tier?lab_directional_drift_config.max_ticks:lab_blue_finish_rescue_config[1])||w(physics,0x5d4))return 0;
    u64 previous=q(route,0x60),next=q(route,0x58);
    if(!ptr(previous)||!ptr(next)||previous==next)return 0;
    const void *a=(const void *)previous,*z=(const void *)next;
    if(!simple_point(a)||!simple_point(z))return 0;
    u64 driver=q(producer,0x70);
    u64 native_base=be_course((u64)lab_module_start-LAB_MAIN_TO_MODULE,(void *)driver,(void *)route);
    if(!native_base)return 0;
    u64 selected=q((void *)driver,0x80);
    if(be_point((void *)selected,w(a,0x8c))!=previous||be_point((void *)selected,w(z,0x8c))!=next||
       !be_edge((void *)a,w(z,0x8c)))return 0;
    float up[3],up2=0;
    for(u32 i=0;i<3;i++){up[i]=f(physics,0x14+4*i);if(!finite(up[i]))return 0;up2+=up[i]*up[i];}
    if(up2<.9f||up2>1.1f)return 0;
    float selected_delta[3],selected_length2,selected_projection;
    /* A native cached history point can still be ahead of the kart. Inspect
     * the projection without relaxing finite, length or road-plane checks. */
    if(!be_intent(physics,a,z,up,up2,.01f,0,selected_delta,&selected_length2,&selected_projection))return 0;
    if(selected_projection<0){
        u64 older=q(route,0x68);
        if(!be_span(older,0x374)||older==previous||older==next)return 0;
        const void *candidate=(const void *)older;
        if(!simple_point(candidate)||be_point((void *)selected,w(candidate,0x8c))!=older||
           !be_edge((void *)candidate,w(a,0x8c)))return 0;
        /* The cache must describe a real adjacent selected-course edge and
         * contain the position. Being older alone does not qualify it. */
        if(!be_intent(physics,candidate,a,up,up2,.01f,1,
                      selected_delta,&selected_length2,&selected_projection))return 0;
        next=previous;previous=older;z=a;a=candidate;
    }else if(selected_projection>selected_length2)return 0;
    u64 ba=be_point((void *)native_base,w(a,0x8c)),bz=be_point((void *)native_base,w(z,0x8c));
    if(!ba||!bz||!be_edge((void *)ba,w((void *)bz,0x8c)))return 0;
    a=(const void *)ba;z=(const void *)bz;
    float radius=10000.f,delta[3],offset[3],length2=0,projection=0;
    for(u32 i=0;i<3;i++){
        float ra=f(a,0x24+4*i),rz=f(z,0x24+4*i);
        float start=f(a,0xc+4*i),end=f(z,0xc+4*i),position=f(physics,0x2c+4*i);
        if(!finite(ra)||!finite(rz)||ra<=0||rz<=0||ra>10000||rz>10000||
           !finite(start)||!finite(end)||!finite(position))return 0;
        if(ra<radius)radius=ra;if(rz<radius)radius=rz;
        delta[i]=end-start;offset[i]=position-start;
        length2+=delta[i]*delta[i];projection+=offset[i]*delta[i];
    }
    float normal_delta=0;for(u32 i=0;i<3;i++)normal_delta+=delta[i]*up[i];
    if(!finite(length2)||length2<64||!finite(projection)||projection<0||projection>length2||
       normal_delta*normal_delta>.01f*length2*up2)return 0;
    float speed=f(physics,0x37c),turn=f(physics,0x3ac);
    if(!finite(speed)||speed<=0||!finite(turn)||turn>lab_directional_drift_config.max_turn||
       turn< -lab_directional_drift_config.max_turn)return 0;
    for(u32 o=0x424;o<=0x430;o+=4){
        float cap=f(physics,o);if(!finite(cap)||cap<0)return 0;if(cap>speed)speed=cap;
    }
    if(speed>lab_drift_completion_config.max_speed)return 0;
    float travel=speed*ticks;
    /* Authored uncertainty reserve for turns during the short hold. It is a
     * prediction assumption, not a proven upper bound on subsequent steering. */
    float reserve=lab_directional_drift_config.margin+
        speed*lab_directional_drift_config.max_turn*ticks*(ticks+1)*.5f;
    float clearance=radius-reserve;
    if(!finite(clearance)||clearance<=0)return 0;
    /* The actual occupied pair was selected and checked above. Use only
     * its current cached adjacent successor, never search/choose a new branch. */
    u64 following=next==q(route,0x60)?q(route,0x58):q(route,0x50);
    if(!ptr(following)||following==previous||following==next||
       be_point((void *)selected,w((void *)following,0x8c))!=following||
       !simple_point((void *)following)||!be_edge((void *)next,w((void *)following,0x8c)))return 0;
    u64 bf=be_point((void *)native_base,w((void *)following,0x8c));
    if(!bf||bf==ba||bf==bz||!be_edge((void *)bz,w((void *)bf,0x8c))||
       !simple_point((void *)bf))return 0;
    float next_selected_delta[3],next_selected_length2,next_selected_projection;
    float next_delta[3],next_length2,next_projection,next_offset[3];
    if(!be_intent(physics,(void *)next,(void *)following,up,up2,.01f,0,
                  next_selected_delta,&next_selected_length2,&next_selected_projection)||
       !be_intent(physics,(void *)bz,(void *)bf,up,up2,.01f,0,
                  next_delta,&next_length2,&next_projection)||
       !bfb_straight(selected_delta,selected_length2,next_selected_delta,next_selected_length2)||
       !bfb_straight(delta,length2,next_delta,next_length2))return 0;
    float next_radius=radius;
    for(u32 i=0;i<3;i++){
        float r=f((void *)bf,0x24+4*i);
        if(!finite(r)||r<=0||r>10000)return 0;if(r<next_radius)next_radius=r;
        next_offset[i]=f(physics,0x2c+4*i)-f((void *)bz,0xc+4*i);
        if(!finite(next_offset[i]))return 0;
    }
    float next_clearance=next_radius-reserve;
    if(!finite(next_clearance)||next_clearance<=0)return 0;
    for(u32 mode=0;mode<2;mode++){
        float motion[3],norm2=0;
        for(u32 i=0;i<3;i++){
            motion[i]=f(physics,(mode?0x308:0x2fc)+4*i);
            if(!finite(motion[i]))return 0;norm2+=motion[i]*motion[i];
        }
        float normal_motion=0;for(u32 i=0;i<3;i++)normal_motion+=motion[i]*up[i];
        if(!finite(norm2)||norm2<.81f||norm2>1.21f||normal_motion*normal_motion>.01f*norm2*up2)return 0;
        float norm=root_positive(norm2);for(u32 i=0;i<3;i++)motion[i]/=norm;
        BfbInterval current_base,next_base,current_selected,next_selected;
        if(!bfb_interval(delta,length2,projection,offset,motion,clearance,0.f,1,&current_base)||
           !bfb_interval(next_delta,next_length2,next_projection,next_offset,motion,next_clearance,reserve,1,&next_base)||
           !bfb_interval(selected_delta,selected_length2,selected_projection,offset,motion,0.f,0.f,0,&current_selected)||
           !bfb_interval(next_selected_delta,next_selected_length2,next_selected_projection,offset,motion,0.f,reserve,0,&next_selected)||
           !bfb_cover(current_base,next_base,travel)||!bfb_cover(current_selected,next_selected,travel))return 0;
    }
    return 1;
}
EXPORT const struct {u32 abi,max_updates;float slow,strong,inward;u32 both_requests;}
lab_drift_charge_forecast_config={1,9,2.f,5.f,.7f,1};
EXPORT float lab_drift_completion_gain(void *producer,void *role,void *navigation,void *state,u32 drift_state){
    u64 packet=q(producer,0x98);
    if(!ptr(packet)||q(navigation,0x78)!=packet||q(state,0xc0)!=packet)return 2.f;
    float actual=f(role,0x17c),pending=f((void *)packet,8);
    if(drift_state==4){actual=-actual;pending=-pending;}
    else if(drift_state!=5)return 2.f;
    if(!finite(actual)||!finite(pending)||actual< -1.f||actual>1.f)return 2.f;
    /* Pending input is clamped later by native70B8C; finite overshoot can
     * still qualify. Publication can also gate it to zero after this call. */
    return actual>=lab_drift_charge_forecast_config.inward&&
           pending>=lab_drift_charge_forecast_config.inward?5.f:2.f;
}

EXPORT const u32 lab_completion_lifecycle_config[4]={1,0x5d3f8,48,28};
EXPORT void lab_corner_arc_legacy_end(void *producer){
    if(!ptr((u64)producer))return;
    u64 driver=q(producer,0x70);if(!ptr(driver))return;
    u64 adapter=q((void *)driver,0x88),state=q((void *)driver,0x50);
    if(!ptr(adapter)||!ptr(state)||q((void *)state,0x88)!=driver)return;
    u64 navigation=q((void *)state,0xb0),ratio=q((void *)state,0x90);
    if(!ptr(navigation)||!ptr(ratio)||q((void *)navigation,0x70)!=(u64)producer||
       q((void *)navigation,0x58)!=driver||q((void *)ratio,0x58)!=driver)return;
    u64 role=q((void *)adapter,0x48),physics=q((void *)adapter,0x50),wrapper=q((void *)ratio,0x48);
    u32 solo=!wrapper && tts_admitted((u64)lab_module_start-LAB_MAIN_TO_MODULE,(u64)producer);
    if(!ptr(role)||!ptr(physics)||(!solo&&(!ptr(wrapper)||q((void *)wrapper,8)!=driver))||
       q((void *)role,0x28)!=physics||q((void *)physics,0xf8)!=role||
       b((void *)role,0xd2)!=1||b((void *)role,0xd3)!=1||b((void *)role,0xe7))return;
    u32 racer=w((void *)role,0xa8);if(racer>=12)return;
    u64 drift=q((void *)physics,0x110);if(!ptr(drift)||q((void *)drift,8)!=role)return;
    Budget *s=&lab_drift_completion_budget[racer];u32 expected=0;
    if(!__atomic_compare_exchange_n(&s->lock,&expected,1,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED))return;
    /* Native state/counter clearing follows this call. No native object,
     * charge, deadline length or cumulative diagnostics are changed here. */
    if(s->producer==(u64)producer&&s->drift==drift)s->initialized=0;
    __atomic_store_n(&s->lock,0,__ATOMIC_RELEASE);
}

EXPORT u32 lab_corner_arc_legacy_hold(void *producer,u32 point_match,float native_dot){
    if(!ptr((u64)producer)||point_match>1||!finite(native_dot)||native_dot<0)return 0;
    u64 driver=q(producer,0x70);
    if(!ptr(driver))return 0;
    u64 adapter=q((void *)driver,0x88),state=q((void *)driver,0x50);
    if(!ptr(adapter)||!ptr(state)||q((void *)state,0x88)!=driver)return 0;
    u64 navigation=q((void *)state,0xb0),ratio=q((void *)state,0x90);
    if(!ptr(navigation)||!ptr(ratio)||q((void *)navigation,0x70)!=(u64)producer||
       q((void *)navigation,0x58)!=driver||q((void *)ratio,0x58)!=driver)return 0;
    u64 role=q((void *)adapter,0x48),physics=q((void *)adapter,0x50),wrapper=q((void *)ratio,0x48);
    u32 solo=!wrapper && tts_admitted((u64)lab_module_start-LAB_MAIN_TO_MODULE,(u64)producer);
    if(!ptr(role)||!ptr(physics)||(!solo&&(!ptr(wrapper)||q((void *)wrapper,8)!=driver))||
       q((void *)role,0x28)!=physics||q((void *)physics,0xf8)!=role||
       b((void *)role,0xd3)!=1||b((void *)role,0xd2)!=1||b((void *)role,0xe1)!=1||b((void *)role,0xe7)||
       (w((void *)role,0x1cc)&0x01204018u))return 0;
    u32 racer=w((void *)role,0xa8),slot=solo?0u:w((void *)wrapper,0x24),counter=w(producer,0x44);
    if(racer>=12||slot>=12||counter>lab_drift_completion_config.max_counter)return 0;
    Budget *s=&lab_drift_completion_budget[racer];u32 expected=0;
    if(!__atomic_compare_exchange_n(&s->lock,&expected,1,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED))return 0;
    s->offered++;
    /* Keep the first offer's window while this maneuver continues. */
    if(s->initialized && (s->producer!=(u64)producer || counter<s->last))s->initialized=0;
    s->last=counter;
    if(!point_match)return done(s,0,1);
    u64 drift=q((void *)physics,0x110),route=q(producer,0x88);
    if(!ptr(drift)||!ptr(route)||q((void *)drift,8)!=role||b(producer,0x41)!=1)return done(s,0,2);
    u32 drift_state=w((void *)drift,0x10),limit=lab_drift_completion_config.slot_updates[slot];
    float direction=f(producer,0xa8);
    u32 tier=w((void *)drift,0x8c);
    if(tier>=3 || (tier==2 && (w((void *)role,0x1cc)&0x2000u)))return done(s,0,3);
    /* A first useful turbo may need more time than improving an earned one.
     * The original budget start/deadline and per-update corridor forecast
     * still bound the attempt. Crossing blue restores the upper-tier limit. */
    if(tier==0 && limit)limit=lab_blue_finish_rescue_config[1];
    if(!limit||!finite(direction)||
       !((drift_state==4&&direction<0)||(drift_state==5&&direction>0)))
        return done(s,0,3);
    if(!s->initialized||s->producer!=(u64)producer||s->drift!=drift||counter<s->last){
        s->producer=(u64)producer;s->drift=drift;s->first=counter;s->limit=limit;s->initialized=1;
    }
    s->last=counter;if(s->limit<limit)limit=s->limit;
    u32 elapsed=counter-s->first;
    if(elapsed>=limit)return done(s,0,4);
    float charge=f((void *)drift,0x70),target=f((void *)drift,0x80+4*tier),difference=target-charge;
    if(!finite(charge)||!finite(target)||charge<0||target<=0||!finite(difference)||difference<0)
        return done(s,0,5);
    float gain=lab_drift_completion_gain(producer,(void *)role,(void *)navigation,(void *)state,drift_state);
    float projected=charge;u32 needed=0;
    /* Mirror the strict native float32 additions, including equality. */
    while(projected<=target&&needed<limit-elapsed){projected+=gain;needed++;}
    if(projected<=target)return done(s,0,5);
    if(!corridor(producer,(void *)physics,(void *)route,needed,tier)&&
       !bfb_corridor(producer,(void *)physics,(void *)route,needed,tier))return done(s,0,6);
    return done(s,1,0);
}

#include "corner_arc_selected_successor_overlay_v305.h"
EXPORT u32 lab_drift_completion_hold(void *producer,u32 request,float dot){
    u32 old=lab_corner_arc_legacy_hold(producer,request,dot);
    u32 extra=lab_corner_arc_hold(producer,request,dot,old);
    return old||extra;
}
EXPORT void lab_drift_completion_end(void *producer){
    lab_corner_arc_end(producer);lab_corner_arc_legacy_end(producer);
}
