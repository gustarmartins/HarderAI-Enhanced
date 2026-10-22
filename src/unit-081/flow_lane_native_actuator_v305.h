#ifndef LAB_FLOW_LANE_NATIVE_ACTUATOR_V305_H
#define LAB_FLOW_LANE_NATIVE_ACTUATOR_V305_H
#include "flow_lane_native_lifetime_v305.h"
#include "flow_lane_native_facet_route_v305.h"
#include "flow_lane_native_route_obstacles_v305.h"
#include "flow_lane_native_cost_v305.h"
#include "flow_lane_native_flow_query_v305.h"
#include "flow_lane_sealed_query_v305.h"
#include "flow_lane_owned_query_v305.h"
#include "flow_course_publication_v305.h"
#include "flow_lane_prepared_geometry_v305.h"
typedef struct {
    unsigned lock,reserved;
    FlowLaneNativeFrame frame;
    FlowLaneLifetimePin pin;
    FlowLaneMesh mesh;
    FlowLaneFacetWorkspace route;
    FlowLaneCostPath paths[2];
    FlowLaneRouteObstacles obstacles;
    FlowLaneOwnedFlow flow_seal;
    FlowCoursePin course_pin;
    FlowLaneDecodeCache decode_cache;
    FlowPreparedGeometry prepared_geometry;
} FlowLaneActuatorWorkspace;
typedef struct {
    unsigned status,route_status,flow_queries,point_queries;
    float original_offset,selected_offset;
    FlowLaneCostResult cost;
} FlowLaneActuatorResult;
enum {FLA_INVALID=0,FLA_OWNER=1,FLA_LIFETIME=2,FLA_MESH=3,FLA_COLD=4,
      FLA_PROPOSAL=5,FLA_MOTION=6,FLA_ROUTE=7,FLA_COST=8,FLA_CHANGED=9,FLA_COMMITTED=10,FLA_BUSY=11};
/* Private ordinary7BE418 callback only. Caller retains native job/allocation
 * ownership; the real COM lifecycle and a separately prepared immutable course
 * cache must admit. This function never hashes in the callback. It changes
 * exactly route+98 after full recheck; caller then replays native arithmetic.
 * Root integration must leave the feature disabled until paired ownership,
 * workload and actual gameplay qualification are complete. */
unsigned lab_flow_lane_native_actuate(const FlowLaneNativeEntry *,FlowLaneGeometryCache *,
    FlowLaneActuatorWorkspace *,FlowLaneActuatorResult *);
#endif
