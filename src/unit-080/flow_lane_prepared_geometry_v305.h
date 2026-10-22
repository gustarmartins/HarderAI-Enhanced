#ifndef LAB_FLOW_LANE_PREPARED_GEOMETRY_V305_H
#define LAB_FLOW_LANE_PREPARED_GEOMETRY_V305_H
#include "flow_course_publication_v305.h"
#include "flow_lane_decode_cache_v305.h"
#include "flow_lane_obstacle_groups_v305.h"
typedef struct {
    unsigned state,reserved,ref_count,padding;
    flm_u64 preparations,reuses,mismatches;
    FlowCoursePin pin;
    FlowLaneMesh mesh;
    unsigned pairs[2*FLDC_ENTRIES];
    FlowObstacleGroups obstacle_groups;
} FlowPreparedGeometry;
enum {FLPG_EMPTY=0,FLPG_READY=1,FLPG_CHANGED=2};
/* Caller owns the same workspace lock for begin, calculation, finish/abort.
 * A matching generation still requires fresh live-input verification. A
 * changed snapshot stays rejected until a different course publication. */
unsigned lab_flow_prepared_begin(const FlowCoursePin *,const FlowLaneMesh *,
    const unsigned *,unsigned,FlowPreparedGeometry *,FlowLaneDecodeCache *);
unsigned lab_flow_prepared_finish(FlowPreparedGeometry *,FlowLaneDecodeCache *);
/* The course-load producer tries that workspace lock; contention leaves the
 * active calculator untouched. A later qualified callback can prepare cold. */
unsigned lab_flow_prepared_publish(const FlowCoursePin *,const FlowLaneMesh *,
    const unsigned *,unsigned,unsigned *,FlowPreparedGeometry *,FlowLaneDecodeCache *);
void lab_flow_course_prepare_geometry(const FlowCoursePin *,const FlowLaneMesh *);
unsigned lab_flow_obstacle_groups_match(const FlowPreparedGeometry *,const FlowLaneMesh *,const unsigned *,unsigned,const FlowLaneDecodeCache *);
unsigned lab_flow_obstacle_group_skip(const FlowPreparedGeometry *,unsigned,const double [3],const double [3],float);
#endif
