/* Exact operands for the existing be_intent refusal. No policy changes. */
typedef struct {
    u64 producer,driver,role,physics,drift,route,course,base_course,clock,a,z;
    u32 tick,racer,start,counter,phase,target,ds,tier;
    u32 stage,detail,component,occupied,radial,valid,keya,keyz;
    float start_xyz[3],end_xyz[3],position[3],up[3],delta[3];
    float length2,projection,vertical,up2,plane2,reserved[2];
} ArcOperandPayload;
typedef struct {
    u32 lock,fault;u64 sequence,events,dropped;ArcOperandPayload value;
} ArcOperandTrace;
_Static_assert(sizeof(ArcOperandPayload)==240,"Arc operand payload ABI1");
_Static_assert(sizeof(ArcOperandTrace)==272,"Arc operand record ABI1");
EXPORT const u32 lab_corner_arc_operand_config[8]={1,12,272,9,1,0,0,0};
EXPORT ArcOperandTrace lab_corner_arc_operand_records[12];
#include "corner_plane_context_v305.h"

static u32 co_fail(const ArcOwner *c,ArcOperandPayload *d,u32 detail){
    d->detail=detail;
    cpc_publish(c,d);
    if(c->racer>=12)return 0;
    ArcOperandTrace *s=&lab_corner_arc_operand_records[c->racer];u32 unlocked=0;
    if(!__atomic_compare_exchange_n(&s->lock,&unlocked,1,1,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED)){
        u64 dropped=__atomic_load_n(&s->dropped,__ATOMIC_RELAXED);
        if(dropped!=~0ull)__atomic_compare_exchange_n(&s->dropped,&dropped,dropped+1,1,__ATOMIC_RELAXED,__ATOMIC_RELAXED);
        return 0;
    }
    if(s->fault)goto done;
    if((s->sequence&1)||s->sequence>~0ull-2||s->events==~0ull){s->fault=1;goto done;}
    __atomic_add_fetch(&s->sequence,1,__ATOMIC_ACQ_REL);
    for(u32 i=0;i<sizeof(*d)/4;i++)((volatile u32 *)&s->value)[i]=((const volatile u32 *)d)[i];
    s->events++;__atomic_add_fetch(&s->sequence,1,__ATOMIC_RELEASE);
done:__atomic_store_n(&s->lock,0,__ATOMIC_RELEASE);return 0;
}

#include "corner_prefix_v305.h"
static u32 cp_intent(const ArcOwner *c,const void *a,const void *z,
    float up2,float plane2,u32 occupied,float *delta,float *length2,float *projection,u32 stage,u32 radial,u32 *reason){
    ArcOperandPayload d;for(u32 i=0;i<sizeof(d)/4;i++)((volatile u32 *)&d)[i]=0;
    const CornerArcState *s=&lab_corner_arc_state[c->racer];
    d.producer=c->producer;d.driver=c->driver;d.role=c->role;d.physics=c->physics;
    d.drift=c->drift;d.route=c->route;d.course=c->course;d.base_course=c->base_course;d.clock=c->clock;
    d.tick=c->now;d.racer=c->racer;d.start=s->start;d.counter=c->counter;
    d.phase=s->phase;d.target=s->target;d.ds=c->ds;d.tier=c->tier;
    d.stage=stage;d.occupied=occupied;d.radial=radial;d.up2=up2;d.plane2=plane2;
    d.a=(u64)a;d.z=(u64)z;d.keya=w(a,0x8c);d.keyz=w(z,0x8c);d.component=0xffffffffu;
    for(u32 i=0;i<3;i++)d.up[i]=c->up[i];
    float n=0,p=0,vertical=0;
    for(u32 i=0;i<3;i++){
        float start=f(a,0xc+4*i),end=f(z,0xc+4*i),position=f((void *)c->physics,0x2c+4*i);
        d.component=i;d.start_xyz[i]=start;d.end_xyz[i]=end;d.position[i]=position;d.valid|=1u<<i;
        if(!finite(start))return cp_fail(c,&d,1,reason);
        if(!finite(end))return cp_fail(c,&d,2,reason);
        if(!finite(position))return cp_fail(c,&d,3,reason);
        delta[i]=end-start;n+=delta[i]*delta[i];
        p+=(position-start)*delta[i];vertical+=delta[i]*c->up[i];d.delta[i]=delta[i];d.valid|=1u<<(i+3);
    }
    d.component=0xffffffffu;d.length2=n;d.projection=p;d.vertical=vertical;d.valid|=64u;
    if(!finite(n))return cp_fail(c,&d,4,reason);
    if(n<64)return cp_fail(c,&d,5,reason);
    if(!finite(p))return cp_fail(c,&d,6,reason);
    if(vertical*vertical>plane2*n*up2)return cp_fail(c,&d,7,reason);
    if(occupied&&p<0)return cp_fail(c,&d,8,reason);
    if(occupied&&p>n)return cp_fail(c,&d,9,reason);
    *length2=n;*projection=p;return 1;
}

static u32 co_intent(const ArcOwner *c,const void *a,const void *z,
    float up2,float plane2,u32 occupied,float *delta,float *length2,float *projection,u32 stage,u32 radial){
    return cp_intent(c,a,z,up2,plane2,occupied,delta,length2,projection,stage,radial,0);
}

/* Preserve the original ca_edge order and all its early topology guards. */
static u32 cp_edge(const ArcOwner *c,void *a,void *z,u32 occupied,u32 radial,ArcEdge *out,u32 stage,u32 *reason){
    if(reason)*reason=0;
    if(!simple_point(a)||!simple_point(z)||!be_edge(a,w(z,0x8c))||
       !cp_intent(c,a,z,1.f,.0025f,occupied,out->delta,&out->length2,&out->projection,stage,radial,reason))return 0;
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

static u32 co_edge(const ArcOwner *c,void *a,void *z,u32 occupied,u32 radial,ArcEdge *out,u32 stage){
    return cp_edge(c,a,z,occupied,radial,out,stage,0);
}
