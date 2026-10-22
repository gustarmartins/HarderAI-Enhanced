/* Private source-only drift-corridor refusal ABI1. Included after the existing
 * SnakeHopFrame/drift-frame declarations. No native object dereferences here.
 * Native equivalence, compiled layout and stack/cost remain unqualified. */
typedef struct {
    u64 horizon_role;
    u32 horizon_flags,elapsed,episode;
    i32 direction;
    u32 packet_flags,native_state;
    u32 axis,inward,target,initial_charge;
} DfrContext;
typedef struct {
    u32 stage,segment,view,mode;
    u64 a,z,ba,bz;
    u32 values[24];
    u32 required_distance,required_distance_valid,fallback;
} DfrProbe;
_Static_assert(sizeof(DfrProbe)==160,"Fallback probe stack layout");
typedef struct {
    u32 lock,fault;
    u64 sequence,events,dropped;
    u64 driver,producer,clock,physics,drift,route,course,base_course;
    u32 tick,start,ticks,stage,segment,view,mode,physical_state;
    u64 a,z,ba,bz;
    u32 values[24];
    DfrContext context;
    u32 required_distance,required_distance_valid,frame_valid,reserved;
} DfrRecord;
_Static_assert(sizeof(DfrContext)==48,"Drift refusal context ABI1");
_Static_assert(sizeof(DfrRecord)==320,"Drift refusal record ABI1");
_Static_assert(__builtin_offsetof(DfrRecord,driver)==32,"Drift refusal owners");
_Static_assert(__builtin_offsetof(DfrRecord,tick)==96,"Drift refusal tick");
_Static_assert(__builtin_offsetof(DfrRecord,values)==160,"Drift refusal operands");
_Static_assert(__builtin_offsetof(DfrRecord,context)==256,"Drift refusal context");
EXPORT const u32 lab_snaking_drift_refusal_diag_config[8]={1,320,12,24,29,48,80,15};
EXPORT DfrRecord lab_snaking_drift_refusal_diag_records[12];

static u32 dfr_bits(float value){
    union {float value;u32 bits;} copy={.value=value};return copy.bits;
}
static u32 dfr_fail(DfrProbe *p,u32 stage,u32 segment,u32 view,u32 mode,
                    u64 a,u64 z,u64 ba,u64 bz,const float *values,u32 count){
    p->stage=stage;p->segment=segment;p->view=view;p->mode=mode;p->fallback=0;
    p->a=a;p->z=z;p->ba=ba;p->bz=bz;
    for(u32 i=0;i<24;i++)p->values[i]=i<count?dfr_bits(values[i]):0;
    return 0;
}
static void dfr_publish(const DfrProbe *p,const SnakeHopFrame *frame,
                        const void *physics,const void *route,u32 ticks,const DfrContext *context){
    u64 index=(u64)frame-(u64)lab_snaking_drift_frames;
    if(!p->stage||index>=sizeof(lab_snaking_drift_frames)||index%sizeof(SnakeHopFrame))return;
    DfrRecord *r=&lab_snaking_drift_refusal_diag_records[index/sizeof(SnakeHopFrame)];
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
    /* These owners/times are already-owned private BSS under SnakeState.lock.
     * Recorded route/point values remain historical identifiers only. */
    r->driver=frame->driver;r->producer=frame->producer;r->clock=frame->clock;
    r->physics=(u64)physics;r->drift=frame->drift;r->route=(u64)route;
    r->course=frame->course;r->base_course=frame->base_course;
    r->tick=frame->last;r->start=frame->start;r->ticks=ticks;
    r->stage=p->stage;r->segment=p->segment;r->view=p->view;r->mode=p->mode;
    r->physical_state=frame->pad;
    r->a=p->a;r->z=p->z;r->ba=p->ba;r->bz=p->bz;
    for(u32 i=0;i<24;i++)r->values[i]=p->values[i];
    r->context=*context;r->required_distance=p->required_distance;
    r->required_distance_valid=p->required_distance_valid;r->frame_valid=frame->valid;r->reserved=p->fallback;
    __atomic_add_fetch(&r->sequence,1,__ATOMIC_RELEASE);
done:
    __atomic_store_n(&r->lock,0,__ATOMIC_RELEASE);
}
