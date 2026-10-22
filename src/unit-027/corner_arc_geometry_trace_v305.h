/* Optional diagnostic only. Existing geometry results and native stores stay
 * unchanged. One failure payload is copied to private storage without waiting. */
typedef struct {
    u64 producer,driver,role,physics,drift,route,course,base_course,clock;
    u32 tick,racer,start,counter,phase,target,ds,tier;
    u32 stage,detail,edge,mode,radial,count,bends,nranges;
    u64 a,z,ba,bz;
    u32 keya,keyz,valid,reserved;
    float charge,direction,speed,reserve,travel,sel_total,base_total,usable;
    float length2,projection,radius,clearance,end_reserve,covered,lo,hi;
    float position[3],up[3],actual[3],pending[3],delta[3],offset[3],caps[4];
} ArcGeometryPayload;
typedef struct {
    u32 lock,fault;
    u64 sequence,events,dropped;
    ArcGeometryPayload value;
} ArcGeometryTrace;
_Static_assert(sizeof(ArcGeometryPayload)==336,"Arc geometry payload ABI1");
_Static_assert(sizeof(ArcGeometryTrace)==368,"Arc geometry trace ABI1");
EXPORT const u32 lab_corner_arc_geometry_config[8]={1,12,368,21,1,0,0,0};
EXPORT ArcGeometryTrace lab_corner_arc_geometry_records[12];

static void cg_init(ArcGeometryPayload *d,const ArcOwner *c,const CornerArcState *s){
    /* Volatile stores keep the bounded initialization freestanding. */
    for(u32 i=0;i<sizeof(*d)/4;i++)((volatile u32 *)d)[i]=0;
    d->producer=c->producer;d->driver=c->driver;d->role=c->role;d->physics=c->physics;
    d->drift=c->drift;d->route=c->route;d->course=c->course;d->base_course=c->base_course;d->clock=c->clock;
    d->tick=c->now;d->racer=c->racer;d->start=s->start;d->counter=c->counter;
    d->phase=s->phase;d->target=s->target;d->ds=c->ds;d->tier=c->tier;
    d->charge=c->charge;d->direction=c->direction;
    for(u32 i=0;i<3;i++)d->position[i]=f((void *)c->physics,0x2c+4*i);
}
static void cg_edge_values(ArcGeometryPayload *d,const ArcEdge *e){
    d->valid|=8;d->length2=e->length2;d->projection=e->projection;d->radius=e->radius;
    for(u32 i=0;i<3;i++){d->delta[i]=e->delta[i];d->offset[i]=e->offset[i];}
}
static void cg_clear_edge(ArcGeometryPayload *d){
    d->valid&=~8u;d->length2=0;d->projection=0;d->radius=0;
    for(u32 i=0;i<3;i++){d->delta[i]=0;d->offset[i]=0;}
}
static u32 cg_fail(const ArcOwner *c,ArcGeometryPayload *d,u32 stage,u32 detail){
    d->stage=stage;d->detail=detail;
    if(c->racer>=12)return 0;
    ArcGeometryTrace *s=&lab_corner_arc_geometry_records[c->racer];u32 unlocked=0;
    if(!__atomic_compare_exchange_n(&s->lock,&unlocked,1,1,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED)){
        u64 dropped=__atomic_load_n(&s->dropped,__ATOMIC_RELAXED);
        if(dropped!=~0ull)__atomic_compare_exchange_n(&s->dropped,&dropped,dropped+1,1,__ATOMIC_RELAXED,__ATOMIC_RELAXED);
        return 0;
    }
    if(s->fault)goto done;
    if((s->sequence&1)||s->sequence>~0ull-2||s->events==~0ull){s->fault=1;goto done;}
    __atomic_add_fetch(&s->sequence,1,__ATOMIC_ACQ_REL);
    for(u32 i=0;i<sizeof(*d)/4;i++)((volatile u32 *)&s->value)[i]=((const volatile u32 *)d)[i];
    s->events++;
    __atomic_add_fetch(&s->sequence,1,__ATOMIC_RELEASE);
done:
    __atomic_store_n(&s->lock,0,__ATOMIC_RELEASE);return 0;
}
/* Same union algorithm and comparison order as ca_cover. These extra stores
 * describe the exact local refusal; no second geometry evaluation is made. */
static u32 cg_cover(BfbInterval *ranges,u32 n,float travel,ArcGeometryPayload *d){
    if(!d)return ca_cover(ranges,n,travel);
    float covered=0;u32 started=0;d->nranges=n;d->covered=0;d->detail=0;
    d->valid&=~(16u|32u|64u);d->lo=0;d->hi=0;
    if(n){d->valid|=16;d->lo=ranges[0].lo;d->hi=ranges[0].hi;
        d->valid|=(ranges[0].lo_open?32u:0u)|(ranges[0].hi_open?64u:0u);}
    if(!n||ranges[0].lo>0||(ranges[0].lo==0&&ranges[0].lo_open)||ranges[0].hi<=0){
        d->detail=!n?1:(ranges[0].lo>0||(ranges[0].lo==0&&ranges[0].lo_open))?2:3;return 0;
    }
    covered=ranges[0].hi;started=1;
    for(u32 pass=0;pass<n&&covered<=travel;pass++){
        float previous=covered;
        for(u32 i=1;i<n;i++)if(ranges[i].lo<covered&&ranges[i].hi>covered)covered=ranges[i].hi;
        if(covered==previous)break;
    }
    d->covered=covered;u32 answer=started&&covered>travel;if(!answer)d->detail=4;return answer;
}
