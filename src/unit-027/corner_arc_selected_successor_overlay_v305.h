/* Private experimental corner lifetime plan. Native PID, input publication,
 * charge, tier and boost dispatch are untouched. These route tubes are not KCL.
 * Must be armed only at real native commitment and retired on native exit. */
EXPORT const u32 lab_corner_arc_config[9]={1,12,144,180,12,4,6,0x2000,24};
typedef struct {
    u64 producer,driver,role,physics,drift,route,course,base_course,clock;
    u32 start,last,counter,phase,target,deadline,lock,reason,offered,held;
    float charge,threshold[3],direction,usable,gain;
    u32 reserved;
} CornerArcState;
_Static_assert(sizeof(CornerArcState)==144,"Corner arc ABI1");
EXPORT CornerArcState lab_corner_arc_state[12];
typedef struct {
    u64 producer,driver,role,physics,drift,route,course,base_course,clock,packet;
    u32 racer,now,counter,ds,tier;
    float direction,charge,up[3],motion[2][3],speed;
} ArcOwner;
typedef struct {
    float delta[3],offset[3],length2,projection,radius;
} ArcEdge;
typedef struct {ArcEdge selected,base;u32 key;} ArcPair;
static float ca_dot(const float *a,const float *z){return a[0]*z[0]+a[1]*z[1]+a[2]*z[2];}
static u32 ca_unit(void *p,u32 o,float *v){
    float n=0;for(u32 i=0;i<3;i++){v[i]=f(p,o+4*i);if(!finite(v[i]))return 0;n+=v[i]*v[i];}
    if(!finite(n)||n<.9f||n>1.1f)return 0;n=root_positive(n);
    for(u32 i=0;i<3;i++)v[i]/=n;return 1;
}
static u32 ca_owner(void *producer,ArcOwner *c){
    if(!ptr((u64)producer))return 0;
    c->producer=(u64)producer;c->driver=q(producer,0x70);c->route=q(producer,0x88);c->packet=q(producer,0x98);
    if(!ptr(c->driver)||!ptr(c->route)||!ptr(c->packet))return 0;
    u64 adapter=q((void *)c->driver,0x88),state=q((void *)c->driver,0x50);
    if(!ptr(adapter)||!ptr(state)||q((void *)state,0x88)!=c->driver)return 0;
    u64 nav=q((void *)state,0xb0);
    if(!ptr(nav)||q((void *)nav,0x70)!=c->producer||q((void *)nav,0x58)!=c->driver||
       q((void *)nav,0x78)!=c->packet||q((void *)state,0xc0)!=c->packet)return 0;
    c->role=q((void *)adapter,0x48);c->physics=q((void *)adapter,0x50);
    if(!ptr(c->role)||!ptr(c->physics)||q((void *)c->role,0x28)!=c->physics||
       q((void *)c->physics,0xf8)!=c->role)return 0;
    c->racer=w((void *)c->role,0xa8);if(c->racer>=12)return 0;
    c->drift=q((void *)c->physics,0x110);
    if(!ptr(c->drift)||q((void *)c->drift,8)!=c->role)return 0;
    /* Manual/incompatible input never calls a course getter or walks route
     * storage, even when the legacy helper refuses. */
    if(b((void *)c->role,0xd2)!=1||b((void *)c->role,0xd3)!=1||
       b((void *)c->role,0xe1)!=1||b((void *)c->role,0xe7)||
       (w((void *)c->role,0x1cc)&0x01204018u)||b(producer,0x41)!=1)return 0;
    u64 main=(u64)lab_module_start-LAB_MAIN_TO_MODULE;
    c->base_course=be_course(main,(void *)c->driver,(void *)c->route);
    c->course=q((void *)c->driver,0x80);if(!c->base_course||!ptr(c->course))return 0;
    /* be_course verified the main scene/system/race chain immediately above. */
    u64 root=q((void *)main,0x12f6388),single=q((void *)root,0),link=q((void *)single,0x20);
    u64 scene=q((void *)link,8),system=q((void *)scene,0x1b0),clocks=q((void *)system,0x218);
    if(!ptr(clocks))return 0;c->clock=q((void *)clocks,0x50);
    if(!ptr(c->clock)||w((void *)c->clock,0x38)!=6)return 0;
    c->now=w((void *)c->clock,0x44);c->counter=w(producer,0x44);
    c->ds=w((void *)c->drift,0x10);c->tier=w((void *)c->drift,0x8c);
    c->charge=f((void *)c->drift,0x70);c->direction=f(producer,0xa8);
    return 1;
}
static u32 ca_driving(const ArcOwner *c){
    void *r=(void *)c->role,*p=(void *)c->physics;
    float branch=f(p,0x274),turn=f(p,0x3ac);
    return b(r,0xd2)==1&&b(r,0xd3)==1&&b(r,0xe1)==1&&!b(r,0xe7)&&
        !(w(r,0x1cc)&0x01204018u)&&b((void *)c->producer,0x41)==1&&
        !w(p,0x5d4)&&!w(p,0x5d8)&&f(p,0x3c8)==0.f&&
        finite(branch)&&branch>=0&&branch<1&&b(p,0x200)<=1&&b(p,0x201)<=1&&
        finite(turn)&&turn>=-.08f&&turn<=.08f;
}
static u32 ca_same(const CornerArcState *s,const ArcOwner *c){
    return s->producer==c->producer&&s->driver==c->driver&&s->role==c->role&&
        s->physics==c->physics&&s->drift==c->drift&&s->route==c->route&&
        s->course==c->course&&s->base_course==c->base_course&&s->clock==c->clock;
}
static u32 ca_done(CornerArcState *s,u32 answer,u32 reason,u32 retire){
    s->reason=reason;s->held+=answer;if(retire)s->phase=4;
    __atomic_store_n(&s->lock,0,__ATOMIC_RELEASE);return answer;
}
EXPORT void lab_corner_arc_begin(void *producer){
    ArcOwner c;if(!ca_owner(producer,&c))return;
    CornerArcState *s=&lab_corner_arc_state[c.racer];u32 expected=0;
    if(!__atomic_compare_exchange_n(&s->lock,&expected,1,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED))return;
    s->phase=4;
    if(!ca_driving(&c)||!finite(c.direction)||c.direction==0||c.ds>5){ca_done(s,0,1,1);return;}
    s->producer=c.producer;s->driver=c.driver;s->role=c.role;s->physics=c.physics;s->drift=c.drift;
    s->route=c.route;s->course=c.course;s->base_course=c.base_course;s->clock=c.clock;
    s->start=c.now;s->last=c.now;s->counter=0;s->target=0;s->deadline=180;
    s->charge=0;s->direction=c.direction;s->usable=0;s->gain=2;s->reserved=0;
    for(u32 i=0;i<3;i++)s->threshold[i]=f((void *)c.drift,0x80+4*i);
    s->phase=1;ca_done(s,0,0,0);
}
EXPORT void lab_corner_arc_end(void *producer){
    if(!ptr((u64)producer))return;
    /* Match only private owner addresses; avoid chasing retired native objects. */
    for(u32 i=0;i<12;i++){
        CornerArcState *s=&lab_corner_arc_state[i];u32 expected=0;
        if(!__atomic_compare_exchange_n(&s->lock,&expected,1,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED))continue;
        if(s->producer==(u64)producer)s->phase=4;
        __atomic_store_n(&s->lock,0,__ATOMIC_RELEASE);
    }
}
static __attribute__((unused)) u32 ca_edge(const ArcOwner *c,void *a,void *z,u32 occupied,u32 radial,ArcEdge *out){
    if(!simple_point(a)||!simple_point(z)||!be_edge(a,w(z,0x8c))||
       !be_intent((void *)c->physics,a,z,c->up,1.f,.0025f,occupied,
                  out->delta,&out->length2,&out->projection))return 0;
    out->radius=10000.f;
    for(u32 i=0;i<3;i++){
        out->offset[i]=f((void *)c->physics,0x2c+4*i)-f(a,0xc+4*i);
        if(!finite(out->offset[i]))return 0;
        for(u32 j=0;j<2;j++){
            float v=f(j?z:a,0x24+4*i);
            if(!finite(v)||(radial&&(v<=0||v>10000)))return 0;
            if(radial&&v<out->radius)out->radius=v;
        }
    }
    return 1;
}
/* 1=same useful bend, 2=near-straight, 0=opposite/sharp/unknown. */
static u32 ca_turn(const ArcOwner *c,const ArcEdge *a,const ArcEdge *z){
    float denom=root_positive(a->length2*z->length2),d=ca_dot(a->delta,z->delta)/denom;
    float cross[3]={a->delta[1]*z->delta[2]-a->delta[2]*z->delta[1],
        a->delta[2]*z->delta[0]-a->delta[0]*z->delta[2],
        a->delta[0]*z->delta[1]-a->delta[1]*z->delta[0]};
    float side=-ca_dot(cross,c->up)/denom*(c->direction<0?-1.f:1.f);
    if(!finite(d)||!finite(side)||d<.94f)return 0;
    if(side>=.01745f)return 1;
    return side>-.01745f?2:0;
}
/* A shared ordinary node has a bounded round join. The BASE ball is centered
 * on the actual base node and uses the smaller contracted adjacent width.
 * SELECTED intent uses only a24-unit join, never a free projection clamp.
 * Both complete ray unions must cover the horizon; genuine gaps still refuse. */
static u32 ca_ball(const ArcEdge *edge,const float *axis,float radius,BfbInterval *out){
    if(!finite(radius)||radius<=0)return 0;
    float offset[3];for(u32 i=0;i<3;i++)offset[i]=edge->offset[i]-edge->delta[i];
    float a=ca_dot(axis,axis),b0=ca_dot(offset,axis),c=ca_dot(offset,offset)-radius*radius;
    float d=b0*b0-a*c;if(!finite(d)||d<=0)return 0;
    float r=root_positive(d);out->lo=(-b0-r)/a;out->hi=(-b0+r)/a;
    out->lo_open=1;out->hi_open=1;return finite(out->lo)&&finite(out->hi)&&out->hi>out->lo;
}
static u32 ca_cover(BfbInterval *ranges,u32 n,float travel){
    float covered=0;u32 started=0;
    /* Only the occupied edge may establish initial containment. */
    if(!n||ranges[0].lo>0||(ranges[0].lo==0&&ranges[0].lo_open)||ranges[0].hi<=0)return 0;
    covered=ranges[0].hi;started=1;
    for(u32 pass=0;pass<n&&covered<=travel;pass++){
        float previous=covered;
        for(u32 i=1;i<n;i++)if(ranges[i].lo<covered&&ranges[i].hi>covered)covered=ranges[i].hi;
        if(covered==previous)break;
    }
    return started&&covered>travel;
}
#include "corner_arc_geometry_trace_v305.h"
#include "corner_arc_geometry_operand_trace_v305.h"
#include "corner_arc_selected_successor_v305.h"
#include "corner_arc_mixed_bend_v305.h"
#include "corner_native_road_plane_v305.h"
static u32 ca_geometry_frame(ArcOwner *c,float *usable,const CornerArcState *s,u32 road){
    ArcGeometryPayload d;cg_init(&d,c,s);
    void *physics=(void *)c->physics;
    if(road?!cnr_up(c,c->up):!ca_unit(physics,0x14,c->up))return cg_fail(c,&d,1,0);
    d.valid|=1;for(u32 i=0;i<3;i++)d.up[i]=c->up[i];
    if(!ca_unit(physics,0x2fc,c->motion[0]))return cg_fail(c,&d,2,0);
    d.valid|=2;for(u32 i=0;i<3;i++)d.actual[i]=c->motion[0][i];
    if(!ca_unit(physics,0x308,c->motion[1]))return cg_fail(c,&d,3,0);
    d.valid|=4;for(u32 i=0;i<3;i++)d.pending[i]=c->motion[1][i];
    c->speed=f(physics,0x37c);d.speed=c->speed;
    if(!finite(c->speed)||c->speed<=0)return cg_fail(c,&d,4,0);
    for(u32 o=0x424;o<=0x430;o+=4){float v=f(physics,o);d.caps[(o-0x424)/4]=v;d.valid|=1u<<(16+(o-0x424)/4);
        if(!finite(v)||v<0)return cg_fail(c,&d,5,o);if(v>c->speed)c->speed=v;d.speed=c->speed;}
    c->speed*=1.1f;d.speed=c->speed;
    if(!finite(c->speed)||c->speed>10000)return cg_fail(c,&d,6,0);
    for(u32 j=0;j<2;j++){float dot=ca_dot(c->motion[j],c->up);if(dot*dot>.0025f)return cg_fail(c,&d,7,j);}
    void *route=(void *)c->route;u64 a=q(route,0x60),z=q(route,0x58);d.a=a;d.z=z;
    if(!be_span(a,0x374)||!be_span(z,0x374))return cg_fail(c,&d,8,0);
    ArcPair edges[12];u32 count=0,bends=0,keys[13];float sel_total=0,base_total=0;
    float delta[3],length2,projection;
    if(!co_intent(c,(void *)a,(void *)z,1.f,.0025f,0,delta,&length2,&projection,9,0))return cg_fail(c,&d,9,0);
    if(projection<0){z=a;a=q(route,0x68);d.a=a;d.z=z;if(!be_span(a,0x374))return cg_fail(c,&d,10,0);}
    else if(projection>length2){if(!css_next(c,&a,&z))return cg_fail(c,&d,11,1);d.a=a;d.z=z;}
    ArcEdge base_lead;u32 base_prefix=0,base_lead_key=0;
    keys[0]=w((void *)a,0x8c);
    for(u32 i=0;i<(base_prefix?11u:12u);i++){
        d.edge=i;d.a=a;d.z=z;d.ba=0;d.bz=0;cg_clear_edge(&d);
        if(!be_span(a,0x374)||!be_span(z,0x374)||a==z)return cg_fail(c,&d,11,0);
        u32 ka=w((void *)a,0x8c),kz=w((void *)z,0x8c);d.keya=ka;d.keyz=kz;
        if(be_point((void *)c->course,ka)!=a||be_point((void *)c->course,kz)!=z)return cg_fail(c,&d,12,0);
        for(u32 j=0;j<=i;j++)if(keys[j]==kz)return cg_fail(c,&d,13,j);keys[i+1]=kz;
        u64 ba=be_point((void *)c->base_course,ka),bz=be_point((void *)c->base_course,kz);d.ba=ba;d.bz=bz;
        if(!ba||!bz)return cg_fail(c,&d,14,0);
        d.radial=0;u32 prefix_reason=0;
        if(!cp_edge(c,(void *)a,(void *)z,!i,0,&edges[i].selected,15,&prefix_reason)){
            if(i&&prefix_reason==7)break;
            return cg_fail(c,&d,15,0);
        }
        cg_edge_values(&d,&edges[i].selected);
        d.radial=1;cg_clear_edge(&d);
        if(!i){
            float bd[3],bl,bp;
            if(!co_intent(c,(void *)ba,(void *)bz,1.f,.0025f,0,bd,&bl,&bp,16,1))return cg_fail(c,&d,16,0);
            if(bp<0){
                if(!cmb_base_lead(c,(void *)ba,(void *)bz,&base_lead,&base_lead_key))return cg_fail(c,&d,16,0);
                base_prefix=1;
            }
        }
        if(!cp_edge(c,(void *)ba,(void *)bz,!i&&!base_prefix,1,&edges[i].base,16,&prefix_reason)){
            if(i&&prefix_reason==7)break;
            return cg_fail(c,&d,16,0);
        }
        if(!i&&base_prefix&&!ca_turn(c,&base_lead,&edges[i].base))return cg_fail(c,&d,16,0);
        cg_edge_values(&d,&edges[i].base);
        if(i){u32 st=ca_turn(c,&edges[i-1].selected,&edges[i].selected);
            u32 bt=ca_turn(c,&edges[i-1].base,&edges[i].base);d.detail=st|(bt<<16);
            if(!st||!bt||(st==2&&bt==2))break;bends++;d.bends=bends;}
        edges[i].key=kz;count++;d.count=count;
        float sl=root_positive(edges[i].selected.length2),bl=root_positive(edges[i].base.length2);
        sel_total+=sl-(!i?edges[i].selected.projection/sl:0);
        if(!i&&base_prefix){float pl=root_positive(base_lead.length2);base_total+=bl+pl-base_lead.projection/pl;}
        else base_total+=bl-(!i?edges[i].base.projection/bl:0);d.sel_total=sel_total;d.base_total=base_total;
        if(!finite(sel_total)||!finite(base_total))return cg_fail(c,&d,17,0);
        u32 next=w((void *)z,b((void *)z,0x2e4)?0x60:0x38);
        a=z;z=be_point((void *)c->course,next);if(!z)break;
    }
    if(!bends||count<2)return cg_fail(c,&d,18,d.detail);
    float reserve=24.f+c->speed*.08f*4.f*5.f*.5f,travel=c->speed*4.f;d.reserve=reserve;d.travel=travel;
    for(u32 mode=0;mode<2;mode++)for(u32 radial=0;radial<2;radial++){
        d.mode=mode;d.radial=radial;d.a=0;d.z=0;d.ba=0;d.bz=0;
        BfbInterval ranges[24];u32 n=0;
        if(radial&&base_prefix){
            BfbInterval range;d.edge=0;d.keya=base_lead_key;d.keyz=keys[0];
            cg_edge_values(&d,&base_lead);d.clearance=base_lead.radius-reserve;d.end_reserve=0;
            if(!bfb_interval(base_lead.delta,base_lead.length2,base_lead.projection,base_lead.offset,c->motion[mode],
                base_lead.radius-reserve,0.f,1,&range))return cg_fail(c,&d,19,0);
            ranges[n++]=range;
            float radius=(base_lead.radius<edges[0].base.radius?base_lead.radius:edges[0].base.radius)-reserve;
            if(ca_ball(&base_lead,c->motion[mode],radius,&range))ranges[n++]=range;
        }
        for(u32 i=0;i<count;i++){
            ArcEdge *edge=radial?&edges[i].base:&edges[i].selected;BfbInterval range;
            float end_reserve=i+1==count?reserve:0.f;
            d.edge=i;d.keya=keys[i];d.keyz=keys[i+1];cg_edge_values(&d,edge);d.clearance=radial?edge->radius-reserve:0.f;d.end_reserve=end_reserve;
            if(!bfb_interval(edge->delta,edge->length2,edge->projection,edge->offset,c->motion[mode],
                 radial?edge->radius-reserve:0.f,end_reserve,radial,&range)){if(!i)return cg_fail(c,&d,19,0);continue;}
            ranges[n++]=range;
            if(i+1<count){
                float width=edges[i].base.radius<edges[i+1].base.radius?edges[i].base.radius:edges[i+1].base.radius;
                float radius=width-reserve;if(!radial&&radius>24.f)radius=24.f;
                if(ca_ball(edge,c->motion[mode],radius,&range))ranges[n++]=range;
            }
        }
        if(!cg_cover(ranges,n,travel,&d)){
            /* Cover is a union decision. Retain the occupied edge as context,
             * never label the last scanned future edge as the failing one. */
            ArcEdge *first=radial?(base_prefix?&base_lead:&edges[0].base):&edges[0].selected;
            d.edge=d.detail<=3?0:0xffffffffu;d.keya=radial&&base_prefix?base_lead_key:keys[0];d.keyz=radial&&base_prefix?keys[0]:keys[1];
            cg_edge_values(&d,first);d.valid|=128;d.clearance=radial?first->radius-reserve:0.f;
            d.end_reserve=count==1&&!(radial&&base_prefix)?reserve:0.f;return cg_fail(c,&d,20,d.detail);
        }
    }
    *usable=(sel_total<base_total?sel_total:base_total)-64.f-6.f*c->speed;d.usable=*usable;
    if(!(finite(*usable)&&*usable>0))return cg_fail(c,&d,21,0);return 1;
}
static u32 ca_geometry(ArcOwner *c,float *usable,const CornerArcState *s){
    if(ca_geometry_frame(c,usable,s,0))return 1;
    return ca_geometry_frame(c,usable,s,1);
}

EXPORT u32 lab_corner_arc_hold(void *producer,u32 request,float native_dot,u32 legacy_held){
    ArcOwner c;if(!ca_owner(producer,&c)){lab_corner_arc_end(producer);return 0;}
    CornerArcState *s=&lab_corner_arc_state[c.racer];u32 expected=0;
    if(!__atomic_compare_exchange_n(&s->lock,&expected,1,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED))return 0;
    s->offered++;
    if(s->phase<1||s->phase>3)return ca_done(s,0,2,0);
    if(!ca_same(s,&c)||!ca_driving(&c)||request>1||!finite(native_dot)||native_dot<0||
       c.now<s->last||c.counter<s->counter||c.now-s->start!=c.counter||
       (s->phase>1&&(c.now!=s->last+1||c.counter!=s->counter+1))||
       c.counter>=s->deadline)return ca_done(s,0,3,1);
    if(c.ds!=4&&c.ds!=5)return ca_done(s,0,4,1);
    if(s->phase==3&&c.tier>=s->target)return ca_done(s,0,9,1);
    if(!finite(c.direction)||c.direction!=s->direction||
       !((c.ds==4&&c.direction<0)||(c.ds==5&&c.direction>0))||
       c.tier>=3||!finite(c.charge)||c.charge<0)return ca_done(s,0,4,1);
    for(u32 i=0;i<3;i++)if(!finite(s->threshold[i])||s->threshold[i]<=0||
        (i&&s->threshold[i]<=s->threshold[i-1])||f((void *)c.drift,0x80+4*i)!=s->threshold[i])return ca_done(s,0,5,1);
    float gain=2.f,actual=f((void *)c.role,0x17c),pending=f((void *)c.packet,8);
    if(c.direction<0){actual=-actual;pending=-pending;}
    if(!finite(actual)||!finite(pending)||actual<-.01f||pending<-.01f||actual>1.f)return ca_done(s,0,6,1);
    if(s->phase>1&&c.charge<=s->charge)return ca_done(s,0,7,1);
    if(s->phase>1&&c.charge-s->charge>=4.9f&&actual>=.7f&&pending>=.7f)gain=5.f;
    s->last=c.now;s->counter=c.counter;s->charge=c.charge;s->gain=gain;
    if(s->phase==1)s->phase=2;
    if(!request||legacy_held)return ca_done(s,0,8,0);
    float usable;if(!ca_geometry(&c,&usable,s))return ca_done(s,0,10,1);s->usable=usable;
    float available=usable/c.speed;u32 remaining=s->deadline-c.counter;
    u32 ticks=available>=remaining?remaining:(u32)available;
    u32 best=c.tier;
    for(u32 tier=c.tier+1;tier<=3;tier++){
        if(tier==3&&(w((void *)c.role,0x1cc)&0x2000u))break;
        float projected=c.charge;u32 n=0;
        while(projected<=s->threshold[tier-1]&&n<ticks){projected+=gain;n++;}
        if(projected>s->threshold[tier-1])best=tier;else break;
    }
    if(s->phase==3&&best>s->target)best=s->target;
    if(best<=c.tier)return ca_done(s,0,11,1);
    s->target=best;s->phase=3;return ca_done(s,1,0,0);
}
