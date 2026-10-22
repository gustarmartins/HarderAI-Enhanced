/* Isolated owned-hop vector refusal ABI. No native fields are dereferenced
 * here: only evaluated helper scalars and already-owned frame fields. */
typedef struct {
    u32 site,detail,component,count;
    u32 values[8],reserved[4];
} HvrFailure;
typedef struct {HvrFailure primary,fallback;} HvrProbe;
typedef struct {
    u32 lock,fault;u64 sequence,events,dropped;
    u64 driver,producer,clock,physics,drift,route,course,base_course;
    u32 tick,start,ticks,reserved;
    HvrProbe failures;
    u32 frame_valid,pad[3];
} HvrRecord;
_Static_assert(sizeof(HvrFailure)==64,"Vector failure ABI1");
_Static_assert(sizeof(HvrRecord)==256,"Vector refusal ABI1");
_Static_assert(__builtin_offsetof(HvrRecord,failures)==112,"Vector failure payload");
EXPORT const u32 lab_snaking_hop_vector_refusal_config[8]={1,256,12,64,15,121,8,0};
EXPORT HvrRecord lab_snaking_hop_vector_refusal_records[12];

static u32 hvr_fail(HvrFailure *p,u32 site,u32 detail,u32 component,const float *v,u32 count){
    if(p->site)return 0;
    p->site=site;p->detail=detail;p->component=component;p->count=count;
    for(u32 i=0;i<8;i++)p->values[i]=i<count?((const u32 *)v)[i]:0;
    return 0;
}
static u32 hvr_group(HvrFailure *p,u32 site){
    return hvr_fail(p,site,0,0xffffffffu,(const float *)0,0);
}
static void hvr_publish(const HvrProbe *p,const SnakeHopFrame *frame,
                        const void *physics,const void *route,u32 ticks){
    u64 index=(u64)frame-(u64)lab_snaking_hop_frames;
    if(!p->primary.site||!p->fallback.site||index>=sizeof(lab_snaking_hop_frames)||index%sizeof(SnakeHopFrame))return;
    HvrRecord *r=&lab_snaking_hop_vector_refusal_records[index/sizeof(SnakeHopFrame)];
    u32 expected=0;
    if(!__atomic_compare_exchange_n(&r->lock,&expected,1,1,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED)){
        u64 lost=__atomic_load_n(&r->dropped,__ATOMIC_RELAXED);
        if(lost!=~0ull)__atomic_compare_exchange_n(&r->dropped,&lost,lost+1,1,__ATOMIC_RELAXED,__ATOMIC_RELAXED);
        return;
    }
    if(r->fault)goto done;
    if((r->sequence&1)||r->sequence>~0ull-2||r->events==~0ull){r->fault=1;goto done;}
    __atomic_add_fetch(&r->sequence,1,__ATOMIC_ACQ_REL);r->events++;
    r->driver=frame->driver;r->producer=frame->producer;r->clock=frame->clock;
    r->physics=(u64)physics;r->drift=frame->drift;r->route=(u64)route;
    r->course=frame->course;r->base_course=frame->base_course;
    r->tick=frame->last;r->start=frame->start;r->ticks=ticks;r->reserved=0;
    r->failures=*p;r->frame_valid=frame->valid;
    __atomic_add_fetch(&r->sequence,1,__ATOMIC_RELEASE);
done:
    __atomic_store_n(&r->lock,0,__ATOMIC_RELEASE);
}
