#ifndef LAB_FLOW_LANE_DECODE_CACHE_V305_H
#define LAB_FLOW_LANE_DECODE_CACHE_V305_H
#include "flow_lane_native_binding_mesh.h"
#include "flow_lane_native_point_contact_v305.h"
#define FLDC_ENTRIES 4096
#define FLDC_BUCKETS 8192
typedef struct {
    flm_u32 prism[5],origin[3],normal[12],category,thickness;
} FlowLanePrismInputs;
typedef struct {
    flm_u64 model;
    flm_u32 prism,envelope_ready;
    FlowLaneMeshModel binding;
    FlowLanePrismInputs input;
    FlowLaneMeshTriangle triangle;
    double envelope;
} FlowLaneDecodedEntry;
typedef struct {
    flm_u32 count,hits,misses,changed,envelope_hits,envelope_misses;
    flm_u32 geometry_ready,flow_count;
    flm_u32 bucket[FLDC_BUCKETS];
    flm_u32 flow_index[FLDC_ENTRIES];
    FlowLaneDecodedEntry entry[FLDC_ENTRIES];
} FlowLaneDecodeCache;
/* Per-callback storage owned by the actuator workspace lock. Reset on every
 * callback; no native pointers survive as cache authority across callbacks.
 * Ordinary mode checks live inputs on each hit. Owned geometry mode prepares
 * every indexed prism, calculates from immutable private inputs, and requires
 * successful final live-input comparison before the caller commits. */
void lab_flow_decode_cache_reset(FlowLaneDecodeCache *);
unsigned lab_flow_mesh_cached_decode(const FlowLaneMeshModel *,unsigned,
    FlowLaneMeshTriangle *,FlowLaneDecodeCache *);
double lab_flow_lane_cached_envelope(const FlowLaneMeshModel *,unsigned,
    const FlowLaneMeshTriangle *,float,FlowLaneDecodeCache *);
unsigned lab_flow_mesh_model_same(const FlowLaneMeshModel *);
double lab_flow_lane_original_envelope(const FlowLaneMeshModel *,unsigned,
    const FlowLaneMeshTriangle *,float);
const FlowLanePrismInputs *lab_flow_geometry_prism(const FlowLaneMeshModel *,unsigned,FlowLaneDecodeCache *);
unsigned lab_flow_geometry_open(const FlowLaneMesh *,const unsigned *,unsigned,FlowLaneDecodeCache *);
unsigned lab_flow_geometry_close(FlowLaneDecodeCache *);
unsigned lab_flow_geometry_point(const FlowLaneMeshModel *,unsigned,const float [3],float,FlowLanePointContact *,FlowLaneDecodeCache *);
unsigned lab_flow_geometry_flow_point(FlowLaneDecodeCache *,unsigned,const float [3],float,FlowLanePointContact *);
unsigned lab_flow_geometry_resume(FlowLaneDecodeCache *);
#endif
