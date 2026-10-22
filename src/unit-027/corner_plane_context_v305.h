/* Passive companion to an exact co_intent plane refusal. Existing diagnostic
 * ABI and all driving predicates remain unchanged. Native words are copied
 * only while the actor/clock still match the evaluated failure context. */
typedef struct {
    u32 chassis[3],actual[3],target[3],road[3];
    u32 contact,air,mode,selector,role_flags,modified_normal;
    u32 displacement_bits,end_tick;
} CornerPlaneContext;
typedef struct {
    u32 lock,fault;u64 sequence,events,dropped;
    ArcOperandPayload value;CornerPlaneContext context;
} CornerPlaneRecord;
_Static_assert(sizeof(CornerPlaneContext)==80,"Corner plane context ABI1");
_Static_assert(sizeof(CornerPlaneRecord)==352,"Corner plane record ABI1");
EXPORT const u32 lab_corner_plane_context_config[4]={1,12,352,7};
EXPORT CornerPlaneRecord lab_corner_plane_context_records[12];

/* Volatile reads deliberately preserve the before/after lifecycle checks.
 * Ordinary q/w/b permit the compiler to reuse the earlier loaded value. */
static u64 cpc_q(const void *p,u32 o){return *(const volatile u64 *)((const u8 *)p+o);}
static u32 cpc_w(const void *p,u32 o){return *(const volatile u32 *)((const u8 *)p+o);}
static u8 cpc_b(const void *p,u32 o){return *(const volatile u8 *)((const u8 *)p+o);}

static void cpc_publish(const ArcOwner *c,const ArcOperandPayload *d){
    if(d->detail!=7||d->valid!=127||c->racer>=12||d->racer!=c->racer||
       d->physics!=c->physics||d->role!=c->role||d->clock!=c->clock||d->drift!=c->drift||
       !ptr(c->physics)||!ptr(c->role)||!ptr(c->clock)||!ptr(c->drift))return;
    const void *p=(void *)c->physics,*role=(void *)c->role,*clock=(void *)c->clock;
    if(cpc_w(clock,0x38)!=6||cpc_w(clock,0x44)!=d->tick||
       cpc_q(p,0xf8)!=c->role||cpc_q(role,0x28)!=c->physics||cpc_q(p,0x110)!=c->drift||
       cpc_q((void *)c->drift,8)!=c->role||cpc_w((void *)c->drift,0x10)!=d->ds)return;
    CornerPlaneContext context;
    for(u32 i=0;i<3;i++){
        context.chassis[i]=cpc_w(p,0x14+4*i);context.actual[i]=cpc_w(p,0x320+4*i);
        context.target[i]=cpc_w(p,0x32c+4*i);context.road[i]=cpc_w(p,0x17c+4*i);
    }
    context.contact=cpc_b(p,0x200)|(cpc_b(p,0x201)<<1);context.air=cpc_w(p,0x26c);
    context.mode=cpc_w(p,0x230);context.selector=cpc_b(p,0x27b);context.role_flags=cpc_w(role,0x1cc);
    context.modified_normal=cpc_b(p,0x20a);context.displacement_bits=cpc_w(p,0x568);
    context.end_tick=cpc_w(clock,0x44);
    if(context.end_tick!=d->tick||cpc_w(clock,0x38)!=6||cpc_q(p,0xf8)!=c->role||
       cpc_q(role,0x28)!=c->physics||cpc_q(p,0x110)!=c->drift||
       cpc_q((void *)c->drift,8)!=c->role||cpc_w((void *)c->drift,0x10)!=d->ds)return;
    CornerPlaneRecord *r=&lab_corner_plane_context_records[c->racer];u32 unlocked=0;
    if(!__atomic_compare_exchange_n(&r->lock,&unlocked,1,1,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED)){
        u64 lost=__atomic_load_n(&r->dropped,__ATOMIC_RELAXED);
        if(lost!=~0ull)__atomic_compare_exchange_n(&r->dropped,&lost,lost+1,1,__ATOMIC_RELAXED,__ATOMIC_RELAXED);
        return;
    }
    if(r->fault)goto done;
    if((r->sequence&1)||r->sequence>~0ull-2||r->events==~0ull){r->fault=1;goto done;}
    __atomic_add_fetch(&r->sequence,1,__ATOMIC_ACQ_REL);
    r->value=*d;r->context=context;r->events++;
    __atomic_add_fetch(&r->sequence,1,__ATOMIC_RELEASE);
done:__atomic_store_n(&r->lock,0,__ATOMIC_RELEASE);
}
