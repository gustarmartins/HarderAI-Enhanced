#ifndef LAB_FLOW_LANE_NATIVE_BINDING_MESH_H
#define LAB_FLOW_LANE_NATIVE_BINDING_MESH_H
#include "flow_lane_native_binding_geometry.h"
typedef unsigned int flm_u32;
typedef unsigned long long flm_u64;
#define FLM_MODELS 32
#define FLM_TOTAL_PRISMS 65535
typedef struct {
    flm_u64 model,vertices,normals,prisms,tree,categories;
    flm_u32 vertex_count,normal_count,prism_count,reserved;
} FlowLaneMeshModel;
typedef struct {
    flm_u64 course_data,header,table;
    flm_u32 model_count,total_prisms;
    FlowLaneMeshModel models[FLM_MODELS];
} FlowLaneMesh;
typedef struct {
    FlowLaneTriangle triangle;
    float normal[3];
    flm_u32 material,category,response_bit,model_index,prism_index,reserved[3];
} FlowLaneMeshTriangle;
typedef struct { flm_u32 model,prism,emitted,reserved; } FlowLaneMeshCursor;
enum { FLM_INVALID=0,FLM_VALUE=1,FLM_END=2 };
/* Borrowed immutable native storage. Caller owns current static-wrapper,
 * identity-transform and course allocation lifetime for every access. Pointer
 * bounds here do not replace that lease. No collision/clearance verdict. */
flm_u32 lab_flow_mesh_open(flm_u64 course_data,FlowLaneMesh *);
flm_u32 lab_flow_mesh_decode(const FlowLaneMeshModel *,flm_u32 index,FlowLaneMeshTriangle *);
/* Zero-initialize cursor. Exhaustively enumerates every index in every model;
 * END only after exactly total_prisms values. No octree or material filtering. */
flm_u32 lab_flow_mesh_next(const FlowLaneMesh *,FlowLaneMeshCursor *,FlowLaneMeshTriangle *);
/* Exact material-bit partitions observed in native response consumers. They
 * are evidence labels, not permission to treat all floor types as safe road. */
#define FLM_NATIVE_GROUND_MASK 0x2601ffffu
#define FLM_NATIVE_WALL_MASK   0x01fe0000u
#define FLM_NATIVE_FLOW_BIT    0x00002000u
#endif
