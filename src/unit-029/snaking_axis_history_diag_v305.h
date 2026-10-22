/* First refusal of an owned native-normal history. Observation only.
 * The caller holds SnakeState's lock. No extra native object reads occur here;
 * publication copies private history and the already evaluated operands. */
typedef struct {u32 code,index,left,right;} NgdFailure;
typedef struct {
    u32 lock,fault;u64 sequence,events,dropped;
    SnakeHopFrame frame;
    GroundNativeAxis previous;
    u32 air,contact,mode,selector,ds,elapsed;
    NgdFailure failure;
    u32 reserved[8];
} NgdRecord;
_Static_assert(sizeof(NgdRecord)==384,"Axis history diagnostic ABI1");
EXPORT const u32 lab_snaking_axis_history_diag_config[4]={1,384,12,0};
EXPORT NgdRecord lab_snaking_axis_history_diag_records[12];

static u32 ngd_test(u32 refused,NgdFailure *p,u32 code){
    if(refused&&!p->code){p->code=code;p->index=0xffffffffu;}
    return refused;
}
static u32 ngd_equal(u32 left,u32 right,NgdFailure *p,u32 code,u32 index){
    if(left==right)return 1;
    if(!p->code){p->code=code;p->index=index;p->left=left;p->right=right;}
    return 0;
}
static void ngd_publish(const SnakeHopFrame *frame,const GroundNativeAxis *previous,
                        const NgdFailure *failure,u32 air,u32 contact,u32 mode,
                        u32 selector,u32 ds,u32 elapsed){
    if(!failure->code||previous->poison)return;
    u64 offset=(u64)previous-(u64)lab_snaking_ground_axis_proposal_records;
    if(offset>=sizeof(lab_snaking_ground_axis_proposal_records)||offset%192)return;
    NgdRecord *r=&lab_snaking_axis_history_diag_records[offset/192];u32 expected=0;
    if(!__atomic_compare_exchange_n(&r->lock,&expected,1,1,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED)){
        u64 lost=__atomic_load_n(&r->dropped,__ATOMIC_RELAXED);
        if(lost!=~0ull)__atomic_compare_exchange_n(&r->dropped,&lost,lost+1,1,__ATOMIC_RELAXED,__ATOMIC_RELAXED);
        return;
    }
    if(r->fault)goto done;
    if((r->sequence&1)||r->sequence>~0ull-2||r->events==~0ull){r->fault=1;goto done;}
    __atomic_add_fetch(&r->sequence,1,__ATOMIC_ACQ_REL);r->events++;
    r->frame=*frame;r->previous=*previous;r->failure=*failure;
    r->air=air;r->contact=contact;r->mode=mode;r->selector=selector;r->ds=ds;r->elapsed=elapsed;
    for(u32 i=0;i<8;i++)r->reserved[i]=0;
    __atomic_add_fetch(&r->sequence,1,__ATOMIC_RELEASE);
done:
    __atomic_store_n(&r->lock,0,__ATOMIC_RELEASE);
}
