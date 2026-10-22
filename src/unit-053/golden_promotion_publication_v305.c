#include "golden_promotion_publication_v305.h"
#define EXPORT __attribute__((visibility("default")))
#define LOAD(p) __atomic_load_n((p),__ATOMIC_RELAXED)
#define STORE(p,v) __atomic_store_n((p),(v),__ATOMIC_RELAXED)
typedef union {gpp_u32 bits;float value;} GPPFloat;
_Static_assert(sizeof(GoldenPromotionCell)==32,"promotion cell ABI");
_Static_assert(sizeof(GoldenPromotionView)==408,"promotion view ABI");
_Static_assert(sizeof(GoldenPromotionPublication)==424,"promotion publication ABI");

static gpp_u32 valid(const GoldenPromotionView *v){
    if(!v||!v->epoch||!v->generation||v->tick>=0x80000000u||v->mask>0xfffu)return 0;
    gpp_u32 winners=0;
    for(gpp_u32 i=0;i<12;i++){
        const GoldenPromotionCell *a=&v->actors[i];GPPFloat f={.bits=a->strength_bits};
        if(a->reserved || (a->strength_bits&0x7fffffffu)>=0x7f800000u || f.value<0 || f.value>1)return 0;
        if(v->mask&(1u<<i)){
            if(!a->driver||!a->role||!a->inventory)return 0;
            winners+=f.value>0;
        }else if(a->driver||a->role||a->inventory||a->strength_bits)return 0;
    }
    return winners<=1;
}
static gpp_u32 begin(GoldenPromotionPublication *p,gpp_u64 *sequence){
    if(!p||__atomic_load_n(&p->fault,__ATOMIC_ACQUIRE))return 0;
    gpp_u32 zero=0;
    if(!__atomic_compare_exchange_n(&p->writer,&zero,1,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED)){
        __atomic_store_n(&p->fault,1,__ATOMIC_RELEASE);return 0;
    }
    *sequence=LOAD(&p->sequence);
    if((*sequence&1)||*sequence>~0ull-2||__atomic_load_n(&p->fault,__ATOMIC_ACQUIRE)){
        __atomic_store_n(&p->fault,1,__ATOMIC_RELEASE);
        __atomic_store_n(&p->writer,0,__ATOMIC_RELEASE);return 0;
    }
    __atomic_add_fetch(&p->sequence,1,__ATOMIC_ACQ_REL);
    /* Pair through a payload atomic read with the reader's acquire fence.
     * Observing newer payload must also order the preceding odd sequence. */
    __atomic_thread_fence(__ATOMIC_RELEASE);return 1;
}
static void finish(GoldenPromotionPublication *p,gpp_u64 sequence){
    __atomic_store_n(&p->sequence,sequence+2,__ATOMIC_RELEASE);
    __atomic_store_n(&p->writer,0,__ATOMIC_RELEASE);
}
EXPORT gpp_u32 lab_golden_promotion_publish(GoldenPromotionPublication *p,const GoldenPromotionView *v){
    if(!p||!v)return 0;
    gpp_u64 pp=(gpp_u64)p,vp=(gpp_u64)v;
    if((vp>=pp&&vp-pp<sizeof(*p))||(pp>=vp&&pp-vp<sizeof(*v)))return 0;
    if(!valid(v))return 0;
    gpp_u64 sequence;if(!begin(p,&sequence))return 0;
    STORE(&p->value.epoch,v->epoch);STORE(&p->value.generation,v->generation);
    STORE(&p->value.tick,v->tick);STORE(&p->value.mask,v->mask);
    for(gpp_u32 i=0;i<12;i++){
        GoldenPromotionCell *z=&p->value.actors[i];const GoldenPromotionCell *a=&v->actors[i];
        STORE(&z->driver,a->driver);STORE(&z->role,a->role);STORE(&z->inventory,a->inventory);
        STORE(&z->strength_bits,a->strength_bits);STORE(&z->reserved,0);
    }
    finish(p,sequence);return !__atomic_load_n(&p->fault,__ATOMIC_ACQUIRE);
}
EXPORT void lab_golden_promotion_unpublish(GoldenPromotionPublication *p){
    gpp_u64 sequence;if(!begin(p,&sequence))return;
    /* Clear authority only. Readers never follow the retained scalar tokens. */
    STORE(&p->value.epoch,0);STORE(&p->value.generation,0);STORE(&p->value.mask,0);
    finish(p,sequence);
}
EXPORT gpp_u32 lab_golden_promotion_read(const GoldenPromotionPublication *p,gpp_u32 racer,
    gpp_u64 epoch,gpp_u64 generation,gpp_u64 driver,gpp_u64 role,gpp_u64 inventory,
    gpp_u32 tick,float *strength){
    if(!strength)return 0;*strength=0;
    if(!p||racer>=12||!epoch||!generation||!driver||!role||!inventory||tick>=0x80000000u)return 0;
    if(__atomic_load_n(&p->fault,__ATOMIC_ACQUIRE))return 0;
    gpp_u64 sequence=__atomic_load_n(&p->sequence,__ATOMIC_ACQUIRE);
    if(!sequence||(sequence&1))return 0;
    gpp_u64 e=LOAD(&p->value.epoch),g=LOAD(&p->value.generation);
    gpp_u32 at=LOAD(&p->value.tick),mask=LOAD(&p->value.mask);
    const GoldenPromotionCell *a=&p->value.actors[racer];
    gpp_u64 d=LOAD(&a->driver),r=LOAD(&a->role),inv=LOAD(&a->inventory);
    GPPFloat f={.bits=LOAD(&a->strength_bits)};gpp_u32 reserved=LOAD(&a->reserved);
    __atomic_thread_fence(__ATOMIC_ACQUIRE);
    if(sequence!=__atomic_load_n(&p->sequence,__ATOMIC_ACQUIRE)||
       __atomic_load_n(&p->fault,__ATOMIC_ACQUIRE)||e!=epoch||g!=generation||
       !(mask&(1u<<racer))||at>tick||tick-at>2||d!=driver||r!=role||inv!=inventory||
       reserved||(f.bits&0x7fffffffu)>=0x7f800000u||f.value<0||f.value>1)return 0;
    *strength=f.value;return 1;
}
