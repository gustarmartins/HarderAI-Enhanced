#ifndef LAB_FLOW_LANE_OBSTACLE_GROUPS_V305_H
#define LAB_FLOW_LANE_OBSTACLE_GROUPS_V305_H
#define FLOG_WIDTH 8
#define FLOG_GROUPS (FLDC_ENTRIES/FLOG_WIDTH)
typedef struct {
    double minimum[3],maximum[3],extra;
} FlowObstacleGroup;
typedef struct {
    unsigned ready,count;
    FlowObstacleGroup group[FLOG_GROUPS];
} FlowObstacleGroups;
#endif
