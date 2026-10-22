#include "flow_lane_runtime_v305.h"
#include "flow_lane_prepared_geometry_v305.h"
extern char lab_module_start[];
FLRT_EXPORT const unsigned lab_flow_lane_runtime_config[8]={1,208,128,12,0,0x7be418,0x7be44c,0};
FLRT_EXPORT unsigned lab_flow_lane_runtime_enable;
FLRT_EXPORT FlowLaneDecisionRecord lab_flow_lane_decision_records[12];
#ifdef LAB_FLOW_RUNTIME_PROBE
/* The fixture's legacy global result exists only for comparison with prior
 * replay tools. Production always uses the callback-local result below. */
extern FlowLaneActuatorWorkspace lab_flow_lane_actuator_probe_workspace;
extern FlowLaneGeometryCache lab_flow_lane_actuator_probe_cache;
extern FlowLaneActuatorResult lab_flow_lane_actuator_probe_result;
#define runtime_workspace lab_flow_lane_actuator_probe_workspace
#define runtime_cache lab_flow_lane_actuator_probe_cache
#else
#include "flow_lane_owner_contact_indices.h"
static FlowLaneActuatorWorkspace runtime_workspace;
static FlowLaneGeometryCache runtime_cache;
void lab_flow_course_prepare_geometry(const FlowCoursePin *pin,const FlowLaneMesh *mesh){
    FlowLaneActuatorWorkspace *w=&runtime_workspace;
    lab_flow_prepared_publish(pin,mesh,(const unsigned *)probe_refs,
        sizeof(probe_refs)/sizeof(probe_refs[0]),&w->lock,&w->prepared_geometry,&w->decode_cache);
}
#endif
static unsigned same(const FlowLaneActor *a,const FlowLaneActor *b){
    return a->role==b->role&&a->physics==b->physics&&a->driver==b->driver&&
        a->context==b->context&&a->race==b->race&&a->system==b->system&&
        a->clock==b->clock&&a->route==b->route&&
        a->lifecycle_generation==b->lifecycle_generation&&a->scene_generation==b->scene_generation;
}
static void publish(const FlowLaneActor *actor,const FlowLaneActuatorResult *result){
    if(actor->valid!=1||actor->reserved||actor->racer>=12)return;
    FlowLaneDecisionRecord *r=&lab_flow_lane_decision_records[actor->racer];unsigned zero=0;
    if(!__atomic_compare_exchange_n(&r->lock,&zero,1,1,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED)){
        fln_u64 old=__atomic_load_n(&r->dropped,__ATOMIC_RELAXED);
        if(old!=~0ull)__atomic_compare_exchange_n(&r->dropped,&old,old+1,1,__ATOMIC_RELAXED,__ATOMIC_RELAXED);
        return;
    }
    if(r->fault)goto done;
    unsigned changed=!r->events||!same(&r->actor,actor)||actor->tick<r->actor.tick;
    if((r->sequence&1)||r->sequence>~0ull-2||r->events==~0ull||(changed&&r->generation==~0ull)){
        r->fault=1;goto done;
    }
    __atomic_add_fetch(&r->sequence,1,__ATOMIC_ACQ_REL);
    r->events++;if(changed)r->generation++;
    r->actor=*actor;r->result=*result;
    __atomic_add_fetch(&r->sequence,1,__ATOMIC_RELEASE);
done:
    __atomic_store_n(&r->lock,0,__ATOMIC_RELEASE);
}
FLRT_EXPORT void lab_flow_lane_runtime_entry(void *route,unsigned x,unsigned y,unsigned z,void *sp){
    if(__atomic_load_n(&lab_flow_lane_runtime_enable,__ATOMIC_ACQUIRE)!=1)return;
    FlowLaneNativeEntry entry={(fln_u64)lab_module_start-LAB_MAIN_TO_MODULE,
        (fln_u64)route,(fln_u64)sp,{x,y,z},0};
    FlowLaneActor actor={0};FlowLaneActuatorResult result;
    lab_flow_lane_native_actuate_observed(&entry,&runtime_cache,&runtime_workspace,&result,&actor);
    publish(&actor,&result);
#ifdef LAB_FLOW_RUNTIME_PROBE
    lab_flow_lane_actuator_probe_result=result;
#endif
}
