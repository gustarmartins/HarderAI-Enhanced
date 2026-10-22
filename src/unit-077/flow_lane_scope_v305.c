#include "flow_lane_scope_v305.h"
static void clear(void *p,unsigned n){for(unsigned i=0;i<n;i++)((unsigned char *)p)[i]=0;}
static unsigned same(const void *a,const void *b,unsigned n){
    for(unsigned i=0;i<n;i++)if(((const unsigned char *)a)[i]!=((const unsigned char *)b)[i])return 0;return 1;
}
unsigned lab_flow_lane_scope_pin(const FlowLaneNativeFrame *f,FlowLaneLifetimePin *out,const FlowLaneScopeLease *lease){
    if(!out)return 0;clear(out,sizeof(*out));if(!f)return 0;
    (void)lease;return lab_flow_lane_lifetime_pin(f,out);
}
unsigned lab_flow_lane_scope_same(const FlowLaneNativeFrame *f,const FlowLaneLifetimePin *pin,const FlowLaneScopeLease *lease){
    FlowLaneLifetimePin now;return pin&&lab_flow_lane_scope_pin(f,&now,lease)&&same(pin,&now,sizeof(now));
}
unsigned lab_flow_lane_scope_final(const FlowLaneNativeEntry *entry,const FlowLaneNativeFrame *prior,
                                  const FlowLaneLifetimePin *pin,const FlowLaneScopeLease *lease){
    if(!entry||!prior||!pin||!lab_flow_lane_scope_same(prior,pin,lease))return 0;
    FlowLaneNativeFrame now;
    return lab_flow_lane_native_binding_collect(entry,&now)==FLN_COLLECTED&&same(prior,&now,sizeof(now))&&
           lab_flow_lane_scope_same(&now,pin,lease);
}
