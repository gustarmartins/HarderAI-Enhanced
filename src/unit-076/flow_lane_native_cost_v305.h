#ifndef LAB_FLOW_LANE_NATIVE_COST_V305_H
#define LAB_FLOW_LANE_NATIVE_COST_V305_H
#define FLOW_NATIVE_COST_SEGMENTS 192
typedef struct {float distance,direction[3],flow[3];unsigned flags;} FlowLaneCostSegment;
typedef struct {float offset;unsigned count;FlowLaneCostSegment segments[FLOW_NATIVE_COST_SEGMENTS];} FlowLaneCostPath;
typedef struct {unsigned choice;float baseline_ticks,selected_ticks,gain_ticks;} FlowLaneCostResult;
/* Bounded extension of the established eight-segment flow forecast. It retains
 * exact native smoothing/deadband/decay and constant-speed assumption. Input
 * paths are caller-owned, already validated geometry/contact/flow forecasts.
 * This is expected travel cost, not a guarantee of future native trajectory. */
float lab_flow_lane_native_cost_arrival(float,const float previous[3],const FlowLaneCostPath *);
unsigned lab_flow_lane_native_cost_rank(float,const float previous[3],const FlowLaneCostPath *,unsigned,FlowLaneCostResult *);
#endif
