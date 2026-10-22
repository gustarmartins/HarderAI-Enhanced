/* The COM ABI2 and identity admission/storage contracts match the existing
 * qualified flow-lane scope. This adapter has no geometry/route dependencies. */
#include "native_ground_target_scope_v305.h"
static void clear(void *p,unsigned n){for(unsigned i=0;i<n;i++)((unsigned char *)p)[i]=0;}
static unsigned same(const void *a,const void *b,unsigned n){
    for(unsigned i=0;i<n;i++)if(((const unsigned char *)a)[i]!=((const unsigned char *)b)[i])return 0;return 1;
}
typedef struct {
    ngs_u64 magic;unsigned abi,size;
    ngs_u64 sequence,generation,scene,system,race,group,scene_generation;
    unsigned lock,active,fault,reserved;
} NativeGroundComLifetime;
_Static_assert(sizeof(NativeGroundComLifetime)==88,"COM lifetime ABI2");
extern NativeGroundComLifetime lab_com_lifetime_record;

unsigned lab_native_ground_scope_enter(NativeGroundLease *l){
    if(!l)return 0;clear(l,sizeof(*l));
    l->held=1;return 1;
}
void lab_native_ground_scope_leave(NativeGroundLease *l){
    if(!l||l->held!=1)return;
    l->held=0;
}

unsigned lab_native_ground_scope_pin(const NativeGroundLease *l,ngs_u64 base,
    ngs_u64 scene,ngs_u64 system,ngs_u64 race,NativeGroundScope *out){
    if(!out)return 0;clear(out,sizeof(*out));if(!l||l->held!=1)return 0;
    (void)base;
    const NativeGroundComLifetime *r=&lab_com_lifetime_record;
    ngs_u64 sequence=__atomic_load_n(&r->sequence,__ATOMIC_ACQUIRE);
    if(sequence<6||(sequence&1))return 0;
#define READ(field) __atomic_load_n(&r->field,__ATOMIC_RELAXED)
    NativeGroundScope pin={sequence,READ(generation),READ(scene_generation),READ(scene),READ(system),READ(race),READ(group)};
    if(READ(magic)!=0x31464c4d4f434148ull||READ(abi)!=2||READ(size)!=88||READ(lock)||READ(active)!=1||READ(fault)||READ(reserved)||
       !pin.epoch||pin.generation<=pin.epoch||pin.scene!=scene||pin.system!=system||pin.race!=race||!pin.group)return 0;
#undef READ
    __atomic_thread_fence(__ATOMIC_ACQUIRE);
    if(sequence!=__atomic_load_n(&r->sequence,__ATOMIC_ACQUIRE)||
       __atomic_load_n(&r->fault,__ATOMIC_ACQUIRE)||__atomic_load_n(&r->lock,__ATOMIC_ACQUIRE)||
       __atomic_load_n(&r->active,__ATOMIC_ACQUIRE)!=1)return 0;
    *out=pin;return 1;
}
unsigned lab_native_ground_scope_same(const NativeGroundLease *l,ngs_u64 base,const NativeGroundScope *prior){
    NativeGroundScope now;
    return prior&&lab_native_ground_scope_pin(l,base,prior->scene,prior->system,prior->race,&now)&&same(&now,prior,sizeof(now));
}
