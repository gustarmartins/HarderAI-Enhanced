/* Private diagnostic ABI1. Called only for an ultimately refused owned-hop
 * corridor. No native object or packet writes; no dereference of stored route
 * pointers. Numeric inputs are copied from existing evaluated local values.
 * This source-only proposal still needs compiled equivalence/stack qualification.
 */
typedef struct {
    u32 stage,substage,segment,view,mode,pad;
    u64 a,z,ba,bz;
    u32 values[24];
} HfdProbe;
typedef struct {
    u32 lock,fault;
    u64 sequence,events,dropped;
    u64 driver,producer,clock,physics,drift,route,course,base_course;
    u32 tick,start,ticks,stage,substage,segment,view,mode;
    u64 a,z,ba,bz;
    u32 values[24];
} HfdRecord;
_Static_assert(sizeof(HfdRecord)==256,"Hop refusal ABI1 size");
_Static_assert(__builtin_offsetof(HfdRecord,driver)==32,"Hop refusal owners");
_Static_assert(__builtin_offsetof(HfdRecord,tick)==96,"Hop refusal tick");
_Static_assert(__builtin_offsetof(HfdRecord,values)==160,"Hop refusal operands");
EXPORT const u32 lab_snaking_hop_refusal_diag_config[6]={1,256,12,24,30,121};
EXPORT HfdRecord lab_snaking_hop_refusal_diag_records[12];

static void hfd_copy(HfdProbe *p,u32 segment,u32 view,u32 mode,
                     u64 a,u64 z,u64 ba,u64 bz,const float *v,u32 count){
    p->segment=segment;p->view=view;p->mode=mode;
    p->a=a;p->z=z;p->ba=ba;p->bz=bz;
    for(u32 i=0;i<24;i++)p->values[i]=i<count?((const u32 *)v)[i]:0;
}
static u32 hfd_root_fail(HfdProbe *p,u32 stage,u32 segment,u32 view,u32 mode,
                         u64 a,u64 z,u64 ba,u64 bz,const float *v,u32 count){
    p->stage=stage;
    /* A returned nested future refusal keeps its actual local operands. */
    if(!p->substage)hfd_copy(p,segment,view,mode,a,z,ba,bz,v,count);
    return 0;
}
static u32 hfd_future_fail(HfdProbe *p,u32 stage,u32 segment,u32 view,u32 mode,
                           u64 a,u64 z,u64 ba,u64 bz,const float *v,u32 count){
    p->substage=stage;hfd_copy(p,segment,view,mode,a,z,ba,bz,v,count);return 0;
}
static u32 hfd_coverage_end(HfdProbe *p,const HfdProbe *coverage,u32 termination){
    if(coverage->substage==103||coverage->substage==104){
        *p=*coverage;p->substage|=termination<<16;
    }else p->substage=termination;
    return 0;
}
static void hfd_publish(const HfdProbe *p,const SnakeHopFrame *frame,
                        const void *physics,const void *route,u32 ticks){
    u64 index=(u64)frame-(u64)lab_snaking_hop_frames;
    if(!p->stage||index>=sizeof(lab_snaking_hop_frames)||index%sizeof(SnakeHopFrame))return;
    HfdRecord *r=&lab_snaking_hop_refusal_diag_records[index/sizeof(SnakeHopFrame)];
    u32 expected=0;
    if(!__atomic_compare_exchange_n(&r->lock,&expected,1,1,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED)){
        u64 lost=__atomic_load_n(&r->dropped,__ATOMIC_RELAXED);
        if(lost!=~0ull)__atomic_compare_exchange_n(&r->dropped,&lost,lost+1,1,__ATOMIC_RELAXED,__ATOMIC_RELAXED);
        return;
    }
    if(r->fault)goto done;
    if((r->sequence&1)||r->sequence>~0ull-2||r->events==~0ull){r->fault=1;goto done;}
    __atomic_add_fetch(&r->sequence,1,__ATOMIC_ACQ_REL);
    r->events++;
    /* These are already-owned private frame fields, not new native reads. */
    r->driver=frame->driver;r->producer=frame->producer;r->clock=frame->clock;
    r->physics=(u64)physics;r->drift=frame->drift;r->route=(u64)route;
    r->course=frame->course;r->base_course=frame->base_course;
    r->tick=frame->last;r->start=frame->start;r->ticks=ticks;
    r->stage=p->stage;r->substage=p->substage;r->segment=p->segment;r->view=p->view;r->mode=p->mode;
    r->a=p->a;r->z=p->z;r->ba=p->ba;r->bz=p->bz;
    for(u32 i=0;i<24;i++)r->values[i]=p->values[i];
    __atomic_add_fetch(&r->sequence,1,__ATOMIC_RELEASE);
done:
    __atomic_store_n(&r->lock,0,__ATOMIC_RELEASE);
}
