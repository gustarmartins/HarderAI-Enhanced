/* Experimental autonomous-CPU blue-turbo pairs on connected straight routes.
 * All charge, drift-state transitions and boost requests remain native.
 * Only this callback's owned movement packet is edited. Physical players,
 * ordinary native drift maneuvers, recoveries and unsupported scenes pass on.
 */
typedef unsigned char u8;
typedef unsigned int u32;
typedef signed int i32;
typedef unsigned long long u64;
#define EXPORT __attribute__((visibility("default")))
extern char lab_module_start[];
#include "time_trial_driving_scope_v305.h"
EXPORT const u32 lab_time_trial_snaking_scope_config[4]={1,1,0,0};

EXPORT const struct {
    u32 abi,leg_deadline,settle,initial_delay;
    u32 cooldown[12];
    float margin,minimum_width,alignment_squared,maximum_speed;
} lab_snaking_config={1,80,12,90,
    {90,90,90,120,120,120,150,150,150,150,150,150},
    40.f,90.f,.95f,40.f};

/* Course-progress distances, shared with the native formation wrapper. A
 * larger gap changes how often a CPU may earn a turbo, never its AI ratio. */
EXPORT const struct {
    u32 abi;
    float crowded,neutral_gap,ramp,catch_reduction,leader_delay;
} lab_snaking_gap_config={1,120.f,300.f,1200.f,.35f,.5f};

typedef struct {
    u64 driver,producer,clock,previous_end;
    u32 lock,last,phase,start,next_tick;
    i32 direction,first_side;
    u32 steer_count,steer_period,episodes,release_requests,aborts,reason,pad;
} SnakeState;
_Static_assert(sizeof(SnakeState)==88,"Snaking state ABI");
EXPORT SnakeState lab_snaking_state[12];

static u64 q(const void *p,u32 o){return *(const u64 *)((const u8 *)p+o);}
static u32 w(const void *p,u32 o){return *(const u32 *)((const u8 *)p+o);}
static u8 b(const void *p,u32 o){return *((const u8 *)p+o);}
static float f(const void *p,u32 o){return *(const float *)((const u8 *)p+o);}
static void sw(void *p,u32 o,u32 v){*(u32 *)((u8 *)p+o)=v;}
static void sf(void *p,u32 o,float v){*(float *)((u8 *)p+o)=v;}
static u32 ptr(u64 p){return p>=0x10000 && !(p&7) && p<(1ull<<39)-0x2000;}
static u32 finite(float v){union {float f;u32 u;}x={.f=v};return (x.u&0x7fffffffu)<0x7f800000u;}
static float root(float v){float out;__asm__("fsqrt %s0,%s1":"=w"(out):"w"(v));return out;}

static float gap_weight(float distance){
    float v=(distance-lab_snaking_gap_config.neutral_gap)/lab_snaking_gap_config.ramp;
    return v<0?0:v>1?1:v;
}

EXPORT u32 lab_snaking_gap_cooldown(u32 cooldown,float ahead,float behind){
    if(!cooldown||!finite(ahead)||!finite(behind)||ahead<0||behind<0)return cooldown;
    float factor=ahead<1.e20f?1.f-lab_snaking_gap_config.catch_reduction*gap_weight(ahead):
        behind<1.e20f?1.f+lab_snaking_gap_config.leader_delay*gap_weight(behind):1.f;
    return (u32)((float)cooldown*factor+.5f);
}

static u32 pack_cooldown(u64 wrapper,u64 wrappers,u64 drivers,u64 group,u32 count,
                          u32 racer,u32 cooldown,u32 *crowded){
    *crowded=0;
    float progress=f((void *)wrapper,0x2c),ahead=1.e20f,behind=1.e20f;
    if(count<2||count>12||!finite(progress))return cooldown;
    for(u32 i=0;i<count;i++)if(i!=racer){
        u64 peer=q((void *)wrappers,8*i),driver=q((void *)drivers,8*i);
        if(!ptr(peer)||!ptr(driver)||peer==wrapper||driver==q((void *)wrapper,8)||
           q((void *)peer,8)!=driver||q((void *)driver,0x60)!=group)return cooldown;
        for(u32 j=0;j<i;j++)if(q((void *)wrappers,8*j)==peer||q((void *)drivers,8*j)==driver)return cooldown;
        float delta=f((void *)peer,0x2c)-progress;
        if(!finite(delta))return cooldown;
        if(delta>=0){if(delta<ahead)ahead=delta;}else if(-delta<behind)behind=-delta;
    }
    *crowded=ahead<lab_snaking_gap_config.crowded||behind<lab_snaking_gap_config.crowded;
    return lab_snaking_gap_cooldown(cooldown,ahead,behind);
}

/* Native route keys index group records then point pointers. Check the key
 * back on the resolved point, including the currently selected point. */
static u32 successor(void *p,u32 *key){
    u32 a=w(p,0x30),z=w(p,0x34);u8 selector=b(p,0x2e4);
    if(a>10||z>10||selector>1||b(p,0x370)||(selector?z:a)!=1)return 0;
    *key=w(p,selector?0x60:0x38);return 1;
}

/* Native route history60 -> current target58 is the occupied approach. */
EXPORT const u32 lab_snaking_approach_config[4]={1,0x60,0x58,0x50};
EXPORT const struct {u32 abi,up_offset;float plane2,norm_tolerance;} lab_snaking_road_frame_config={1,0x14,.0025f,.1f};
#include "snaking_cpu_charge_horizon_v305.h"
EXPORT const struct {u32 abi,entry_allowance,release_updates,max_leg;float gain,minimum_inward;}
lab_snaking_charge_horizon_config={1,31,1,80,5.f,.71f};
EXPORT u32 lab_snaking_corridor_ticks(void *drift,u32 state,u32 elapsed){
    u32 remaining=elapsed<80?80-elapsed:0;
    u32 fallback=remaining+lab_snaking_config.settle;
    if(!ptr((u64)drift)||elapsed>=80||(state!=0&&state!=2&&state!=4&&state!=5))return fallback;
    if(state==4||state==5){
        u64 role=q(drift,8);if(!ptr(role))return fallback;
        /* Native17293C ->181B7C ->182008 supplies role+170 as input;
         * explicit17BE10 reads the charging axis at input+0C. */
        float axis=f((void *)role,0x17c),inward=state==4?-axis:axis;
        if(!sch_inward((void *)role,inward))return fallback;
    }
    float target=f(drift,0x80),charge=(state==4||state==5)?f(drift,0x70):0.f;
    if(!finite(target)||target<=0||target>400||!finite(charge)||charge<0)return fallback;
    u32 ticks=0;
    /* Actual explicit native charging uses scale1; our owned axis is +/-1.
     * Simulate its strict > comparison, retaining the original deadline cap. */
    while(charge<=target&&ticks<80){charge+=5.f;ticks++;}
    u32 entry=(state==4||state==5)?0:(elapsed<31?31-elapsed:0);
    u32 needed=entry+ticks+1;
    return (needed<remaining?needed:remaining)+lab_snaking_config.settle;
}

EXPORT const struct {u32 abi,record_bytes;float alignment_squared,max_projected_step;}
lab_snaking_repeat_clearance_config={1,48,.95f,80.f};
typedef struct {
    u64 course,endpoint;
    float origin[3],axis[3],distance,last_advance;
} SnakeReservation;
_Static_assert(sizeof(SnakeReservation)==48,"Snaking reservation ABI");
EXPORT SnakeReservation lab_snaking_reservations[12];

/* A discontinuity removes only the supplemental unlock. 80 units/update is
 * a conservative observation policy, not a claimed native motion bound. */
EXPORT void lab_snaking_update_reservation(SnakeReservation *r,void *physics,void *route,u32 dt){
    if(!r->endpoint)return;
    u64 holder=q(route,0x48);
    if(dt>1||!ptr(holder)||q((void *)holder,0)!=r->course){r->endpoint=0;return;}
    float advance=0;
    for(u32 i=0;i<3;i++){
        float position=f(physics,0x2c+4*i);
        if(!finite(position)||!finite(r->origin[i])||!finite(r->axis[i])){r->endpoint=0;return;}
        advance+=(position-r->origin[i])*r->axis[i];
    }
    float change=advance-r->last_advance;
    float limit=dt?lab_snaking_repeat_clearance_config.max_projected_step:0.f;
    if(!finite(advance)||!finite(change)||change>limit||change<-limit){r->endpoint=0;return;}
    r->last_advance=advance;
}

EXPORT u32 lab_snaking_reservation_cleared(const SnakeReservation *r,void *physics,void *route,u64 endpoint){
    if(!endpoint||r->endpoint!=endpoint||!finite(r->distance)||r->distance<=0)return 0;
    u64 holder=q(route,0x48);
    if(!ptr(holder)||q((void *)holder,0)!=r->course)return 0;
    float advance=0,dot=0,axis2=0,heading2=0;
    for(u32 i=0;i<3;i++){
        float position=f(physics,0x2c+4*i),heading=f(physics,0x20+4*i);
        if(!finite(position)||!finite(heading)||!finite(r->origin[i])||!finite(r->axis[i]))return 0;
        advance+=(position-r->origin[i])*r->axis[i];
        dot+=heading*r->axis[i];heading2+=heading*heading;axis2+=r->axis[i]*r->axis[i];
    }
    if(!finite(advance)||axis2<.9f||axis2>1.1f||heading2<.9f||heading2>1.1f||dot<=0||
       dot*dot<lab_snaking_repeat_clearance_config.alignment_squared*heading2*axis2)return 0;
    return advance>=r->distance*root(axis2);
}

EXPORT void lab_snaking_remember_reservation(SnakeReservation *r,void *physics,void *route,u64 endpoint,float distance){
    r->endpoint=0;
    if(!endpoint||!finite(distance)||distance<=0)return;
    u64 holder=q(route,0x48);if(!ptr(holder))return;
    u64 course=q((void *)holder,0);if(!ptr(course))return;
    float norm=0;
    for(u32 i=0;i<3;i++){
        r->origin[i]=f(physics,0x2c+4*i);r->axis[i]=f(physics,0x20+4*i);
        if(!finite(r->origin[i])||!finite(r->axis[i]))return;
        norm+=r->axis[i]*r->axis[i];
    }
    if(norm<.9f||norm>1.1f)return;
    norm=root(norm);for(u32 i=0;i<3;i++)r->axis[i]/=norm;
    r->course=course;r->distance=distance;r->last_advance=0;r->endpoint=endpoint;
}

extern char lab_module_start[];
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

EXPORT const u32 lab_snaking_base_envelope_config[4]={1,0x7bf5c4,0x10,0x18};
EXPORT const u32 lab_snaking_occupied_segment_config[4]={1,0x60,0x58,0x68};
#include "snaking_hop_frame_v305.h"
#include "snaking_drift_frame_v305.h"
#include "snaking_drift_refusal_diag_v305.h"

static u32 dfr_corridor_ticks(void *drift,u32 state,u32 elapsed,DfrContext *context){
    u32 remaining=elapsed<80?80-elapsed:0;
    u32 fallback=remaining+lab_snaking_config.settle;
    if(!ptr((u64)drift)||elapsed>=80||(state!=0&&state!=2&&state!=4&&state!=5))return fallback;
    if(state==4||state==5){
        u64 role=q(drift,8);if(!ptr(role))return fallback;
        /* Native17293C ->181B7C ->182008 supplies role+170 as input;
         * explicit17BE10 reads the charging axis at input+0C. */
        float axis=f((void *)role,0x17c),inward=state==4?-axis:axis;
        context->horizon_role=role;context->axis=dfr_bits(axis);context->inward=dfr_bits(inward);context->horizon_flags|=1u;
        if(!sch_inward((void *)role,inward))return fallback;
        context->horizon_flags|=2u;
    }
    float target=f(drift,0x80),charge=(state==4||state==5)?f(drift,0x70):0.f;
    context->target=dfr_bits(target);context->initial_charge=dfr_bits(charge);context->horizon_flags|=4u;
    if(!finite(target)||target<=0||target>400||!finite(charge)||charge<0)return fallback;
    context->horizon_flags|=8u;
    u32 ticks=0;
    /* Actual explicit native charging uses scale1; our owned axis is +/-1.
     * Simulate its strict > comparison, retaining the original deadline cap. */
    while(charge<=target&&ticks<80){charge+=5.f;ticks++;}
    u32 entry=(state==4||state==5)?0:(elapsed<31?31-elapsed:0);
    u32 needed=entry+ticks+1;
    return (needed<remaining?needed:remaining)+lab_snaking_config.settle;
}
static u32 corridor(void *driver,void *physics,void *route,u32 ticks,u64 previous_end,u64 *last,u32 *old_ahead,float *reserved){
    u64 holder=q(route,0x48),a=q(route,0x60),z=q(route,0x58);
    if(!ptr(holder)||!ptr(a)||!ptr(z)||a==z)return 0;
    u64 c=q((void *)holder,0);if(!ptr(c))return 0;
    void *course=(void *)c;
    u64 native_base=be_course((u64)lab_module_start-LAB_MAIN_TO_MODULE,driver,route);
    if(!native_base)return 0;
    if(be_point(course,w((void *)z,0x8c))!=z||be_point(course,w((void *)a,0x8c))!=a)return 0;
    u32 approach_key;
    if(!successor((void *)a,&approach_key)||be_point(course,approach_key)!=z)return 0;
    float speed=f(physics,0x37c);
    if(!finite(speed)||speed<=2.f)return 0;
    for(u32 o=0x424;o<=0x430;o+=4){float v=f(physics,o);if(!finite(v)||v<0)return 0;if(v>speed)speed=v;}
    if(speed>lab_snaking_config.maximum_speed)return 0;
    float required=speed*ticks+64.f,remaining=0,selected_remaining=0,heading[3],norm=0;
    for(u32 i=0;i<3;i++){heading[i]=f(physics,0x20+4*i);if(!finite(heading[i]))return 0;norm+=heading[i]*heading[i];}
    if(norm<.9f||norm>1.1f)return 0;
    float up[3],up2=0,heading_normal=0;
    for(u32 i=0;i<3;i++){
        up[i]=f(physics,0x14+4*i);if(!finite(up[i]))return 0;
        up2+=up[i]*up[i];heading_normal+=heading[i]*up[i];
    }
    if(up2<.9f||up2>1.1f||heading_normal*heading_normal>.0025f*norm*up2)return 0;
    /* Cache age is not occupation. Keep the current selected edge valid,
     * then qualify at most its one cached predecessor before the normal walk. */
    float occupied_delta[3],occupied_length2,occupied_projection;
    if(!be_intent(physics,(void *)a,(void *)z,up,up2,.0025f,0,
                  occupied_delta,&occupied_length2,&occupied_projection))return 0;
    if(occupied_projection<0){
        u64 older=q(route,0x68);
        if(!be_span(older,0x374)||older==a||older==z||
           be_point(course,w((void *)older,0x8c))!=older||
           !be_edge((void *)older,w((void *)a,0x8c)))return 0;
        if(!be_intent(physics,(void *)older,(void *)a,up,up2,.0025f,1,
                      occupied_delta,&occupied_length2,&occupied_projection))return 0;
        z=a;a=older;
    }else if(occupied_projection>occupied_length2)return 0;
    u64 seen[18];seen[0]=a;u32 used=1;*old_ahead=0;
    for(u32 segment=0;segment<16;segment++){
        if(z==previous_end)*old_ahead=1;
        for(u32 j=0;j<used;j++)if(seen[j]==z)return 0;
        seen[used++]=z;
        u64 ba=be_point((void *)native_base,w((void *)a,0x8c));
        u64 bz=be_point((void *)native_base,w((void *)z,0x8c));
        if(!ba||!bz||!be_edge((void *)ba,w((void *)bz,0x8c)))return 0;
        float selected_delta[3],selected_length2,selected_projection,selected_dot=0;
        if(!be_intent(physics,(void *)a,(void *)z,up,up2,.0025f,!segment,
                      selected_delta,&selected_length2,&selected_projection))return 0;
        for(u32 i=0;i<3;i++)selected_dot+=selected_delta[i]*heading[i];
        if(selected_dot<=0||selected_dot*selected_dot<lab_snaking_config.alignment_squared*selected_length2*norm)return 0;
        float selected_length=root(selected_length2);
        selected_remaining+=!segment?selected_length-selected_projection/selected_length:selected_length;
        float delta[3],length2=0,dot=0,width=10000.f;
        for(u32 j=0;j<2;j++){
            void *p=(void *)(j?bz:ba);float radius=f(p,0x24);
            /* Retail 810E98..810F00 loads Scale and halves all components.
             * X is also the native drift-width gate at 7B6D14 (threshold 70). */
            if(!finite(radius)||radius<lab_snaking_config.minimum_width||radius>10000)return 0;
            if(radius<width)width=radius;
        }
        for(u32 i=0;i<3;i++){
            float start=f((void *)ba,0xc+4*i),end=f((void *)bz,0xc+4*i);
            if(!finite(start)||!finite(end))return 0;
            delta[i]=end-start;length2+=delta[i]*delta[i];dot+=delta[i]*heading[i];
        }
        float normal_delta=0;for(u32 i=0;i<3;i++)normal_delta+=delta[i]*up[i];
        if(!finite(length2)||length2<64||dot<=0||dot*dot<lab_snaking_config.alignment_squared*length2*norm||
           normal_delta*normal_delta>.0025f*length2*up2)return 0;
        float length=root(length2);
        if(!segment){
            float offset[3],projection=0,error=0;
            for(u32 i=0;i<3;i++){
                float position=f(physics,0x2c+4*i);if(!finite(position))return 0;
                offset[i]=position-f((void *)ba,0xc+4*i);projection+=offset[i]*delta[i];
            }
            if(projection<0||projection>length2)return 0;
            for(u32 i=0;i<3;i++){float e=offset[i]-projection/length2*delta[i];error+=e*e;}
            float margin=lab_snaking_config.margin+2.f*speed;
            if(width<=margin||!finite(error)||error>=(width-margin)*(width-margin))return 0;
            remaining=length-projection/length;
        }else remaining+=length;
        *last=z;
        if(remaining>=required&&selected_remaining>=required){if(reserved)*reserved=required;return 1;}
        u32 key;if(!successor((void *)z,&key))return 0;
        a=z;z=be_point(course,key);if(!z)return 0;
    }
    return 0;
}

EXPORT const u32 lab_snaking_cached_predecessor_config[4]={1,1,4,0x70};
#include "snaking_cached_predecessor_v305.h"
#include "snaking_hop_node_bridge_v305.h"
#include "snaking_hop_air_route_frame_v305.h"
#include "snaking_hop_vector_refusal_diag_v305.h"
#include "snaking_hop_vector_refusal_generated_v305.h"
#include "snaking_hop_base_successor_v305.h"
#include "snaking_hop_refusal_diag_v305.h"
#include "snaking_hop_refusal_future_generated_v305.h"
/* Future route-frame admission; never substitutes for the occupied plane.
 * Native7BCE40/5E1B8 consume point90 as a route orientation normal. This is
 * route geometry, not a claim that a point normal proves future KCL contact. */
EXPORT const struct {u32 abi,point_normal_offset,max_edges,fallback_normal_offset;float normal_alignment_squared;}
lab_snaking_future_plane_config={2,0x90,16,0x9c,.95f};

static u32 sfp_normal(const void *point,float *normal){
    if(hf_unit(point,0x90,normal,.999f,1.001f))return 1;
    /* The retail sentinel is exactly (0,0,0), including signed zero under
     * its float comparisons. Malformed nonzero normals never take fallback. */
    for(u32 i=0;i<3;i++)if(f(point,0x90+4*i)!=0.f)return 0;
    return hf_unit(point,0x9c,normal,.999f,1.001f);
}

static u32 sfp_normals(const void *a,const void *z,const HopVectors *v,
                       float *one,float *two){
    if(!sfp_normal(a,one)||!sfp_normal(z,two))return 0;
    float values[5]={hf_dot(one,two),hf_dot(one,v->up),hf_dot(two,v->up),
                     hf_dot(one,v->target_up),hf_dot(two,v->target_up)};
    for(u32 i=0;i<5;i++)if(!finite(values[i])||values[i]<=0||
        values[i]*values[i]<lab_snaking_future_plane_config.normal_alignment_squared)return 0;
    return 1;
}

static u32 sfp_intent(const void *physics,const void *a,const void *z,
    const HopVectors *v,u32 future,float *delta,float *length2,float *projection){
    if(!future)return hf_intent(physics,a,z,v,0,delta,length2,projection);
    float one[3],two[3];
    if(!sfp_normals(a,z,v,one,two)||
       !be_intent(physics,a,z,one,1.f,.0025f,0,delta,length2,projection))return 0;
    float normal=hf_dot(delta,two);
    return finite(normal)&&normal*normal<=.0025f*(*length2);
}

static u32 sfp_walk_intent(const void *physics,const void *a,const void *z,
    const HopVectors *v,u32 occupied,u32 future,float *delta,float *length2,float *projection){
    if(hf_intent(physics,a,z,v,occupied,delta,length2,projection))return 1;
    if(!future)return 0;
    return sfp_intent(physics,a,z,v,1,delta,length2,projection);
}

static u32 sfp_projected_cone(const float *delta,float length2,const float *axis,
                              const float *up,float alignment){
    float dn=hf_dot(delta,up),an=hf_dot(axis,up);
    float edge2=length2-dn*dn,axis2=hf_dot(axis,axis)-an*an;
    float advance=hf_dot(delta,axis)-dn*an;
    return finite(edge2)&&finite(axis2)&&finite(advance)&&edge2>=.90f*length2&&
        axis2>=.95f&&advance>0&&advance*advance>=alignment*edge2*axis2;
}

static u32 sfp_walk_edge(const void *a,const void *z,const float *delta,float length2,
                         const HopVectors *v,u32 future,float alignment){
    if(!finite(length2)||length2<64.f)return 0;
    float n=hf_dot(delta,v->up),t=hf_dot(delta,v->target_up);
    if(future){
        float one[3],two[3];if(!sfp_normals(a,z,v,one,two))return 0;
        n=hf_dot(delta,one);t=hf_dot(delta,two);
    }
    float actual=hf_dot(delta,v->motion),pending=hf_dot(delta,v->pending);
    if(!finite(actual)||!finite(pending)||!finite(n)||!finite(t)||actual<=0||pending<=0||
       n*n>.0025f*length2||t*t>.0025f*length2)return 0;
    if(!future)return actual*actual>=alignment*length2&&pending*pending>=alignment*length2;
    /* Grade is bounded by native future normals above. Test lateral steering
     * in both current control planes, so uphill/downhill rotation is not
     * counted a second time as a sideways heading error. */
    return sfp_projected_cone(delta,length2,v->motion,v->up,alignment)&&
           sfp_projected_cone(delta,length2,v->pending,v->up,alignment)&&
           sfp_projected_cone(delta,length2,v->motion,v->target_up,alignment)&&
           sfp_projected_cone(delta,length2,v->pending,v->target_up,alignment);
}

EXPORT const u32 lab_snaking_hop_grade_config[4]={3,16,30,3};
EXPORT const u32 lab_snaking_hop_predecessor_config[4]={1,1,4,0};
static u32 gbp_occupation(const void *,const void *,void *,void *,
    u64,u64,u64,u64,const float *,float,float,float,float,const HopVectors *,float *);
static u32 hf_corridor_diag_inner(void *driver,void *physics,void *route,u32 ticks,u64 previous_end,u64 *last,u32 *old_ahead,float *reserved,const SnakeHopFrame *frame,HfdProbe *probe){
    u64 holder=q(route,0x48),a=q(route,0x60),z=q(route,0x58);
    if(!ptr(holder)||!ptr(a)||!ptr(z)||a==z)return hfd_root_fail(probe,1,0xffffffffu,0xffffffffu,0xffffffffu,a,z,0,0,(const float *)0,0);
    u64 c=q((void *)holder,0);if(!ptr(c))return hfd_root_fail(probe,2,0xffffffffu,0xffffffffu,0xffffffffu,a,z,0,0,(const float *)0,0);
    void *course=(void *)c;
    u64 native_base=be_course((u64)lab_module_start-LAB_MAIN_TO_MODULE,driver,route);
    if(!native_base||native_base!=frame->base_course||c!=frame->course||(u64)physics!=frame->physics||q(physics,0x110)!=frame->drift)return hfd_root_fail(probe,3,0xffffffffu,0xffffffffu,0xffffffffu,a,z,0,0,(const float *)0,0);
    if(be_point(course,w((void *)z,0x8c))!=z||be_point(course,w((void *)a,0x8c))!=a)return hfd_root_fail(probe,4,0xffffffffu,0xffffffffu,0xffffffffu,a,z,0,0,(const float *)0,0);
    u32 approach_key;
    if(!successor((void *)a,&approach_key)||be_point(course,approach_key)!=z)return hfd_root_fail(probe,5,0xffffffffu,0xffffffffu,0xffffffffu,a,z,0,0,(const float *)0,0);
    float speed=f(physics,0x37c);
    if(!finite(speed)||speed<=2.f)return hfd_root_fail(probe,6,0xffffffffu,0xffffffffu,0xffffffffu,a,z,0,0,(const float[]){speed},1);
    for(u32 o=0x424;o<=0x430;o+=4){float v=f(physics,o);if(!finite(v)||v<0)return hfd_root_fail(probe,7,0xffffffffu,0xffffffffu,o,a,z,0,0,(const float[]){speed,v},2);if(v>speed)speed=v;}
    if(speed>lab_snaking_config.maximum_speed)return hfd_root_fail(probe,8,0xffffffffu,0xffffffffu,0xffffffffu,a,z,0,0,(const float[]){speed,lab_snaking_config.maximum_speed},2);
    float required=speed*ticks+64.f,remaining=0,selected_remaining=0,heading[3],norm=0;
    HopVectors hv;if(!hvr_observe(physics,route,ticks,frame,&hv))return hfd_root_fail(probe,9,0xffffffffu,0xffffffffu,0xffffffffu,a,z,0,0,(const float[]){speed,required},2);
    float up[3],up2=1.f;norm=1.f;
    for(u32 i=0;i<3;i++){heading[i]=hv.motion[i];up[i]=hv.up[i];}
    /* Cache age is not occupation. Keep the current selected edge valid,
     * then qualify at most its one cached predecessor before the normal walk. */
    float occupied_delta[3],occupied_length2,occupied_projection;
    if(!hf_intent(physics,(void *)a,(void *)z,&hv,0,
                  occupied_delta,&occupied_length2,&occupied_projection))return hfd_root_fail(probe,10,0xffffffffu,0,0xffffffffu,a,z,0,0,(const float[]){hv.motion[0],hv.motion[1],hv.motion[2],hv.pending[0],hv.pending[1],hv.pending[2],hv.up[0],hv.up[1],hv.up[2],hv.target_up[0],hv.target_up[1],hv.target_up[2]},12);
    if(occupied_projection<0){
        u64 older=q(route,0x68);
        if(!be_span(older,0x374)||older==a||older==z||
           be_point(course,w((void *)older,0x8c))!=older||
           !be_edge((void *)older,w((void *)a,0x8c)))return hfd_root_fail(probe,11,0xffffffffu,0,0xffffffffu,a,z,0,0,(const float[]){occupied_projection,occupied_length2},2);
        if(!hf_intent(physics,(void *)older,(void *)a,&hv,1,
                      occupied_delta,&occupied_length2,&occupied_projection))return hfd_root_fail(probe,12,0xffffffffu,0,0xffffffffu,older,a,0,0,(const float[]){hv.motion[0],hv.motion[1],hv.motion[2],hv.pending[0],hv.pending[1],hv.pending[2],hv.up[0],hv.up[1],hv.up[2],hv.target_up[0],hv.target_up[1],hv.target_up[2]},12);
        z=a;a=older;
    }else if(occupied_projection>occupied_length2)return hfd_root_fail(probe,13,0xffffffffu,0,0xffffffffu,a,z,0,0,(const float[]){occupied_projection,occupied_length2},2);
    u32 future_coverage=0;u64 first_a=a,first_z=z;
    u64 seen[18];seen[0]=a;u32 used=1;*old_ahead=0;
    for(u32 segment=0;segment<16;segment++){
        if(z==previous_end)*old_ahead=1;
        for(u32 j=0;j<used;j++)if(seen[j]==z)return hfd_root_fail(probe,14,segment,0xffffffffu,0xffffffffu,a,z,0,0,(const float *)0,0);
        seen[used++]=z;
        u64 ba=be_point((void *)native_base,w((void *)a,0x8c));
        u64 bz=be_point((void *)native_base,w((void *)z,0x8c));
        if(!ba||!bz||!be_edge((void *)ba,w((void *)bz,0x8c)))return hfd_root_fail(probe,15,segment,1,0xffffffffu,a,z,ba,bz,(const float *)0,0);
        float selected_delta[3],selected_length2,selected_projection,selected_dot=0;
        if(!sfp_walk_intent(physics,(void *)a,(void *)z,&hv,!segment,segment!=0,
                      selected_delta,&selected_length2,&selected_projection))return hfd_root_fail(probe,16,segment,0,0xffffffffu,a,z,ba,bz,(const float[]){hv.motion[0],hv.motion[1],hv.motion[2],hv.pending[0],hv.pending[1],hv.pending[2],hv.up[0],hv.up[1],hv.up[2],hv.target_up[0],hv.target_up[1],hv.target_up[2]},12);
        if(!(hf_edge(selected_delta,selected_length2,&hv)||(segment&&sfp_walk_edge((void *)a,(void *)z,selected_delta,selected_length2,&hv,1,lab_snaking_config.alignment_squared)))&&!hfr_allow(segment,&future_coverage,physics,course,(void *)native_base,first_a,first_z,speed,required,&hv,probe))return hfd_root_fail(probe,17,segment,0,0xffffffffu,a,z,ba,bz,(const float[]){selected_delta[0],selected_delta[1],selected_delta[2],selected_length2,selected_projection,speed,required,hv.motion[0],hv.motion[1],hv.motion[2],hv.pending[0],hv.pending[1],hv.pending[2],hv.up[0],hv.up[1],hv.up[2],hv.target_up[0],hv.target_up[1],hv.target_up[2]},19);
        for(u32 i=0;i<3;i++)selected_dot+=selected_delta[i]*heading[i];
        if(selected_dot<=0||((!segment&&selected_dot*selected_dot<lab_snaking_config.alignment_squared*selected_length2*norm)&&!hfr_allow(segment,&future_coverage,physics,course,(void *)native_base,first_a,first_z,speed,required,&hv,probe)))return hfd_root_fail(probe,18,segment,0,0xffffffffu,a,z,ba,bz,(const float[]){selected_dot,selected_length2,norm,speed,required},5);
        float selected_length=root(selected_length2);
        selected_remaining+=!segment?selected_length-selected_projection/selected_length:selected_length;
        float delta[3],length2=0,dot=0,width=10000.f;
        for(u32 j=0;j<2;j++){
            void *p=(void *)(j?bz:ba);float radius=f(p,0x24);
            /* Retail 810E98..810F00 loads Scale and halves all components.
             * X is also the native drift-width gate at 7B6D14 (threshold 70). */
            if(!finite(radius)||radius<lab_snaking_config.minimum_width||radius>10000)return hfd_root_fail(probe,19,segment,1,j,a,z,ba,bz,(const float[]){radius,lab_snaking_config.minimum_width},2);
            if(radius<width)width=radius;
        }
        for(u32 i=0;i<3;i++){
            float start=f((void *)ba,0xc+4*i),end=f((void *)bz,0xc+4*i);
            if(!finite(start)||!finite(end))return hfd_root_fail(probe,20,segment,1,i,a,z,ba,bz,(const float[]){start,end},2);
            delta[i]=end-start;length2+=delta[i]*delta[i];dot+=delta[i]*heading[i];
        }
        if(!(hf_edge(delta,length2,&hv)||(segment&&sfp_walk_edge((void *)ba,(void *)bz,delta,length2,&hv,1,lab_snaking_config.alignment_squared)))&&!hfr_allow(segment,&future_coverage,physics,course,(void *)native_base,first_a,first_z,speed,required,&hv,probe))return hfd_root_fail(probe,21,segment,1,0xffffffffu,a,z,ba,bz,(const float[]){delta[0],delta[1],delta[2],length2,speed,required,hv.motion[0],hv.motion[1],hv.motion[2],hv.pending[0],hv.pending[1],hv.pending[2],hv.up[0],hv.up[1],hv.up[2],hv.target_up[0],hv.target_up[1],hv.target_up[2]},18);
        float normal_delta=0;for(u32 i=0;i<3;i++)normal_delta+=delta[i]*up[i];
        if(!finite(length2)||length2<64||dot<=0||((!segment&&dot*dot<lab_snaking_config.alignment_squared*length2*norm)&&!hfr_allow(segment,&future_coverage,physics,course,(void *)native_base,first_a,first_z,speed,required,&hv,probe))||
           (!segment&&normal_delta*normal_delta>.0025f*length2*up2))return hfd_root_fail(probe,22,segment,1,0xffffffffu,a,z,ba,bz,(const float[]){length2,dot,norm,normal_delta,up2,speed,required},7);
        float length=root(length2);
        if(!segment){
            float offset[3],projection=0,error=0;
            for(u32 i=0;i<3;i++){
                float position=f(physics,0x2c+4*i);if(!finite(position))return hfd_root_fail(probe,23,segment,1,i,a,z,ba,bz,(const float[]){position},1);
                offset[i]=position-f((void *)ba,0xc+4*i);projection+=offset[i]*delta[i];
            }
            u32 base_predecessor=projection<0;
            float prior_remaining=0;
            if(base_predecessor&&!gbp_occupation(physics,route,course,(void *)native_base,
                a,z,ba,bz,delta,length2,projection,width,speed,&hv,&prior_remaining))
                return hfd_root_fail(probe,24,segment,1,0xffffffffu,a,z,ba,bz,(const float[]){projection,length2,speed,required},4);
            u32 base_successor=projection>=length2;float base_debt=0;
            if(base_successor&&!hbs_occupation(physics,route,course,(void *)native_base,
                a,z,ba,bz,delta,length2,projection,speed,&hv,&base_debt))return hfd_root_fail(probe,25,segment,1,0xffffffffu,a,z,ba,bz,(const float[]){projection,length2,speed,required,delta[0],delta[1],delta[2],hv.motion[0],hv.motion[1],hv.motion[2],hv.pending[0],hv.pending[1],hv.pending[2],hv.up[0],hv.up[1],hv.up[2],hv.target_up[0],hv.target_up[1],hv.target_up[2]},19);
            for(u32 i=0;i<3;i++){float e=offset[i]-projection/length2*delta[i];error+=e*e;}
            float margin=lab_snaking_config.margin+2.f*speed;
            if(width<=margin||!finite(error)||error>=(width-margin)*(width-margin))return hfd_root_fail(probe,26,segment,1,0xffffffffu,a,z,ba,bz,(const float[]){width,margin,error,projection,length2,speed,required},7);
            u32 hnp_detail=0;float hnp_observed[4]={0};
            if(!base_predecessor&&!base_successor&&!hnb_projection(physics,route,course,(void *)native_base,a,z,ba,bz,delta,length2,width,speed,&hv,&hnp_detail,hnp_observed))return hfd_root_fail(probe,27,segment,1,hnp_detail,a,z,ba,bz,(const float[]){projection,length2,width,speed,required,delta[0],delta[1],delta[2],hv.motion[0],hv.motion[1],hv.motion[2],hv.pending[0],hv.pending[1],hv.pending[2],hv.up[0],hv.up[1],hv.up[2],hv.target_up[0],hv.target_up[1],hv.target_up[2],hnp_observed[0],hnp_observed[1],hnp_observed[2],hnp_observed[3]},24);
            remaining=base_predecessor?length+prior_remaining:(base_successor?-base_debt:length-projection/length);
        }else remaining+=length;
        *last=z;
        if(remaining>=required&&selected_remaining>=required){if(reserved)*reserved=required;return 1;}
        u32 key;if(!successor((void *)z,&key))return hfd_root_fail(probe,28,segment,0xffffffffu,0xffffffffu,a,z,ba,bz,(const float[]){remaining,selected_remaining,required,speed},4);
        a=z;z=be_point(course,key);if(!z)return hfd_root_fail(probe,29,segment,0xffffffffu,0xffffffffu,a,z,0,0,(const float[]){remaining,selected_remaining,required,speed},4);
    }
    return hfd_root_fail(probe,30,0xffffffffu,0xffffffffu,0xffffffffu,a,z,0,0,(const float[]){remaining,selected_remaining,required,speed},4);
}

EXPORT u32 lab_snaking_hop_route_recovery(void *,void *,void *,u32,const SnakeHopFrame *);
static u32 hf_corridor(void *driver,void *physics,void *route,u32 ticks,u64 previous_end,u64 *last,u32 *old_ahead,float *reserved,const SnakeHopFrame *frame){
    HfdProbe probe={0};
    u32 accepted=hf_corridor_diag_inner(driver,physics,route,ticks,previous_end,last,old_ahead,reserved,frame,&probe);
    if(!accepted&&!previous_end&&!reserved&&!probe.substage&&probe.segment==0&&(probe.stage==17||probe.stage==21))
        accepted=lab_snaking_hop_route_recovery(driver,physics,route,ticks,frame);
    if(!accepted)hfd_publish(&probe,frame,physics,route,ticks);
    return accepted;
}

EXPORT const u32 lab_snaking_drift_node_bridge_config[6]={1,12,48,4,2,0};
#include "snaking_ground_base_predecessor_v305.h"
EXPORT const u32 lab_snaking_ground_base_successor_config[4]={1,2,4,0};
EXPORT const struct {u32 abi;float alignment_squared;u32 padding,reserved;}
lab_snaking_padding_recovery_config={1,.90f,64,0};
static u32 spr_edge(const float *delta,float length2,const HopVectors *v){
    if(!finite(length2)||length2<64.f)return 0;
    float a=hf_dot(delta,v->motion),z=hf_dot(delta,v->pending);
    float n=hf_dot(delta,v->up),t=hf_dot(delta,v->target_up);
    return finite(a)&&finite(z)&&finite(n)&&finite(t)&&a>0&&z>0&&
           a*a>=lab_snaking_padding_recovery_config.alignment_squared*length2&&
           z*z>=lab_snaking_padding_recovery_config.alignment_squared*length2&&
           n*n<=.0025f*length2&&t*t<=.0025f*length2;
}
#include "snaking_selected_joint_v305.h"
static u32 df_corridor_diag_inner(void *driver,void *physics,void *route,u32 ticks,u64 previous_end,u64 *last,u32 *old_ahead,float *reserved,const SnakeHopFrame *frame,DfrProbe *probe){
    u64 holder=q(route,0x48),a=q(route,0x60),z=q(route,0x58);
    if(!ptr(holder)||!ptr(a)||!ptr(z)||a==z)return dfr_fail(probe,1,0xffffffffu,0xffffffffu,0xffffffffu,0,0,0,0,(const float *)0,0);
    u64 c=q((void *)holder,0);if(!ptr(c))return dfr_fail(probe,2,0xffffffffu,0xffffffffu,0xffffffffu,a,z,0,0,(const float *)0,0);
    void *course=(void *)c;
    u64 native_base=be_course((u64)lab_module_start-LAB_MAIN_TO_MODULE,driver,route);
    if(!native_base||native_base!=frame->base_course||c!=frame->course||(u64)physics!=frame->physics||q(physics,0x110)!=frame->drift)return dfr_fail(probe,3,0xffffffffu,0xffffffffu,0xffffffffu,a,z,0,0,(const float *)0,0);
    if(be_point(course,w((void *)z,0x8c))!=z||be_point(course,w((void *)a,0x8c))!=a)return dfr_fail(probe,4,0xffffffffu,0xffffffffu,0xffffffffu,a,z,0,0,(const float *)0,0);
    u32 approach_key;
    if(!successor((void *)a,&approach_key)||be_point(course,approach_key)!=z)return dfr_fail(probe,5,0xffffffffu,0xffffffffu,0xffffffffu,a,z,0,0,(const float *)0,0);
    float speed=f(physics,0x37c);
    if(!finite(speed)||speed<=2.f)return dfr_fail(probe,6,0xffffffffu,0xffffffffu,0xffffffffu,a,z,0,0,(const float[]){speed},1);
    for(u32 o=0x424;o<=0x430;o+=4){float v=f(physics,o);if(!finite(v)||v<0)return dfr_fail(probe,7,0xffffffffu,0xffffffffu,o,a,z,0,0,(const float[]){speed,v},2);if(v>speed)speed=v;}
    if(speed>lab_snaking_config.maximum_speed)return dfr_fail(probe,8,0xffffffffu,0xffffffffu,0xffffffffu,a,z,0,0,(const float[]){speed,lab_snaking_config.maximum_speed},2);
    float maneuver=speed*ticks;
    float required=maneuver+64.f,remaining=0,selected_remaining=0,heading[3],norm=0;
    probe->required_distance=dfr_bits(required);probe->required_distance_valid=1;
    HopVectors hv;if(!df_vectors(physics,frame,&hv))return dfr_fail(probe,9,0xffffffffu,0xffffffffu,0xffffffffu,a,z,0,0,(const float[]){speed,required},2);
    float up[3],up2=1.f;norm=1.f;
    for(u32 i=0;i<3;i++){heading[i]=hv.motion[i];up[i]=hv.up[i];}
    /* Cache age is not occupation. Keep the current selected edge valid,
     * then qualify at most its one cached predecessor before the normal walk. */
    u32 selected_joint_gap=0;
    float occupied_delta[3],occupied_length2,occupied_projection;
    if(!hf_intent(physics,(void *)a,(void *)z,&hv,0,
                  occupied_delta,&occupied_length2,&occupied_projection))return dfr_fail(probe,10,0xffffffffu,0,0xffffffffu,a,z,0,0,(const float[]){hv.motion[0],hv.motion[1],hv.motion[2],hv.pending[0],hv.pending[1],hv.pending[2],hv.up[0],hv.up[1],hv.up[2],hv.target_up[0],hv.target_up[1],hv.target_up[2]},12);
    if(occupied_projection<0){
        u64 older=q(route,0x68);
        if(!be_span(older,0x374)||older==a||older==z||
           be_point(course,w((void *)older,0x8c))!=older||
           !be_edge((void *)older,w((void *)a,0x8c)))return dfr_fail(probe,11,0xffffffffu,0,0xffffffffu,a,z,0,0,(const float[]){occupied_projection,occupied_length2},2);
        u64 physical_prior=0;
        if(!hf_intent(physics,(void *)older,(void *)a,&hv,1,
                      occupied_delta,&occupied_length2,&occupied_projection)&&
           !(selected_joint_gap=sjg_occupation(physics,(void *)older,(void *)a,
                                               (void *)z,speed,&hv))){
            if(!hf_intent(physics,(void *)older,(void *)a,&hv,0,
                          occupied_delta,&occupied_length2,&occupied_projection)||
               occupied_projection>=0||
               !(physical_prior=scp_earlier(route,course,older,a,z))||
               !hf_intent(physics,(void *)physical_prior,(void *)older,&hv,1,
                          occupied_delta,&occupied_length2,&occupied_projection)||
               !hf_edge(occupied_delta,occupied_length2,&hv))
                return sjg_refusal(probe,physics,(void *)older,(void *)a,(void *)z,&hv);
        }
        if(physical_prior){z=older;a=physical_prior;}
        else{z=a;a=older;}
    }else if(occupied_projection>occupied_length2)return dfr_fail(probe,13,0xffffffffu,0,0xffffffffu,a,z,0,0,(const float[]){occupied_projection,occupied_length2},2);
    u64 seen[18];seen[0]=a;u32 used=1;*old_ahead=0;
    for(u32 segment=0;segment<16;segment++){
        u32 padding_only=segment&&remaining>=maneuver&&selected_remaining>=maneuver;
        if(z==previous_end)*old_ahead=1;
        for(u32 j=0;j<used;j++)if(seen[j]==z)return dfr_fail(probe,14,segment,0xffffffffu,0xffffffffu,a,z,0,0,(const float *)0,0);
        seen[used++]=z;
        u64 ba=be_point((void *)native_base,w((void *)a,0x8c));
        u64 bz=be_point((void *)native_base,w((void *)z,0x8c));
        if(!ba||!bz||!be_edge((void *)ba,w((void *)bz,0x8c)))return dfr_fail(probe,15,segment,1,0xffffffffu,a,z,ba,bz,(const float *)0,0);
        float selected_delta[3],selected_length2,selected_projection,selected_dot=0;
        if(!sfp_walk_intent(physics,(void *)a,(void *)z,&hv,!segment&&!selected_joint_gap,segment!=0,
                      selected_delta,&selected_length2,&selected_projection))return dfr_fail(probe,16,segment,0,0xffffffffu,a,z,ba,bz,(const float[]){hv.motion[0],hv.motion[1],hv.motion[2],hv.pending[0],hv.pending[1],hv.pending[2],hv.up[0],hv.up[1],hv.up[2],hv.target_up[0],hv.target_up[1],hv.target_up[2]},12);
        if(!(hf_edge(selected_delta,selected_length2,&hv)||(padding_only&&spr_edge(selected_delta,selected_length2,&hv))||(segment&&sfp_walk_edge((void *)a,(void *)z,selected_delta,selected_length2,&hv,1,lab_snaking_config.alignment_squared))))return dfr_fail(probe,17,segment,0,0xffffffffu,a,z,ba,bz,(const float[]){selected_delta[0],selected_delta[1],selected_delta[2],selected_length2,selected_projection,speed,required,hv.motion[0],hv.motion[1],hv.motion[2],hv.pending[0],hv.pending[1],hv.pending[2],hv.up[0],hv.up[1],hv.up[2],hv.target_up[0],hv.target_up[1],hv.target_up[2]},19);
        for(u32 i=0;i<3;i++)selected_dot+=selected_delta[i]*heading[i];
        if(selected_dot<=0||(!segment&&selected_dot*selected_dot<lab_snaking_config.alignment_squared*selected_length2*norm))return dfr_fail(probe,18,segment,0,0xffffffffu,a,z,ba,bz,(const float[]){selected_dot,selected_length2,norm,speed,required},5);
        float selected_length=root(selected_length2);
        selected_remaining+=!segment?selected_length-selected_projection/selected_length:selected_length;
        float delta[3],length2=0,dot=0,width=10000.f;
        for(u32 j=0;j<2;j++){
            void *p=(void *)(j?bz:ba);float radius=f(p,0x24);
            /* Retail 810E98..810F00 loads Scale and halves all components.
             * X is also the native drift-width gate at 7B6D14 (threshold 70). */
            if(!finite(radius)||radius<lab_snaking_config.minimum_width||radius>10000)return dfr_fail(probe,19,segment,1,j,a,z,ba,bz,(const float[]){radius,lab_snaking_config.minimum_width},2);
            if(radius<width)width=radius;
        }
        for(u32 i=0;i<3;i++){
            float start=f((void *)ba,0xc+4*i),end=f((void *)bz,0xc+4*i);
            if(!finite(start)||!finite(end))return dfr_fail(probe,20,segment,1,i,a,z,ba,bz,(const float[]){start,end},2);
            delta[i]=end-start;length2+=delta[i]*delta[i];dot+=delta[i]*heading[i];
        }
        if(!(hf_edge(delta,length2,&hv)||(padding_only&&spr_edge(delta,length2,&hv))||(segment&&sfp_walk_edge((void *)ba,(void *)bz,delta,length2,&hv,1,lab_snaking_config.alignment_squared))))return dfr_fail(probe,21,segment,1,0xffffffffu,a,z,ba,bz,(const float[]){delta[0],delta[1],delta[2],length2,speed,required,hv.motion[0],hv.motion[1],hv.motion[2],hv.pending[0],hv.pending[1],hv.pending[2],hv.up[0],hv.up[1],hv.up[2],hv.target_up[0],hv.target_up[1],hv.target_up[2]},18);
        float normal_delta=0;for(u32 i=0;i<3;i++)normal_delta+=delta[i]*up[i];
        if(!finite(length2)||length2<64||dot<=0||(!segment&&dot*dot<lab_snaking_config.alignment_squared*length2*norm)||
           (!segment&&normal_delta*normal_delta>.0025f*length2*up2))return dfr_fail(probe,22,segment,1,0xffffffffu,a,z,ba,bz,(const float[]){length2,dot,norm,normal_delta,up2,speed,required},7);
        float length=root(length2);
        if(!segment){
            float offset[3],projection=0,error=0;
            for(u32 i=0;i<3;i++){
                float position=f(physics,0x2c+4*i);if(!finite(position))return dfr_fail(probe,23,segment,1,i,a,z,ba,bz,(const float[]){position},1);
                offset[i]=position-f((void *)ba,0xc+4*i);projection+=offset[i]*delta[i];
            }
            u32 base_predecessor=projection<0,base_successor=projection>length2;
            float prior_remaining=0,base_debt=0;
            if((base_predecessor&&!gbp_occupation(physics,route,course,(void *)native_base,
                a,z,ba,bz,delta,length2,projection,width,speed,&hv,&prior_remaining))||
               (base_successor&&!hbs_occupation(physics,route,course,(void *)native_base,
                a,z,ba,bz,delta,length2,projection,speed,&hv,&base_debt)))
                return dfr_fail(probe,24,segment,1,0xffffffffu,a,z,ba,bz,(const float[]){projection,length2,speed,required},4);
            for(u32 i=0;i<3;i++){float e=offset[i]-projection/length2*delta[i];error+=e*e;}
            float margin=lab_snaking_config.margin+2.f*speed;
            if(width<=margin||!finite(error)||error>=(width-margin)*(width-margin))return dfr_fail(probe,25,segment,1,0xffffffffu,a,z,ba,bz,(const float[]){width,margin,error,projection,length2,speed,required},7);
            u32 hnp_detail=0;float hnp_observed[4]={0};
            if(!base_predecessor&&!base_successor&&!hnb_projection(physics,route,course,(void *)native_base,a,z,ba,bz,delta,length2,width,speed,&hv,&hnp_detail,hnp_observed))return dfr_fail(probe,26,segment,1,hnp_detail,a,z,ba,bz,(const float[]){projection,length2,width,speed,required,delta[0],delta[1],delta[2],hv.motion[0],hv.motion[1],hv.motion[2],hv.pending[0],hv.pending[1],hv.pending[2],hv.up[0],hv.up[1],hv.up[2],hv.target_up[0],hv.target_up[1],hv.target_up[2],hnp_observed[0],hnp_observed[1],hnp_observed[2],hnp_observed[3]},24);
            remaining=base_predecessor?length+prior_remaining:(base_successor?-base_debt:length-projection/length);
        }else remaining+=length;
        *last=z;
        if(remaining>=required&&selected_remaining>=required){if(reserved)*reserved=required;return 1;}
        u32 key;if(!successor((void *)z,&key))return dfr_fail(probe,27,segment,0xffffffffu,0xffffffffu,a,z,ba,bz,(const float[]){remaining,selected_remaining,required,speed},4);
        a=z;z=be_point(course,key);if(!z)return dfr_fail(probe,28,segment,0xffffffffu,0xffffffffu,a,z,0,0,(const float[]){remaining,selected_remaining,required,speed},4);
    }
    return dfr_fail(probe,29,0xffffffffu,0xffffffffu,0xffffffffu,a,z,0,0,(const float[]){remaining,selected_remaining,required,speed},4);
}

#include "snaking_ground_route_recovery_generated_v305.h"
#include "snaking_hop_route_recovery_v305.h"

/* Private experiment; no native object, charge, role or ratio writes. */
EXPORT const struct {u32 abi,capacity,max_grants;float alignment_squared;}
lab_snaking_ground_window_config={1,12,8,.90f};
typedef struct {
    u64 driver,producer,clock;
    u32 start,episode,grants,last;
} GroundWindow;
_Static_assert(sizeof(GroundWindow)==40,"Ground window record");
EXPORT GroundWindow lab_snaking_ground_windows[12];

static u32 gw_edge(const float *delta,float length2,const HopVectors *v){
    if(!finite(length2)||length2<64.f)return 0;
    float a=hf_dot(delta,v->motion),z=hf_dot(delta,v->pending);
    float n=hf_dot(delta,v->up),t=hf_dot(delta,v->target_up);
    return finite(a)&&finite(z)&&finite(n)&&finite(t)&&a>0&&z>0&&
           a*a>=lab_snaking_ground_window_config.alignment_squared*length2&&
           z*z>=lab_snaking_ground_window_config.alignment_squared*length2&&
           n*n<=.0025f*length2&&t*t<=.0025f*length2;
}

#include "snaking_ground_window_occupation_v305.h"
EXPORT const u32 lab_snaking_ground_node_window_config[4]={1,2,4,8};
static u32 gwn_projection(const void *physics,const void *route,void *course,void *native_base,
    u64 a,u64 z,u64 ba,u64 bz,const float *delta,float length2,float width,float speed,const HopVectors *v,u32 *__restrict detail,float *__restrict observed){
    u32 root_detail=0;
    if(hf_projection(physics,(void *)ba,delta,length2,width,speed,v,&root_detail,observed))return 1;
    *detail=root_detail<<16;
    float offset[3],projection=0;
    for(u32 i=0;i<3;i++){
        offset[i]=f(physics,0x2c+4*i)-f((void *)ba,0xc+4*i);
        if(!finite(offset[i]))return hnp_refused(detail,1,255);projection+=offset[i]*delta[i];
    }
    float travel=4.f*speed,clearance=width-lab_snaking_config.margin-2.f*speed;
    if(!finite(projection)||projection<0||projection>length2||clearance<=0)return hnp_refused(detail,2,255);
    u32 crosses=0;
    for(u32 mode=0;mode<2;mode++){
        float advance=hf_dot(mode?v->pending:v->motion,delta);
        if(!finite(advance)||advance<=0)return hnp_refused(detail,3,mode);
        if(projection+travel*advance>=length2)crosses=1;
    }
    if(!crosses)return hnp_refused(detail,4,255);
    /* Use only the cache successor for this occupied edge, with exact matching
     * singleton topology in selected and base views. No path search. */
    u64 following=z==q(route,0x60)?q(route,0x58):q(route,0x50);
    u32 key;
    if(!ptr(following)||following==a||following==z)return hnp_refused(detail,5,255);
    if(!successor((void *)z,&key))return hnp_refused(detail,6,255);
    if(be_point(course,key)!=following)return hnp_refused(detail,7,255);
    if(w((void *)following,0x8c)!=key)return hnp_refused(detail,8,255);
    u64 bf=be_point(native_base,key);
    if(!bf||bf==ba||bf==bz)return hnp_refused(detail,9,255);
    if(!be_edge((void *)bz,key))return hnp_refused(detail,10,255);
    float sd[3],sd2,sp,nsd[3],nsd2,nsp,nd[3],nd2,np;
    if(!hf_intent(physics,(void *)a,(void *)z,v,1,sd,&sd2,&sp))return hnp_refused(detail,11,255);
    if(!hf_intent(physics,(void *)z,(void *)following,v,0,nsd,&nsd2,&nsp))return hnp_refused(detail,12,255);
    if(!hf_intent(physics,(void *)bz,(void *)bf,v,0,nd,&nd2,&np))return hnp_refused(detail,13,255);
    if(!gw_edge(sd,sd2,v))return hnp_refused(detail,14,255);
    if(!hnb_future_edge(nsd,nsd2,nsp,v))return hnp_refused(detail,15,255);
    if(!hnb_future_edge(nd,nd2,np,v))return hnp_refused(detail,16,255);
    if(!hnb_straight(sd,sd2,nsd,nsd2))return hnp_refused(detail,17,255);
    if(!hnb_straight(delta,length2,nd,nd2))return hnp_refused(detail,18,255);
    float next_width=f((void *)bz,0x24),r=f((void *)bf,0x24);
    if(!finite(next_width)||!finite(r)||next_width<lab_snaking_config.minimum_width||
       r<lab_snaking_config.minimum_width||next_width>10000||r>10000)return hnp_refused(detail,19,255);
    if(r<next_width)next_width=r;
    float next_clearance=next_width-lab_snaking_config.margin-2.f*speed;
    if(next_clearance<=0)return hnp_refused(detail,20,255);
    float next_offset[3],selected_offset[3],next_selected_offset[3];
    for(u32 i=0;i<3;i++){
        next_offset[i]=f(physics,0x2c+4*i)-f((void *)bz,0xc+4*i);
        selected_offset[i]=f(physics,0x2c+4*i)-f((void *)a,0xc+4*i);
        next_selected_offset[i]=f(physics,0x2c+4*i)-f((void *)z,0xc+4*i);
        if(!finite(next_offset[i])||!finite(selected_offset[i])||!finite(next_selected_offset[i]))return hnp_refused(detail,21,255);
    }
    for(u32 mode=0;mode<2;mode++){
        const float *axis=mode?v->pending:v->motion;
        HnbInterval first_base,second_base,first_selected,second_selected;
        if(!hnb_interval(delta,length2,projection,offset,axis,clearance,0.f,1,&first_base))return hnp_refused(detail,22,mode);
        if(!hnb_interval(nd,nd2,np,next_offset,axis,next_clearance,0.f,1,&second_base))return hnp_refused(detail,23,mode);
        if(!hnb_interval(sd,sd2,sp,selected_offset,axis,0.f,0.f,0,&first_selected))return hnp_refused(detail,24,mode);
        if(!hnb_interval(nsd,nsd2,nsp,next_selected_offset,axis,0.f,0.f,0,&second_selected))return hnp_refused(detail,25,mode);
        if(!hnb_selected_join(sd,sd2,nsd,nsd2,next_selected_offset,axis,&first_selected,&second_selected))return hnp_refused(detail,26,mode);
        if(!hnc_cover(first_base,second_base,travel,next_offset,axis,clearance,next_clearance))return hnp_refused(detail,27,mode);
        if(!lab_snaking_selected_boundary_cover(first_selected,second_selected,travel))return hnp_refused(detail,28,mode);
    }
    return 1;
}

static u32 gw_corridor_inner(void *driver,void *physics,void *route,u32 ticks,u64 previous_end,u64 *last,u32 *old_ahead,float *reserved,const SnakeHopFrame *frame,DfrProbe *probe){
    u64 holder=q(route,0x48),a=q(route,0x60),z=q(route,0x58);
    if(!ptr(holder)||!ptr(a)||!ptr(z)||a==z)return dfr_fail(probe,1,0xffffffffu,0xffffffffu,0xffffffffu,0,0,0,0,(const float *)0,0);
    u64 c=q((void *)holder,0);if(!ptr(c))return dfr_fail(probe,2,0xffffffffu,0xffffffffu,0xffffffffu,a,z,0,0,(const float *)0,0);
    void *course=(void *)c;
    u64 native_base=be_course((u64)lab_module_start-LAB_MAIN_TO_MODULE,driver,route);
    if(!native_base||native_base!=frame->base_course||c!=frame->course||(u64)physics!=frame->physics||q(physics,0x110)!=frame->drift)return dfr_fail(probe,3,0xffffffffu,0xffffffffu,0xffffffffu,a,z,0,0,(const float *)0,0);
    if(be_point(course,w((void *)z,0x8c))!=z||be_point(course,w((void *)a,0x8c))!=a)return dfr_fail(probe,4,0xffffffffu,0xffffffffu,0xffffffffu,a,z,0,0,(const float *)0,0);
    u32 approach_key;
    if(!successor((void *)a,&approach_key)||be_point(course,approach_key)!=z)return dfr_fail(probe,5,0xffffffffu,0xffffffffu,0xffffffffu,a,z,0,0,(const float *)0,0);
    float speed=f(physics,0x37c);
    if(!finite(speed)||speed<=2.f)return dfr_fail(probe,6,0xffffffffu,0xffffffffu,0xffffffffu,a,z,0,0,(const float[]){speed},1);
    for(u32 o=0x424;o<=0x430;o+=4){float v=f(physics,o);if(!finite(v)||v<0)return dfr_fail(probe,7,0xffffffffu,0xffffffffu,o,a,z,0,0,(const float[]){speed,v},2);if(v>speed)speed=v;}
    if(speed>lab_snaking_config.maximum_speed)return dfr_fail(probe,8,0xffffffffu,0xffffffffu,0xffffffffu,a,z,0,0,(const float[]){speed,lab_snaking_config.maximum_speed},2);
    float required=speed*ticks+64.f,remaining=0,selected_remaining=0,heading[3],norm=0;
    probe->required_distance=dfr_bits(required);probe->required_distance_valid=1;
    HopVectors hv;if(!df_vectors(physics,frame,&hv))return dfr_fail(probe,9,0xffffffffu,0xffffffffu,0xffffffffu,a,z,0,0,(const float[]){speed,required},2);
    float up[3],up2=1.f;norm=1.f;
    for(u32 i=0;i<3;i++){heading[i]=hv.motion[i];up[i]=hv.up[i];}
    /* Cache age is not occupation. Keep the current selected edge valid,
     * then qualify at most its one cached predecessor before the normal walk. */
    float occupied_delta[3],occupied_length2,occupied_projection;
    if(!hf_intent(physics,(void *)a,(void *)z,&hv,0,
                  occupied_delta,&occupied_length2,&occupied_projection))return dfr_fail(probe,10,0xffffffffu,0,0xffffffffu,a,z,0,0,(const float[]){hv.motion[0],hv.motion[1],hv.motion[2],hv.pending[0],hv.pending[1],hv.pending[2],hv.up[0],hv.up[1],hv.up[2],hv.target_up[0],hv.target_up[1],hv.target_up[2]},12);
    if(occupied_projection<0){
        u64 older=q(route,0x68);
        if(!be_span(older,0x374)||older==a||older==z||
           be_point(course,w((void *)older,0x8c))!=older||
           !be_edge((void *)older,w((void *)a,0x8c)))return dfr_fail(probe,11,0xffffffffu,0,0xffffffffu,a,z,0,0,(const float[]){occupied_projection,occupied_length2},2);
        if(!hf_intent(physics,(void *)older,(void *)a,&hv,1,
                      occupied_delta,&occupied_length2,&occupied_projection))return dfr_fail(probe,12,0xffffffffu,0,0xffffffffu,older,a,0,0,(const float[]){hv.motion[0],hv.motion[1],hv.motion[2],hv.pending[0],hv.pending[1],hv.pending[2],hv.up[0],hv.up[1],hv.up[2],hv.target_up[0],hv.target_up[1],hv.target_up[2]},12);
        z=a;a=older;
    }else if(occupied_projection>occupied_length2)return dfr_fail(probe,13,0xffffffffu,0,0xffffffffu,a,z,0,0,(const float[]){occupied_projection,occupied_length2},2);
    u64 seen[18];seen[0]=a;u32 used=1;*old_ahead=0;
    for(u32 segment=0;segment<16;segment++){
        if(z==previous_end)*old_ahead=1;
        for(u32 j=0;j<used;j++)if(seen[j]==z)return dfr_fail(probe,14,segment,0xffffffffu,0xffffffffu,a,z,0,0,(const float *)0,0);
        seen[used++]=z;
        u64 ba=be_point((void *)native_base,w((void *)a,0x8c));
        u64 bz=be_point((void *)native_base,w((void *)z,0x8c));
        if(!ba||!bz||!be_edge((void *)ba,w((void *)bz,0x8c)))return dfr_fail(probe,15,segment,1,0xffffffffu,a,z,ba,bz,(const float *)0,0);
        float selected_delta[3],selected_length2,selected_projection,selected_dot=0;
        if(!sfp_walk_intent(physics,(void *)a,(void *)z,&hv,!segment,segment!=0,
                      selected_delta,&selected_length2,&selected_projection))return dfr_fail(probe,16,segment,0,0xffffffffu,a,z,ba,bz,(const float[]){hv.motion[0],hv.motion[1],hv.motion[2],hv.pending[0],hv.pending[1],hv.pending[2],hv.up[0],hv.up[1],hv.up[2],hv.target_up[0],hv.target_up[1],hv.target_up[2]},12);
        if(!(gw_edge(selected_delta,selected_length2,&hv)||(segment&&sfp_walk_edge((void *)a,(void *)z,selected_delta,selected_length2,&hv,1,lab_snaking_ground_window_config.alignment_squared))))return dfr_fail(probe,17,segment,0,0xffffffffu,a,z,ba,bz,(const float[]){selected_delta[0],selected_delta[1],selected_delta[2],selected_length2,selected_projection,speed,required,hv.motion[0],hv.motion[1],hv.motion[2],hv.pending[0],hv.pending[1],hv.pending[2],hv.up[0],hv.up[1],hv.up[2],hv.target_up[0],hv.target_up[1],hv.target_up[2]},19);
        for(u32 i=0;i<3;i++)selected_dot+=selected_delta[i]*heading[i];
        if(selected_dot<=0||(!segment&&selected_dot*selected_dot<lab_snaking_ground_window_config.alignment_squared*selected_length2*norm))return dfr_fail(probe,18,segment,0,0xffffffffu,a,z,ba,bz,(const float[]){selected_dot,selected_length2,norm,speed,required},5);
        float selected_length=root(selected_length2);
        selected_remaining+=!segment?selected_length-selected_projection/selected_length:selected_length;
        float delta[3],length2=0,dot=0,width=10000.f;
        for(u32 j=0;j<2;j++){
            void *p=(void *)(j?bz:ba);float radius=f(p,0x24);
            /* Retail 810E98..810F00 loads Scale and halves all components.
             * X is also the native drift-width gate at 7B6D14 (threshold 70). */
            if(!finite(radius)||radius<lab_snaking_config.minimum_width||radius>10000)return dfr_fail(probe,19,segment,1,j,a,z,ba,bz,(const float[]){radius,lab_snaking_config.minimum_width},2);
            if(radius<width)width=radius;
        }
        for(u32 i=0;i<3;i++){
            float start=f((void *)ba,0xc+4*i),end=f((void *)bz,0xc+4*i);
            if(!finite(start)||!finite(end))return dfr_fail(probe,20,segment,1,i,a,z,ba,bz,(const float[]){start,end},2);
            delta[i]=end-start;length2+=delta[i]*delta[i];dot+=delta[i]*heading[i];
        }
        if(!(gw_edge(delta,length2,&hv)||(segment&&sfp_walk_edge((void *)ba,(void *)bz,delta,length2,&hv,1,lab_snaking_ground_window_config.alignment_squared))))return dfr_fail(probe,21,segment,1,0xffffffffu,a,z,ba,bz,(const float[]){delta[0],delta[1],delta[2],length2,speed,required,hv.motion[0],hv.motion[1],hv.motion[2],hv.pending[0],hv.pending[1],hv.pending[2],hv.up[0],hv.up[1],hv.up[2],hv.target_up[0],hv.target_up[1],hv.target_up[2]},18);
        float normal_delta=0;for(u32 i=0;i<3;i++)normal_delta+=delta[i]*up[i];
        if(!finite(length2)||length2<64||dot<=0||(!segment&&dot*dot<lab_snaking_ground_window_config.alignment_squared*length2*norm)||
           (!segment&&normal_delta*normal_delta>.0025f*length2*up2))return dfr_fail(probe,22,segment,1,0xffffffffu,a,z,ba,bz,(const float[]){length2,dot,norm,normal_delta,up2,speed,required},7);
        float length=root(length2);
        if(!segment){
            float offset[3],projection=0,error=0;
            for(u32 i=0;i<3;i++){
                float position=f(physics,0x2c+4*i);if(!finite(position))return dfr_fail(probe,23,segment,1,i,a,z,ba,bz,(const float[]){position},1);
                offset[i]=position-f((void *)ba,0xc+4*i);projection+=offset[i]*delta[i];
            }
            u32 base_predecessor=projection<0,base_successor=projection>length2;
            float prior_remaining=0,base_debt=0;
            if((base_predecessor&&!gwp_occupation(physics,route,course,(void *)native_base,
                a,z,ba,bz,delta,length2,projection,width,speed,&hv,&prior_remaining))||
               (base_successor&&!gws_occupation(physics,route,course,(void *)native_base,
                a,z,ba,bz,delta,length2,projection,speed,&hv,&base_debt)))
                return dfr_fail(probe,24,segment,1,0xffffffffu,a,z,ba,bz,(const float[]){projection,length2,speed,required},4);
            for(u32 i=0;i<3;i++){float e=offset[i]-projection/length2*delta[i];error+=e*e;}
            float margin=lab_snaking_config.margin+2.f*speed;
            if(width<=margin||!finite(error)||error>=(width-margin)*(width-margin))return dfr_fail(probe,25,segment,1,0xffffffffu,a,z,ba,bz,(const float[]){width,margin,error,projection,length2,speed,required},7);
            u32 hnp_detail=0;float hnp_observed[4]={0};
            if(!base_predecessor&&!base_successor&&!gwn_projection(physics,route,course,(void *)native_base,a,z,ba,bz,delta,length2,width,speed,&hv,&hnp_detail,hnp_observed))return dfr_fail(probe,26,segment,1,hnp_detail,a,z,ba,bz,(const float[]){projection,length2,width,speed,required,delta[0],delta[1],delta[2],hv.motion[0],hv.motion[1],hv.motion[2],hv.pending[0],hv.pending[1],hv.pending[2],hv.up[0],hv.up[1],hv.up[2],hv.target_up[0],hv.target_up[1],hv.target_up[2],hnp_observed[0],hnp_observed[1],hnp_observed[2],hnp_observed[3]},24);
            remaining=base_predecessor?length+prior_remaining:(base_successor?-base_debt:length-projection/length);
        }else remaining+=length;
        *last=z;
        if(remaining>=required&&selected_remaining>=required){if(reserved)*reserved=required;return 1;}
        u32 key;if(!successor((void *)z,&key))return dfr_fail(probe,27,segment,0xffffffffu,0xffffffffu,a,z,ba,bz,(const float[]){remaining,selected_remaining,required,speed},4);
        a=z;z=be_point(course,key);if(!z)return dfr_fail(probe,28,segment,0xffffffffu,0xffffffffu,a,z,0,0,(const float[]){remaining,selected_remaining,required,speed},4);
    }
    return dfr_fail(probe,29,0xffffffffu,0xffffffffu,0xffffffffu,a,z,0,0,(const float[]){remaining,selected_remaining,required,speed},4);
}

EXPORT const u32 lab_snaking_ground_fallback_trace_config[4]={1,320,316,0x47570000u};
static u32 gw_continue(void *driver,void *physics,void *route,u32 ticks,
    u64 previous_end,u64 *last,u32 *old_ahead,float *reserved,
    const SnakeHopFrame *frame,DfrProbe *probe){
    if(probe->segment||(probe->stage!=17&&probe->stage!=21))return 0;
    u64 index=(u64)frame-(u64)lab_snaking_drift_frames;
    if(index>=sizeof(lab_snaking_drift_frames)||index%sizeof(SnakeHopFrame))return 0;
    u32 racer=index/sizeof(SnakeHopFrame);
    const SnakeState *s=&lab_snaking_state[racer];
    u64 drift=q(physics,0x110);
    if(!ptr(drift)||s->lock!=1||s->driver!=(u64)driver||
       !ptr(s->clock)||w((void *)s->clock,0x38)!=6||
       w((void *)s->clock,0x44)!=s->last||
       !df_scope(frame,s,physics,(void *)drift,s->last,w((void *)drift,0x10)))return 0;
    GroundWindow *r=&lab_snaking_ground_windows[racer];
    u32 same=r->driver==s->driver&&r->producer==s->producer&&r->clock==s->clock&&
             r->start==s->start&&r->episode==s->episodes;
    if(same&&(r->grants>=lab_snaking_ground_window_config.max_grants||s->last<=r->last))return 0;
    DfrProbe supplemental={0};
    if(!gw_corridor_inner(driver,physics,route,ticks,previous_end,last,old_ahead,
                         reserved,frame,&supplemental)){
        if(supplemental.stage){
            u32 primary_stage=probe->stage;
            *probe=supplemental;
            probe->fallback=0x47570000u|primary_stage;
        }
        return 0;
    }
    if(!same){
        r->driver=s->driver;r->producer=s->producer;r->clock=s->clock;
        r->start=s->start;r->episode=s->episodes;r->grants=0;
    }
    r->grants++;r->last=s->last;
    return 1;
}

static u32 df_corridor(void *driver,void *physics,void *route,u32 ticks,u64 previous_end,u64 *last,u32 *old_ahead,float *reserved,const SnakeHopFrame *frame,const DfrContext *context){
    DfrProbe probe={0};
    u32 accepted=df_corridor_diag_inner(driver,physics,route,ticks,previous_end,last,old_ahead,reserved,frame,&probe);
    if(!accepted)accepted=sgr_continue(driver,physics,route,ticks,frame,&probe);
    if(!accepted)accepted=gw_continue(driver,physics,route,ticks,previous_end,last,old_ahead,reserved,frame,&probe);
    if(!accepted)dfr_publish(&probe,frame,physics,route,ticks,context);
    return accepted;
}

static void finish(SnakeState *s,u32 reason){s->reason=reason;__atomic_store_n(&s->lock,0,__ATOMIC_RELEASE);}
/* Private passive ABI1. Inserted before the exact abort_episode definition.
 * Payload comes only from its already locked SnakeState and scalar arguments.
 * No native object, movement packet, clock or gameplay state is written here. */
typedef struct {
    u32 lock,fault;
    u64 sequence,events,dropped;
    u64 driver,producer,clock;
    u32 tick,start,phase,reason;
    i32 direction;
    u32 episodes,aborts_before,racer;
    u32 reasons[16];
    u32 flags,native_state,drift_state,reserved;
} SnakeAbortTrace;
_Static_assert(sizeof(SnakeAbortTrace)==168,"Snaking abort trace ABI");
_Static_assert(__builtin_offsetof(SnakeAbortTrace,driver)==32,"Abort trace owner offset");
_Static_assert(__builtin_offsetof(SnakeAbortTrace,tick)==56,"Abort trace tick offset");
_Static_assert(__builtin_offsetof(SnakeAbortTrace,reasons)==88,"Abort trace counter offset");
EXPORT const u32 lab_snaking_abort_trace_config[6]={1,168,12,96,16,1};
EXPORT SnakeAbortTrace lab_snaking_abort_trace_records[12];


/* Private passive hop-decision publication. Called only after an original
 * active corridor refusal, with already evaluated scope/result scalars. */
typedef struct {
    u32 lock,fault;
    u64 sequence,events,dropped;
    u64 driver,producer,clock;
    u32 tick,start,phase,drift_state,racer,episodes;
    u32 original,scope,accepted,reserved;
    u64 eligible,accepted_count,scope_refused,geometry_refused;
} HopAbortUse;
_Static_assert(sizeof(HopAbortUse)==128,"Hop abort use ABI1");
_Static_assert(__builtin_offsetof(HopAbortUse,eligible)==96,"Hop abort use counters");
EXPORT const u32 lab_snaking_hop_abort_use_config[4]={1,128,12,96};
EXPORT HopAbortUse lab_snaking_hop_abort_use_records[12];
static void ha_publish(SnakeState *s,u32 now,u32 ds,u32 scope,u32 accepted){
    u64 delta=(u64)s-(u64)lab_snaking_state;
    if(delta>=sizeof(lab_snaking_state)||delta%sizeof(SnakeState))return;
    u32 racer=(u32)(delta/sizeof(SnakeState));
    HopAbortUse *r=&lab_snaking_hop_abort_use_records[racer];u32 expected=0;
    if(__atomic_compare_exchange_n(&r->lock,&expected,1,1,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED)){
        u64 seq=r->sequence;
        if((seq&1)||seq>~0ull-2||r->events==~0ull){r->fault|=1;}
        else{
            __atomic_store_n(&r->sequence,seq+1,__ATOMIC_RELEASE);
            __atomic_thread_fence(__ATOMIC_RELEASE);
            r->driver=s->driver;r->producer=s->producer;r->clock=s->clock;
            r->tick=now;r->start=s->start;r->phase=s->phase;r->drift_state=ds;
            r->racer=racer;r->episodes=s->episodes;r->original=0;
            r->scope=scope;r->accepted=accepted;r->reserved=0;
            if(scope){r->eligible++;if(accepted)r->accepted_count++;else r->geometry_refused++;}
            else r->scope_refused++;
            r->events++;
            __atomic_store_n(&r->sequence,seq+2,__ATOMIC_RELEASE);
        }
        __atomic_store_n(&r->lock,0,__ATOMIC_RELEASE);
    }else{
        u64 dropped=__atomic_load_n(&r->dropped,__ATOMIC_RELAXED);
        if(dropped!=~0ull)__atomic_compare_exchange_n(&r->dropped,&dropped,dropped+1,1,__ATOMIC_RELAXED,__ATOMIC_RELAXED);
    }
}

static void abort_episode(SnakeState *s,u32 now,u32 cooldown,u32 reason,u32 trace_flags,u32 trace_native_state,u32 trace_drift_state){
    if(s->phase){
        /* Unsigned range/alignment checks derive the index from the owned
         * private array. Never turn an arbitrary s into a diagnostic address. */
        u64 trace_delta=(u64)s-(u64)lab_snaking_state;
        if(trace_delta<sizeof(lab_snaking_state)&&!(trace_delta%sizeof(SnakeState))&&reason<16){
            u32 trace_racer=(u32)(trace_delta/sizeof(SnakeState));
            SnakeAbortTrace *trace=&lab_snaking_abort_trace_records[trace_racer];
            u32 trace_expected=0;
            /* A single weak CAS may drop. It never waits for diagnostic work. */
            if(__atomic_compare_exchange_n(&trace->lock,&trace_expected,1,1,
                                          __ATOMIC_ACQUIRE,__ATOMIC_RELAXED)){
                u64 trace_seq=trace->sequence;
                if((trace_seq&1)||trace_seq>~0ull-2||trace->events==~0ull){
                    trace->fault|=1;
                }else{
                    __atomic_store_n(&trace->sequence,trace_seq+1,__ATOMIC_RELEASE);
                    __atomic_thread_fence(__ATOMIC_RELEASE);
                    trace->driver=s->driver;trace->producer=s->producer;trace->clock=s->clock;
                    trace->tick=now;trace->start=s->start;trace->phase=s->phase;
                    trace->reason=reason;trace->direction=s->direction;
                    trace->episodes=s->episodes;trace->aborts_before=s->aborts;
                    trace->racer=trace_racer;
                    trace->flags=trace_flags;trace->native_state=trace_native_state;
                    trace->drift_state=trace_drift_state;
                    if(trace->reasons[reason]!=~0u)trace->reasons[reason]++;
                    else trace->fault|=2;
                    trace->events++;
                    __atomic_store_n(&trace->sequence,trace_seq+2,__ATOMIC_RELEASE);
                }
                __atomic_store_n(&trace->lock,0,__ATOMIC_RELEASE);
            }else{
                u64 trace_dropped=__atomic_load_n(&trace->dropped,__ATOMIC_RELAXED);
                if(trace_dropped!=~0ull)
                    __atomic_compare_exchange_n(&trace->dropped,&trace_dropped,trace_dropped+1,1,
                                                __ATOMIC_RELAXED,__ATOMIC_RELAXED);
            }
        }
        s->aborts++;s->next_tick=now+cooldown;}
    s->phase=0;finish(s,reason);
}

/* Native admission reads only these producer fields (5C210..5C35C).
 * Use snaking cooldown and ordinary straight admission in stack storage; native object untouched. */
EXPORT const u32 lab_snaking_single_blue_config[5]={1,1,30,6,80};
EXPORT const struct {u32 abi,preserve_d9;float ordinary[3],straight;}
lab_snaking_straight_admission_config={1,1,{.96f,.979f,.998f},1.f};
EXPORT float lab_snaking_admission_limit(void *role,float original){
    if(!b(role,0xd9)&&(original==.96f||original==.979f||original==.998f))return 1.f;
    return original;
}
#include "snaking_settled_admission_v305.h"
static u32 single_blue_ready(u64 base,void *producer,void *role){
    if(w(producer,0x144)<30)return 0;
    u64 shadow[42];for(u32 i=0;i<42;i++)shadow[i]=0;
    shadow[0x70/8]=q(producer,0x70);shadow[0x80/8]=q(producer,0x80);
    u64 other_shadow[27];
    shadow[0x90/8]=lab_snaking_settled_other(base,producer,role,other_shadow);
    sf(shadow,0xcc,lab_snaking_admission_limit(role,f(producer,0xcc)));sf(shadow,0x134,f(producer,0x134));
    sw(shadow,0x144,180);
    return ((u32 (*)(void *))(base+0x5c210))(shadow);
}
static u32 admissible(u64 base,void *producer,void *role){
    u64 eligibility=q(producer,0x80),route_flags=q(producer,0x90);
    return ptr(eligibility)&&ptr(route_flags)&&ptr(q((void *)eligibility,0x68))&&
           ptr(q((void *)eligibility,0x70))&&ptr(q(producer,0x68))&&
           single_blue_ready(base,producer,role);
}

/* This shadow contains exactly the fields consumed by native 5D468. Native
 * countersteering writes the owned packet and this private phase storage. */
static void steer(u64 base,void *producer,void *packet,SnakeState *s,u32 elapsed){
    u64 shadow[44];
    for(u32 i=0;i<44;i++)((volatile u64 *)shadow)[i]=0;
    shadow[0x70/8]=q(producer,0x70);shadow[0x68/8]=q(producer,0x68);
    shadow[0x98/8]=(u64)packet;
    sw(shadow,0x44,elapsed);sf(shadow,0xa8,(float)s->direction);
    sw(shadow,0x150,s->steer_count);sw(shadow,0x154,s->steer_period);
    ((void (*)(void *))(base+0x5d468))(shadow);
    s->steer_count=w(shadow,0x150);s->steer_period=w(shadow,0x154);
}

EXPORT const u32 lab_snaking_manual_handoff_config[3]={1,0x80,15};
#include "snaking_entry_choice_v305.h"
#include "snaking_scope_retirement_v305.h"
#include "snaking_success_recheck_v305.h"
EXPORT void lab_snaking_dispatch(void *producer){
    u64 base=(u64)lab_module_start-LAB_MAIN_TO_MODULE;
    if((base&4095)||base>=(1ull<<39)-0x1400000||!ptr((u64)producer))return;
    u64 driver=q(producer,0x70),packet=q(producer,0x98),route=q(producer,0x88);
    if(!ptr(driver)||!ptr(packet)||!ptr(route))return;
    void *d=(void *)driver;u64 adapter=q(d,0x88),state=q(d,0x50);
    if(!ptr(adapter)||!ptr(state)||q((void *)state,0x88)!=driver)return;
    u64 nav=q((void *)state,0xb0),ratio=q((void *)state,0x90);
    if(!ptr(nav)||!ptr(ratio)||q((void *)nav,0x70)!=(u64)producer||q((void *)nav,0x78)!=packet||
       q((void *)nav,0x58)!=driver||q((void *)ratio,0x58)!=driver)return;
    u64 eligibility=q(producer,0x80);
    if(!ptr(eligibility)||q((void *)nav,0x60)!=eligibility||q((void *)eligibility,0x60)!=route)return;
    u64 role=q((void *)adapter,0x48),physics=q((void *)adapter,0x50),wrapper=q((void *)ratio,0x48);
    u32 solo=!wrapper && tts_admitted(base,(u64)producer);
    if(!ptr(role)||!ptr(physics)||(!solo&&(!ptr(wrapper)||q((void *)wrapper,8)!=driver))||
       q((void *)role,0x28)!=physics||q((void *)physics,0xf8)!=role)return;
    void *r=(void *)role,*p=(void *)physics;
    if(b(r,0xd2)!=1||b(r,0xd3)!=1||!b(r,0xe1)||b(r,0xe7))return;
    u32 racer=w(r,0xa8),slot=solo?0u:w((void *)wrapper,0x24);
    if(racer>=12||slot>=12)return;
    u64 rootptr=q((void *)base,0x12f6388);
    if(!ptr(rootptr))return;u64 single=q((void *)rootptr,0);
    if(!ptr(single))return;u64 link=q((void *)single,0x20);
    if(!ptr(link))return;u64 scene=q((void *)link,8);
    if(!ptr(scene)||!(b((void *)scene,0x180)&4))return;
    u64 system=q((void *)scene,0x1b0);
    if(!ptr(system))return;u64 race=q((void *)system,0x238),clocks=q((void *)system,0x218);
    if(!ptr(race)||!ptr(clocks))return;u64 clock=q((void *)clocks,0x50);
    if(!ptr(clock)||w((void *)clock,0x38)!=6)return;
    u64 group=q((void *)race,0xb0);
    if(!ptr(group)||q((void *)group,0x28)!=race||q(d,0x60)!=group)return;
    u32 count=w((void *)group,0x58);u64 drivers=q((void *)group,0x60);
    if(count>12||racer>=count||!ptr(drivers)||q((void *)drivers,8*racer)!=driver)return;
    u64 manager=q((void *)group,0x68),wrappers=0;
    if(solo){if(manager||q((void *)ratio,0x40))return;}
    else{
        if(!ptr(manager)||q((void *)ratio,0x40)!=manager)return;
        wrappers=q((void *)manager,0xa0);
        if(!ptr(wrappers)||q((void *)wrappers,8*racer)!=wrapper)return;
    }
    void *config=((void *(*)(void))(base+0x87c244))();
    if(!ptr((u64)config)||w(config,4)==1||(solo?w(config,8)!=2:w(config,8)>1))return;
    u64 drift=q(p,0x110);
    if(!ptr(drift)||q((void *)drift,8)!=role)return;
    u32 now=w((void *)clock,0x44),cooldown=lab_snaking_config.cooldown[slot],crowded=0;
    if(cooldown&&!solo)cooldown=pack_cooldown(wrapper,wrappers,drivers,group,count,racer,cooldown,&crowded);
    SnakeState *s=&lab_snaking_state[racer];u32 expected=0;
    if(!__atomic_compare_exchange_n(&s->lock,&expected,1,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED))return;
    if(s->driver!=driver||s->producer!=(u64)producer||s->clock!=clock||now<s->last){
        s->driver=driver;s->producer=(u64)producer;s->clock=clock;s->phase=0;s->previous_end=0;lab_snaking_reservations[racer].endpoint=0;
        s->first_side=(racer&1)?-1:1;s->next_tick=now+lab_snaking_config.initial_delay+17*racer;
    }
    u32 observed_dt=now-s->last;s->last=now;lab_snaking_update_reservation(&lab_snaking_reservations[racer],p,(void *)route,observed_dt);
    /* Native5BF5C..5BF9C normalizes movement for this external/manual source.
     * An already active private episode must not add R back at the epilogue.
     * Retire only our locked state; intentional handback is not a failed drift. */
    if(b((void *)eligibility,0x80)){
        s->phase=0;s->previous_end=0;lab_snaking_reservations[racer].endpoint=0;
        s->next_tick=now+(cooldown?cooldown:720);finish(s,15);return;
    }
    u32 flags=w((void *)packet,4),native_state=b(producer,0x41),ds=w((void *)drift,0x10);
    u32 scope_before=srt_mask(racer);
    hf_touch(&lab_snaking_hop_frames[racer],s,d,p,(void *)drift,now,observed_dt,ds);
    df_touch(&lab_snaking_drift_frames[racer],s,d,p,(void *)drift,now,observed_dt,ds);
    if(scope_before&~srt_mask(racer))lab_snaking_scope_trace(racer,s,p,d,observed_dt,ds,2,scope_before);
    if(!cooldown||(w(r,0x1cc)&0x01204018u)||native_state||(flags&10)||!(flags&1)){
        lab_snaking_reservations[racer].endpoint=0;abort_episode(s,now,cooldown?cooldown:720,1,flags,native_state,ds);return;
    }
    if(!s->phase){
        if((i32)(now-s->next_tick)<0){finish(s,2);return;}
        s->next_tick=now+6+racer%3;
        /* Let native driving handle nearby exchanges. Do not cancel an
         * already active drift just because another racer draws alongside. */
        if(crowded){finish(s,14);return;}
        if(ds||!b(p,0x201)||!finite(f((void *)drift,0x80))||f((void *)drift,0x80)<=0||f((void *)drift,0x80)>400){finish(s,3);return;}
        if(!admissible(base,producer,r)){finish(s,12);return;}
        u64 end=0;u32 old_ahead=0;float reserved_distance=0;
        if(!corridor(d,p,(void *)route,lab_snaking_corridor_ticks((void *)drift,ds,0),s->previous_end,&end,&old_ahead,&reserved_distance)||(old_ahead&&!lab_snaking_reservation_cleared(&lab_snaking_reservations[racer],p,(void *)route,s->previous_end))){finish(s,4);return;}
        i32 entry_direction=lab_snaking_entry_choice(d,p,producer,(void *)route,racer,now,s->first_side,reserved_distance);
        if(!entry_direction){finish(s,4);return;}
        lab_snaking_remember_reservation(&lab_snaking_reservations[racer],p,(void *)route,end,reserved_distance);s->previous_end=end;s->phase=1;s->start=now;s->direction=entry_direction;
        s->steer_count=0;s->steer_period=0;s->episodes++;
        hf_remember(&lab_snaking_hop_frames[racer],s,d,p,(void *)drift,(void *)route,now);
        df_remember(&lab_snaking_drift_frames[racer],&lab_snaking_hop_frames[racer]);
        lab_snaking_scope_trace(racer,s,p,d,observed_dt,ds,1,0);
    }
    u32 elapsed=now-s->start;
    if(s->phase==2||s->phase==4){
        if(ds&&ds!=4&&ds!=5){abort_episode(s,now,cooldown,5,flags,native_state,ds);return;}
        if(elapsed<lab_snaking_config.settle){finish(s,6);return;}
        if(s->phase==2||s->phase==4){s->phase=0;s->next_tick=now+lab_snaking_success_recheck(p,r,racer,cooldown);finish(s,7);return;}
        if(ds){abort_episode(s,now,cooldown,8,flags,native_state,ds);return;}
        if(!admissible(base,producer,r)){abort_episode(s,now,cooldown,13,flags,native_state,ds);return;}
        s->phase=3;s->direction=-s->direction;s->start=now;s->steer_count=0;s->steer_period=0;elapsed=0;
    }
    if(elapsed>=lab_snaking_config.leg_deadline || (ds!=0&&ds!=2&&ds!=(s->direction<0?4u:5u)) ||
       (elapsed>30&&ds!=4&&ds!=5)){abort_episode(s,now,cooldown,9,flags,native_state,ds);return;}
    DfrContext dfr_context={0};
    dfr_context.elapsed=elapsed;dfr_context.episode=s->episodes;dfr_context.direction=s->direction;
    dfr_context.packet_flags=flags;dfr_context.native_state=native_state;
    u32 remaining=dfr_corridor_ticks((void *)drift,ds,elapsed,&dfr_context);
    /* Each blue leg re-evaluates the route before another entry. */
    u64 end=0;u32 ignored=0;
    scope_before=srt_mask(racer);
    hf_qualify(&lab_snaking_hop_frames[racer],s,d,p,(void *)drift,(void *)route,now,ds);
    df_qualify(&lab_snaking_drift_frames[racer],s,d,p,(void *)drift,(void *)route,now,ds);
    if(scope_before&~srt_mask(racer))lab_snaking_scope_trace(racer,s,p,d,observed_dt,ds,3,scope_before);
    u32 ha_original=corridor(d,p,(void *)route,remaining,0,&end,&ignored,0);
    u32 ha_scope=0,ha_accepted=0;
    if(!ha_original){
        ha_scope=hf_scope(&lab_snaking_hop_frames[racer],s,p,(void *)drift,now,ds);
        if(ha_scope)ha_accepted=hf_corridor(d,p,(void *)route,remaining,0,&end,&ignored,0,&lab_snaking_hop_frames[racer]);
        ha_publish(s,now,ds,ha_scope,ha_accepted);
    }
    u32 df_accepted=0;
    if(!ha_original&&!ha_accepted&&df_scope(&lab_snaking_drift_frames[racer],s,p,(void *)drift,now,ds))
        df_accepted=df_corridor(d,p,(void *)route,remaining,0,&end,&ignored,0,&lab_snaking_drift_frames[racer],&dfr_context);
    if(!ha_original&&!ha_accepted&&!df_accepted){
        abort_episode(s,now,cooldown,10,flags,native_state,ds);return;
    }
    if((ds==4||ds==5)&&w((void *)drift,0x8c)>=1){
        s->phase++;s->start=now;s->release_requests++;finish(s,11);return;
    }
    sw((void *)packet,4,flags|8u);sf((void *)packet,8,(float)s->direction);
    steer(base,producer,(void *)packet,s,elapsed);finish(s,0);
}

/* Keep an owned, still-valid snaking leg on its current native view until its
 * release. Pace-mode changes between native views1..10 can wait for that
 * leg. AF shortcut/base, special C4/A4/A5 and the initial store retain priority.
 * Geometry remains the existing per-update corridor/BASE validation. */
EXPORT const u32 lab_snaking_route_hold_config[5]={1,1,5,80,0};

static void *srh_locked(void *driver,void *candidate,void *manager,u64 base,
                        SnakeState *s,u64 role,u64 physics,u32 racer){
    u64 current=q(driver,0x80),state=q(driver,0x50),group=q(driver,0x60);
    if(!ptr(current)||current==(u64)candidate||!ptr(state)||!ptr(group)||
       q((void *)group,0x50)!=(u64)manager||q(manager,0x38)!=group||
       b((void *)group,0xb0)||q((void *)state,0x88)!=(u64)driver)return candidate;
    u64 nav=q((void *)state,0xb0),race=q((void *)group,0x28);
    if(!ptr(nav)||!ptr(race)||q((void *)race,0xb0)!=group||
       q((void *)nav,0x58)!=(u64)driver)return candidate;
    u64 rootptr=q((void *)base,0x12f6388);
    if(!ptr(rootptr))return candidate;
    u64 single=q((void *)rootptr,0);
    if(!ptr(single))return candidate;
    u64 link=q((void *)single,0x20);
    if(!ptr(link))return candidate;
    u64 scene=q((void *)link,8);
    if(!ptr(scene)||!(b((void *)scene,0x180)&4))return candidate;
    u64 system=q((void *)scene,0x1b0);
    if(!ptr(system)||q((void *)system,0x238)!=race)return candidate;
    u64 clocks=q((void *)system,0x218);
    if(!ptr(clocks))return candidate;
    u64 clock=q((void *)clocks,0x50);
    if(!ptr(clock)||w((void *)clock,0x38)!=6)return candidate;
    u32 now=w((void *)clock,0x44);
    u64 producer=q((void *)nav,0x70),eligibility=q((void *)nav,0x60);
    if(!ptr(producer)||!ptr(eligibility)||q((void *)producer,0x70)!=(u64)driver||
       q((void *)producer,0x80)!=eligibility||b((void *)producer,0x41)||
       b((void *)eligibility,0x80)||s->driver!=(u64)driver||s->producer!=producer||
       s->clock!=clock||s->phase!=1||now<s->start||now-s->start>=80||
       now-s->start>=lab_snaking_config.leg_deadline||now<s->last||now-s->last>1||
       (s->direction!=-1&&s->direction!=1))return candidate;
    u64 route=q((void *)producer,0x88),drift=q((void *)physics,0x110);
    if(!ptr(route)||!ptr(drift)||q((void *)drift,8)!=role||
       q((void *)route,0x40)!=(u64)driver||q((void *)route,0x48)!=(u64)driver+0x80||
       q((void *)eligibility,0x60)!=route)return candidate;
    u32 ds=w((void *)drift,0x10),matching=s->direction<0?4u:5u;
    const SnakeHopFrame *frame=&lab_snaking_drift_frames[racer];
    if(frame->valid!=1||frame->driver!=(u64)driver||frame->producer!=producer||
       frame->clock!=clock||frame->start!=s->start||frame->last!=s->last||
       frame->physics!=physics||frame->drift!=drift||frame->course!=current||
       (ds!=2&&ds!=matching)||(ds==matching&&(!frame->reserved||
       !df_contact((void *)physics,frame))))return candidate;
    u64 vt=q(driver,0);
    if(!ptr(vt)||q((void *)vt,0x188)!=base+0x71ea4)return candidate;
    u32 alt=b(driver,0xd0)!=0&&q(manager,0x59a8)!=0;
    const void *head=(const u8 *)manager+(alt?0xb0:0x40);
    if(w(head,0)!=12||w(head,4)!=12)return candidate;
    u64 array=q(head,8);if(!ptr(array))return candidate;
    u64 views[12];u32 old=12,next=12;
    for(u32 i=0;i<12;i++){
        views[i]=q((void *)array,8*i);if(!ptr(views[i]))return candidate;
        if(views[i]==current)old=i;
        if(views[i]==(u64)candidate)next=i;
    }
    if(views[0]!=views[11]||old<1||old>10||next<1||next>10||
       frame->base_course!=views[0])return candidate;
    for(u32 i=1;i<=10;i++){
        if(views[i]==views[0])return candidate;
        for(u32 j=1;j<i;j++)if(views[i]==views[j])return candidate;
    }
    /* Course objects are followed only after current-array membership. */
    u32 groups=w((void *)current,0x10);
    if(!groups||groups>100||w(candidate,0x10)!=groups||
       b((void *)current,0x4c)!=alt||b(candidate,0x4c)!=alt||
       !ptr(q((void *)current,0x20))||!ptr(q(candidate,0x20)))return candidate;
    return (void *)current;
}

EXPORT void *lab_snaking_route_hold(void *driver,void *candidate,void *manager,u64 base){
    if(!ptr((u64)driver)||!ptr((u64)candidate)||!ptr((u64)manager)||
       b(driver,0xc4)||b(driver,0xa4)||b(driver,0xa5)||
       b(driver,0xaf))return candidate;
    u64 adapter=q(driver,0x88);
    if(!ptr(adapter))return candidate;
    u64 role=q((void *)adapter,0x48),physics=q((void *)adapter,0x50);
    if(!ptr(role)||!ptr(physics)||q((void *)role,0x28)!=physics||
       q((void *)physics,0xf8)!=role||b((void *)role,0xd0)||
       b((void *)role,0xd2)!=1||b((void *)role,0xd3)!=1||
       !b((void *)role,0xe1)||b((void *)role,0xe7)||
       (w((void *)role,0x1cc)&0x01204018u))return candidate;
    u64 ai=q((void *)role,0x18);
    if(!ptr(ai)||q(driver,0x90)!=ai||q((void *)ai,0x10)!=role||
       q((void *)physics,0x100)!=ai)return candidate;
    u32 racer=w((void *)role,0xa8);if(racer>=12)return candidate;
    SnakeState *s=&lab_snaking_state[racer];u32 expected=0;
    if(!__atomic_compare_exchange_n(&s->lock,&expected,1,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED))return candidate;
    void *result=srh_locked(driver,candidate,manager,base,s,role,physics,racer);
    __atomic_store_n(&s->lock,0,__ATOMIC_RELEASE);
    return result;
}

EXPORT const u32 lab_snaking_pace_route_hold_config[6]={1,1,10,80,1,1};
