#ifndef LAB_FLOW_LANE_NATIVE_ROUTE_OBSTACLES_V305_H
#define LAB_FLOW_LANE_NATIVE_ROUTE_OBSTACLES_V305_H
#include "flow_lane_decode_cache_v305.h"
#include "flow_lane_native_facet_route_v305.h"
#include "flow_lane_prepared_geometry_v305.h"
typedef struct {
    unsigned status,decoded,nearby,distance_queries;
    unsigned obstacle_model,obstacle_prism,segment,reserved;
    double distance_squared;
} FlowLaneRouteObstacles;
/* Continuous full-radius sweep of the supplied sphere-center polyline.
 * Complete candidate-index coverage and current immutable mesh lifetime remain
 * caller obligations. Ordinary and flow floors may be intended contacts only
 * when below the center and aligned with the segment's supporting facet.
 * Every other material remains an obstacle candidate. Native finite prism
 * thickness is conservatively enclosed; native sphere misses never grant.
 * This does not prove physical steering follows the proposed polyline. */
unsigned lab_flow_lane_route_obstacles(const FlowLaneMesh *,const FlowLanePrismRef *,unsigned,
    const FlowLaneFacetWorkspace *,float radius,FlowLaneRouteObstacles *,FlowLaneDecodeCache *,const FlowPreparedGeometry *);
#endif
