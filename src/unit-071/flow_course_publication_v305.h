#ifndef LAB_FLOW_COURSE_PUBLICATION_V305_H
#define LAB_FLOW_COURSE_PUBLICATION_V305_H
#include "flow_lane_native_binding_mesh.h"

typedef struct {
    flm_u64 sequence,generation,main_base,owner,wrapper,course_data,header;
} FlowCoursePin;
typedef struct {
    flm_u64 magic;
    flm_u32 abi,size;
    FlowCoursePin pin;
    flm_u32 lock,phase,fault,reserved;
    FlowLaneMesh mesh;
    flm_u32 thickness[FLM_MODELS],radius[FLM_MODELS];
    unsigned char digest[32];
    flm_u64 computations,hits;
} FlowCoursePublication;

enum {FCP_COLD=0,FCP_LOADING=1,FCP_READY=2,FCP_OTHER=3};
enum {FCP_BEGIN=1,FCP_PUBLISH=2,FCP_RETIRE=3,FCP_DATA_RELOAD=4,
      FCP_WRAPPER_DELETE=5,FCP_FACTORY_RESET=6,FCP_DATA_DELETE=7,FCP_FACTORY_RELEASE=8};
extern FlowCoursePublication lab_flow_course_publication;

/* Called only at the qualified retail resource-load/cleanup boundaries.
 * PUBLISH follows native model/category and wrapper initialization. It may
 * hash at load time; admission never hashes. It writes only module storage.
 * Per-racer callers still require current job/role/frame lifetime. */
void lab_flow_course_event(FlowCoursePublication *,flm_u64 main_base,
                          flm_u32 event,flm_u64 object);
flm_u32 lab_flow_course_admit(FlowCoursePublication *,flm_u64 main_base,
                             flm_u64 wrapper,const FlowLaneMesh *,FlowCoursePin *);
flm_u32 lab_flow_course_same(FlowCoursePublication *,const FlowCoursePin *);
#endif
