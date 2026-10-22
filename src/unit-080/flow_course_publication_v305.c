#include "flow_course_publication_v305.h"
#include "flow_lane_prepared_geometry_v305.h"
#include "flow_lane_native_geometry_identity_v305.h"
typedef flm_u64 u64;
typedef flm_u32 u32;
#define MAGIC 0x31434c464941484cull
#define EXPORT __attribute__((visibility("default")))
_Static_assert(sizeof(FlowCoursePin)==56,"Course pin ABI");
_Static_assert(sizeof(FlowCoursePublication)==2472,"Course publication ABI");
EXPORT FlowCoursePublication lab_flow_course_publication={.magic=MAGIC,.abi=1,.size=2472};
static u64 q(u64 p,u32 o){return *(const volatile u64 *)(p+o);}
static u32 w(u64 p,u32 o){return *(const volatile u32 *)(p+o);}
static u32 ptr(u64 p,u32 size){return p>=0x10000&&!(p&7)&&p<(1ull<<39)-size;}
static u32 equal(const void *a,const void *b,u32 size){
    const unsigned char *x=a,*y=b;for(u32 k=0;k<size;k++)if(x[k]!=y[k])return 0;return 1;
}
static u32 header(const FlowCoursePublication *r){
    return r&&r->magic==MAGIC&&r->abi==1&&r->size==sizeof(*r)&&!r->reserved;
}
static u32 acquire(FlowCoursePublication *r){
    u32 zero=0;return __atomic_compare_exchange_n(&r->lock,&zero,1,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED);
}
static void release(FlowCoursePublication *r){__atomic_store_n(&r->lock,0,__ATOMIC_RELEASE);}
static u32 bound(u64 base,u64 owner,u64 *wrapper,u64 *data,u64 *raw){
    if(!ptr(owner,0x70))return 0;*wrapper=q(owner,0x68);
    if(!ptr(*wrapper,0x3a8)||q(*wrapper,0)!=base+0x12d0060||
       (w(*wrapper,0x28)&7))return 0;
    *data=q(*wrapper,0x370);
    if(!ptr(*data,0xa8)||q(*data,0)!=base+0x12c1b00||
       *(const volatile unsigned char *)(*data+0x88)!=1)return 0;
    *raw=q(*data,0x40);
    return ptr(*raw,0x40)&&q(*data,0x78)==*raw&&w(*raw,0)==0x202;
}

EXPORT void lab_flow_course_event(FlowCoursePublication *r,u64 base,u32 event,u64 object){
    if(!header(r)||(base&4095)||base>(1ull<<39)-0x1400000||
       event<FCP_BEGIN||event>FCP_FACTORY_RELEASE||
       ((event!=FCP_FACTORY_RESET&&event!=FCP_FACTORY_RELEASE)&&!ptr(object,0x70)))return;
    if(!acquire(r)){__atomic_store_n(&r->fault,1,__ATOMIC_RELEASE);return;}
    if(r->pin.sequence>~0ull-2||r->pin.generation==~0ull){
        r->phase=FCP_COLD;__atomic_store_n(&r->fault,2,__ATOMIC_RELEASE);goto done;
    }
    if(event==FCP_BEGIN){
        __atomic_add_fetch(&r->pin.sequence,1,__ATOMIC_ACQ_REL);
        r->pin.generation++;r->pin.main_base=base;r->pin.owner=object;
        r->pin.wrapper=r->pin.course_data=r->pin.header=0;r->phase=FCP_LOADING;
        __atomic_store_n(&r->fault,0,__ATOMIC_RELEASE);
        __atomic_add_fetch(&r->pin.sequence,1,__ATOMIC_RELEASE);goto done;
    }
    if(base!=r->pin.main_base)goto done;
    if(event==FCP_PUBLISH){
        if(object!=r->pin.owner||r->phase!=FCP_LOADING||__atomic_load_n(&r->fault,__ATOMIC_ACQUIRE))goto done;
        __atomic_add_fetch(&r->pin.sequence,1,__ATOMIC_ACQ_REL);
        r->phase=FCP_OTHER;
        u64 wrapper,data,raw;FlowLaneMesh mesh,after;
        if(!bound(base,object,&wrapper,&data,&raw)||!lab_flow_mesh_open(data,&mesh)||
           mesh.model_count!=18||mesh.total_prisms!=54153)goto published;
        for(u32 k=0;k<mesh.model_count;k++)if(w(mesh.models[k].model,0x20)!=w(mesh.models[k].model,0x168))goto published;
        r->computations++;
        if(!lab_flow_lane_geometry_is_stock_bigblue(&mesh,r->digest))goto published;
        u64 next_wrapper,next_data,next_raw;
        if(__atomic_load_n(&r->fault,__ATOMIC_ACQUIRE)||!bound(base,object,&next_wrapper,&next_data,&next_raw)||
           wrapper!=next_wrapper||data!=next_data||raw!=next_raw||
           !lab_flow_mesh_open(data,&after)||!equal(&mesh,&after,sizeof(mesh)))goto published;
        r->mesh=mesh;r->pin.wrapper=wrapper;r->pin.course_data=data;r->pin.header=raw;
        for(u32 k=0;k<mesh.model_count;k++){
            r->thickness[k]=w(mesh.models[k].model,0x20);
            r->radius[k]=w(mesh.models[k].model,0x48);
            if(r->thickness[k]!=w(mesh.models[k].model,0x168))goto published;
        }
        if(!__atomic_load_n(&r->fault,__ATOMIC_ACQUIRE))r->phase=FCP_READY;
published:
        __atomic_add_fetch(&r->pin.sequence,1,__ATOMIC_RELEASE);
        if(r->phase==FCP_READY&&!__atomic_load_n(&r->fault,__ATOMIC_ACQUIRE))
            lab_flow_course_prepare_geometry(&r->pin,&r->mesh);
        goto done;
    }
    if(event==FCP_FACTORY_RELEASE||(event==FCP_FACTORY_RESET&&r->phase!=FCP_LOADING)||
       (event==FCP_RETIRE&&object==r->pin.owner)||
       ((event==FCP_DATA_RELOAD||event==FCP_DATA_DELETE)&&object==r->pin.course_data)||
       (event==FCP_WRAPPER_DELETE&&object==r->pin.wrapper)){
        __atomic_add_fetch(&r->pin.sequence,1,__ATOMIC_ACQ_REL);
        r->phase=FCP_COLD;
        __atomic_add_fetch(&r->pin.sequence,1,__ATOMIC_RELEASE);
    }
done: release(r);
}

EXPORT u32 lab_flow_course_admit(FlowCoursePublication *r,u64 base,u64 wrapper,
                                 const FlowLaneMesh *mesh,FlowCoursePin *out){
    if(!out)return 0;*out=(FlowCoursePin){0};
    if(!header(r)||!mesh||!acquire(r))return 0;
    u32 accepted=0;
    /* Match current owner pointers before touching any cached native address.
     * A retired/new generation never follows its previous mesh bindings. */
    if(r->phase!=FCP_READY||__atomic_load_n(&r->fault,__ATOMIC_ACQUIRE)||!r->pin.generation||r->pin.sequence<4||
       (r->pin.sequence&1)||r->pin.main_base!=base||r->pin.wrapper!=wrapper||
       r->pin.course_data!=mesh->course_data||!equal(mesh,&r->mesh,sizeof(*mesh)))goto done;
    u64 current_wrapper,data,raw;FlowLaneMesh current;
    if(!bound(base,r->pin.owner,&current_wrapper,&data,&raw)||current_wrapper!=wrapper||
       data!=mesh->course_data||raw!=r->pin.header||!lab_flow_mesh_open(data,&current)||
       !equal(mesh,&current,sizeof(*mesh)))goto done;
    for(u32 k=0;k<mesh->model_count;k++){
        u64 p=mesh->models[k].model;
        if(r->thickness[k]!=w(p,0x20)||r->thickness[k]!=w(p,0x168)||r->radius[k]!=w(p,0x48))goto done;
    }
    if(__atomic_load_n(&r->fault,__ATOMIC_ACQUIRE))goto done;
    *out=r->pin;r->hits++;accepted=1;
done: release(r);return accepted;
}

EXPORT u32 lab_flow_course_same(FlowCoursePublication *r,const FlowCoursePin *pin){
    if(!header(r)||!pin||!acquire(r))return 0;
    u32 accepted=r->phase==FCP_READY&&!__atomic_load_n(&r->fault,__ATOMIC_ACQUIRE)&&equal(&r->pin,pin,sizeof(*pin));
    release(r);return accepted;
}
