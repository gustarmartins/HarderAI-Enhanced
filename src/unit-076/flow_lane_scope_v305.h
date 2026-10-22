#ifndef LAB_FLOW_LANE_SCOPE_V305_H
#define LAB_FLOW_LANE_SCOPE_V305_H
#include "flow_lane_native_lifetime_v305.h"
typedef struct {const void *call,*storage;} FlowLaneScopeLease;
/* Ordinary callbacks use the real COM publication. Identity callbacks must
 * already hold both existing callback and private-storage leases before any
 * native or reclaimable descriptor reads. Neither replaces native job life. */
unsigned lab_flow_lane_scope_pin(const FlowLaneNativeFrame *,FlowLaneLifetimePin *,const FlowLaneScopeLease *);
unsigned lab_flow_lane_scope_same(const FlowLaneNativeFrame *,const FlowLaneLifetimePin *,const FlowLaneScopeLease *);
unsigned lab_flow_lane_scope_final(const FlowLaneNativeEntry *,const FlowLaneNativeFrame *,
                                  const FlowLaneLifetimePin *,const FlowLaneScopeLease *);
#endif
