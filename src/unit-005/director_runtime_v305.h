#ifndef HARDERAI_DIRECTOR_RUNTIME_V305_H
#define HARDERAI_DIRECTOR_RUNTIME_V305_H
#include "director_core_v305.h"
#define DR_EXPORT __attribute__((visibility("default")))
typedef struct {
    dc_u64 magic;
    dc_u32 abi,size,capacity,flags,site,continuation;
    DirectorCoreConfig policy;
} DirectorRuntimeConfig;
typedef struct {
    dc_u32 lock,fault;
    dc_u64 sequence,updates;
    dc_u64 controller,scene,system,race,group,manager,parameters;
    DirectorCoreState state;
    DirectorCoreInput input;
    DirectorCoreOutput output;
} DirectorRuntimeRecord;
_Static_assert(sizeof(DirectorRuntimeConfig)==72,"Director config ABI1");
_Static_assert(sizeof(DirectorRuntimeRecord)==672,"Director record ABI1");
DR_EXPORT dc_u32 lab_director_sum_callback(void *,dc_u32,dc_u32,dc_u32,dc_u32);
#endif
