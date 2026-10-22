#ifndef LAB_FLOW_LANE_NATIVE_FACET_ROUTE_V305_H
#define LAB_FLOW_LANE_NATIVE_FACET_ROUTE_V305_H
#include "flow_lane_decode_cache_v305.h"
#include "flow_lane_native_edge_certificate_v305.h"
#define FLFR_FACETS 96
#define FLFR_SEGMENTS 192
typedef struct {unsigned model,prism;} FlowLanePrismRef;
typedef struct {
    float a[3],b[3],up[3],local_offset_y,radius;
    unsigned reserved;
} FlowLaneFacetInput;
typedef struct {
    double begin,end,p0[3],velocity[3];
    float normal[3];
    FlowLanePrismRef ref;
} FlowLaneNativeFacet;
typedef struct {
    float a[3],b[3];
    FlowLanePrismRef ref;
    unsigned seam,reserved;
} FlowLaneFacetSegment;
typedef struct {
    unsigned status,facets,segments,failed_segment;
    unsigned contact_queries,contact_intervals,branches,reserved;
    FlowLaneNativeFacet facet[FLFR_FACETS];
    FlowLaneFacetSegment segment[FLFR_SEGMENTS];
    double boundary[2*FLFR_FACETS+2];
} FlowLaneFacetWorkspace;
enum {FLFR_INVALID=0,FLFR_READY=1,FLFR_BUDGET=2,FLFR_ENDPOINT=3,
      FLFR_BANK=4,FLFR_AMBIGUOUS=5,FLFR_CONTACT_GAP=6};
/* Candidate construction from real immutable prism planes, followed by
 * continuous native sphere-contact coverage. The caller provides borrowed
 * mesh/index storage under its actual native lifetime and owns workspace.
 * No stack-sized workspace, heap allocation, global mutable scratch or writes
 * to native state. Ambiguous separated projected floors refuse. This proves
 * support for a candidate polyline; actual overlapping native response winners,
 * full-radius obstacles, movement feasibility and flow cost remain separate.
 * It is never sufficient authority for route+98 commit. */
unsigned lab_flow_lane_native_facet_route(const FlowLaneMesh *,
    const FlowLanePrismRef *,unsigned,const FlowLaneFacetInput *,FlowLaneFacetWorkspace *,FlowLaneDecodeCache *);
#endif
