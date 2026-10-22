#include "flow_lane_native_lifetime_v305.h"
typedef struct {
    fln_u64 magic;unsigned abi,size;
    fln_u64 sequence,generation,scene,system,race,group,scene_generation;
    unsigned lock,active,fault,reserved;
} FlowLaneComLifetime;
_Static_assert(sizeof(FlowLaneComLifetime)==88,"COM lifetime ABI2");
extern FlowLaneComLifetime lab_com_lifetime_record;
static void zero(void *p,unsigned n){for(unsigned k=0;k<n;k++)((unsigned char *)p)[k]=0;}
static unsigned equal(const void *a,const void *b,unsigned n){for(unsigned k=0;k<n;k++)if(((const unsigned char *)a)[k]!=((const unsigned char *)b)[k])return 0;return 1;}
static unsigned w(fln_u64 p,unsigned offset){return *(const volatile unsigned *)(p+offset);}
unsigned lab_flow_lane_lifetime_pin(const FlowLaneNativeFrame *frame,FlowLaneLifetimePin *out){
    if(!out)return 0;zero(out,sizeof(*out));if(!frame)return 0;
    const FlowLaneComLifetime *r=&lab_com_lifetime_record;
    fln_u64 sequence=__atomic_load_n(&r->sequence,__ATOMIC_ACQUIRE);
    if(sequence<6||(sequence&1))return 0;
#define READ(field) __atomic_load_n(&r->field,__ATOMIC_RELAXED)
    FlowLaneLifetimePin pin={sequence,READ(generation),READ(scene_generation),READ(scene),READ(system),READ(race),READ(group)};
    if(READ(magic)!=0x31464c4d4f434148ull||READ(abi)!=2||READ(size)!=88||READ(lock)||READ(active)!=1||READ(fault)||READ(reserved)||
       !pin.scene_generation||pin.generation<=pin.scene_generation||pin.scene!=frame->scene||pin.system!=frame->system||pin.race!=frame->race||pin.group!=frame->group)return 0;
#undef READ
    __atomic_thread_fence(__ATOMIC_ACQUIRE);
    if(sequence!=__atomic_load_n(&r->sequence,__ATOMIC_ACQUIRE)||
       __atomic_load_n(&r->fault,__ATOMIC_ACQUIRE)||__atomic_load_n(&r->lock,__ATOMIC_ACQUIRE)||
       __atomic_load_n(&r->active,__ATOMIC_ACQUIRE)!=1)return 0;
    *out=pin;return 1;
}
unsigned lab_flow_lane_lifetime_same(const FlowLaneNativeFrame *frame,const FlowLaneLifetimePin *pin){
    FlowLaneLifetimePin current;return pin&&lab_flow_lane_lifetime_pin(frame,&current)&&equal(pin,&current,sizeof(current));
}
unsigned lab_flow_lane_native_final_recheck(const FlowLaneNativeEntry *entry,const FlowLaneNativeFrame *prior,const FlowLaneLifetimePin *pin){
    if(!entry||!prior||!pin||!lab_flow_lane_lifetime_same(prior,pin))return 0;
    FlowLaneNativeFrame now;
    return lab_flow_lane_native_binding_collect(entry,&now)==FLN_COLLECTED&&equal(prior,&now,sizeof(now))&&lab_flow_lane_lifetime_same(&now,pin);
}
unsigned lab_flow_lane_geometry_prepare(const FlowLaneNativeFrame *frame,const FlowLaneLifetimePin *pin,const FlowLaneMesh *mesh,FlowLaneGeometryCache *cache){
    if(!frame||!pin||!mesh||!cache||!lab_flow_lane_lifetime_same(frame,pin))return 0;
    unsigned expected=0;
    if(!__atomic_compare_exchange_n(&cache->lock,&expected,1,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED))return 0;
    unsigned success=0;FlowLaneMesh current;
    if(mesh->course_data!=frame->course_data||!lab_flow_mesh_open(frame->course_data,&current)||!equal(mesh,&current,sizeof(current)))goto done;
    unsigned reuse=cache->valid&&equal(&cache->pin,pin,sizeof(*pin))&&equal(&cache->mesh,mesh,sizeof(*mesh));
    for(unsigned i=0;i<mesh->model_count;i++){
        fln_u64 model=mesh->models[i].model;
        if(w(model,0x20)!=w(model,0x168))goto done;
        if(cache->thickness[i]!=w(model,0x20)||cache->radius[i]!=w(model,0x48))reuse=0;
    }
    if(reuse){
        if(lab_flow_lane_lifetime_same(frame,pin)){cache->hits++;success=1;}
        goto done;
    }
    cache->valid=0;cache->computations++;
    if(!lab_flow_lane_geometry_is_stock_bigblue(mesh,cache->digest)||!lab_flow_lane_lifetime_same(frame,pin)||
       !lab_flow_mesh_open(frame->course_data,&current)||!equal(mesh,&current,sizeof(current)))goto done;
    cache->pin=*pin;cache->mesh=*mesh;
    for(unsigned i=0;i<mesh->model_count;i++){cache->thickness[i]=w(mesh->models[i].model,0x20);cache->radius[i]=w(mesh->models[i].model,0x48);}
    if(!lab_flow_lane_lifetime_same(frame,pin))goto done;
    cache->valid=1;success=1;
done:
    __atomic_store_n(&cache->lock,0,__ATOMIC_RELEASE);return success;
}
unsigned lab_flow_lane_geometry_admit(const FlowLaneNativeFrame *frame,const FlowLaneLifetimePin *pin,const FlowLaneMesh *mesh,FlowLaneGeometryCache *cache){
    if(!frame||!pin||!mesh||!cache||!lab_flow_lane_lifetime_same(frame,pin))return 0;
    unsigned expected=0;
    if(!__atomic_compare_exchange_n(&cache->lock,&expected,1,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED))return 0;
    unsigned success=0;FlowLaneMesh current;
    if(!cache->valid||!equal(&cache->pin,pin,sizeof(*pin))||!equal(&cache->mesh,mesh,sizeof(*mesh))||
       mesh->course_data!=frame->course_data||!lab_flow_mesh_open(frame->course_data,&current)||!equal(mesh,&current,sizeof(current)))goto done;
    for(unsigned i=0;i<mesh->model_count;i++){
        fln_u64 model=mesh->models[i].model;
        if(cache->thickness[i]!=w(model,0x20)||cache->thickness[i]!=w(model,0x168)||cache->radius[i]!=w(model,0x48))goto done;
    }
    if(lab_flow_lane_lifetime_same(frame,pin)){cache->hits++;success=1;}
done:
    __atomic_store_n(&cache->lock,0,__ATOMIC_RELEASE);return success;
}
