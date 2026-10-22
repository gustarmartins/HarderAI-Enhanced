#ifndef LAB_FLOW_LANE_NATIVE_LIFETIME_V305_H
#define LAB_FLOW_LANE_NATIVE_LIFETIME_V305_H
#include "flow_lane_native_binding.h"
#include "flow_lane_native_geometry_identity_v305.h"
typedef struct {
    fln_u64 sequence,generation,scene_generation,scene,system,race,group;
} FlowLaneLifetimePin;
/* COM ABI2 reader composed with the existing producer. Observer generations
 * are deliberately not accepted. Identity mode requires its independent
 * identity_call admission integration and refuses in this initial adapter. */
unsigned lab_flow_lane_lifetime_pin(const FlowLaneNativeFrame *,FlowLaneLifetimePin *);
unsigned lab_flow_lane_lifetime_same(const FlowLaneNativeFrame *,const FlowLaneLifetimePin *);
unsigned lab_flow_lane_native_final_recheck(const FlowLaneNativeEntry *,const FlowLaneNativeFrame *,const FlowLaneLifetimePin *);
typedef struct {
    unsigned lock,valid;
    FlowLaneLifetimePin pin;
    FlowLaneMesh mesh;
    unsigned thickness[FLM_MODELS],radius[FLM_MODELS];
    unsigned char digest[32];
    fln_u64 computations,hits;
} FlowLaneGeometryCache;
/* Module-owned cache; serialized by a nonblocking lock. It borrows current
 * callback allocation lifetime, pins real COM sequence/generations, compares
 * fresh mesh/model bindings, and rechecks before publishing/using identity.
 * Native geometry arrays must remain immutable for that course lifetime.
 * A future course editor must invalidate this cache before any mesh mutation.
 * No pointers from an expired cache are dereferenced on a new lifetime.
 * prepare is restricted to an owned course-allocation/initialization boundary;
 * admit never hashes and is suitable for a per-racer callback. A cold cache
 * causes native fallback instead of doing fingerprint work in the hot path. */
unsigned lab_flow_lane_geometry_prepare(const FlowLaneNativeFrame *,const FlowLaneLifetimePin *,const FlowLaneMesh *,FlowLaneGeometryCache *);
unsigned lab_flow_lane_geometry_admit(const FlowLaneNativeFrame *,const FlowLaneLifetimePin *,const FlowLaneMesh *,FlowLaneGeometryCache *);
#endif
