#ifndef HARDERAI_NATIVE_TURBO_COMMIT_V305_H
#define HARDERAI_NATIVE_TURBO_COMMIT_V305_H
typedef unsigned char ntc_u8;
typedef unsigned int ntc_u32;
typedef int ntc_i32;
typedef unsigned long long ntc_u64;
#define NTC_EXPORT __attribute__((visibility("default")))
#define NTC_CAPACITY 12u
#define NTC_SIZE 184u

/* A committed physical-release call, not a speed-increase assertion. Counters
 * are module-lifetime observed lower bounds; readers bind current race/owners.
 * Contended publication skips rather than delaying the native physics job. */
typedef struct {
    ntc_u32 lock,fault;
    ntc_u64 sequence,events,dropped;
    ntc_u64 role,physics,dash,drift,clock,race_system,caller;
    ntc_u32 tick,racer,event,tier,requested_kind,dominant_kind;
    ntc_i32 duration,total_ticks,kind_ticks_a,kind_ticks_b;
    ntc_u32 role_flags,role_bytes,active_remote;
    ntc_i32 star_ticks;
    ntc_u32 charge_bits,drift_state;
    ntc_u64 by_tier[3],reserved;
} NativeTurboCommit;
typedef struct {ntc_u64 magic;ntc_u32 abi,size,capacity,flags,site,continuation;} NativeTurboCommitConfig;
_Static_assert(sizeof(NativeTurboCommit)==NTC_SIZE,"Turbo commit ABI1");
_Static_assert(__builtin_offsetof(NativeTurboCommit,role)==32,"Turbo commit owner offset");
_Static_assert(__builtin_offsetof(NativeTurboCommit,tick)==88,"Turbo commit payload offset");
_Static_assert(__builtin_offsetof(NativeTurboCommit,by_tier)==152,"Turbo commit count offset");
_Static_assert(sizeof(NativeTurboCommitConfig)==32,"Turbo commit config ABI1");

NTC_EXPORT void lab_native_turbo_commit_publish(void *dash,ntc_u32 event,ntc_i32 duration,ntc_u64 caller);
#endif
