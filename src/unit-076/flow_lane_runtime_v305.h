#ifndef LAB_FLOW_LANE_RUNTIME_V305_H
#define LAB_FLOW_LANE_RUNTIME_V305_H
#include "flow_lane_native_actuator_v305.h"
#define FLRT_EXPORT __attribute__((visibility("default")))
/* Copied while the actuator owns its frame and workspace. These are historical
 * identifiers, never pointers that the publication routine dereferences. */
typedef struct {
    fln_u64 role,physics,driver,context,race,system,clock,route,current,following;
    fln_u64 lifecycle_generation,scene_generation,course_generation;
    unsigned valid,racer,tick,current_key,following_key,reserved;
} FlowLaneActor;
typedef struct {
    unsigned lock,fault;
    fln_u64 sequence,events,dropped,generation;
    FlowLaneActor actor;
    FlowLaneActuatorResult result;
} FlowLaneDecisionRecord;
_Static_assert(sizeof(FlowLaneActor)==128,"Lane actor snapshot ABI");
_Static_assert(sizeof(FlowLaneDecisionRecord)==208,"Lane decision ABI");
extern unsigned lab_flow_lane_runtime_enable;
extern FlowLaneDecisionRecord lab_flow_lane_decision_records[12];
unsigned lab_flow_lane_native_actuate_observed(const FlowLaneNativeEntry *,
    FlowLaneGeometryCache *,FlowLaneActuatorWorkspace *,FlowLaneActuatorResult *,FlowLaneActor *);
FLRT_EXPORT void lab_flow_lane_runtime_entry(void *,unsigned,unsigned,unsigned,void *);
/* Current host epoch and matching live owner graph are required to interpret
 * these records. Target and physical flow observers remain independent. */
#endif
