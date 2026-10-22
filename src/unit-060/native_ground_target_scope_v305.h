#ifndef HARDERAI_NATIVE_GROUND_TARGET_SCOPE_V305_H
#define HARDERAI_NATIVE_GROUND_TARGET_SCOPE_V305_H
typedef unsigned long long ngs_u64;
typedef struct {ngs_u64 instance,generation,epoch,scene,system,race,group;} NativeGroundScope;
typedef struct {unsigned held;} NativeGroundLease;
/* Callback-owned native allocations remain the caller's responsibility.
 * Identity leases additionally protect the private descriptor/admission data. */
unsigned lab_native_ground_scope_enter(NativeGroundLease *);
void lab_native_ground_scope_leave(NativeGroundLease *);
unsigned lab_native_ground_scope_pin(const NativeGroundLease *,ngs_u64 main_base,
    ngs_u64 scene,ngs_u64 system,ngs_u64 race,NativeGroundScope *);
unsigned lab_native_ground_scope_same(const NativeGroundLease *,ngs_u64 main_base,const NativeGroundScope *);
#endif
