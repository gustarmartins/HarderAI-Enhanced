#ifndef HARDERAI_NATIVE_GROUND_TARGET_OBSERVER_V305_H
#define HARDERAI_NATIVE_GROUND_TARGET_OBSERVER_V305_H
typedef unsigned char ngt_u8;
typedef unsigned int ngt_u32;
typedef unsigned long long ngt_u64;
#define NGT_EXPORT __attribute__((visibility("default")))
#define NGT_CAPACITY 12u
#include "native_ground_target_scope_v305.h"
#define NGT_SIZE 328u
#define NGT_ABI 2u
typedef struct {
    ngt_u64 role,physics,context,clock,race_system,race;
    ngt_u32 tick,racer,accepted,role_flags,role_bytes,active_remote;
    ngt_u32 air,contact,mode,selector,normal_flags,displacement_bits;
    ngt_u32 old_road[3],new_road[3],old_hit[3],new_hit[3],target[3];
    ngt_u32 actual[3],gravity[3],motion[3],pending[3],chassis[3];
    ngt_u64 reserved;
} NativeGroundTargetPayload;
typedef struct {
    ngt_u32 lock,fault;
    ngt_u64 sequence,events,dropped,accepted_count,retained_count;
    NativeGroundTargetPayload value;
    NativeGroundScope scope;
} NativeGroundTargetRecord;
_Static_assert(sizeof(NativeGroundTargetPayload)==224,"Ground target payload ABI1");
_Static_assert(sizeof(NativeGroundTargetRecord)==NGT_SIZE,"Ground target record ABI1");
_Static_assert(__builtin_offsetof(NativeGroundTargetRecord,value)==48,"Ground target payload offset");
NGT_EXPORT void lab_native_ground_target_publish(void *,ngt_u32,const ngt_u32 *,const ngt_u32 *);
NGT_EXPORT ngt_u32 lab_native_ground_target_retained(const void *,ngt_u64,ngt_u32,ngt_u32);
#endif
