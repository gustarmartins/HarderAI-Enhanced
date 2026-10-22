#ifndef HARDERAI_NATIVE_DRIVING_TRACE_V305_H
#define HARDERAI_NATIVE_DRIVING_TRACE_V305_H

typedef unsigned char ndt_u8;
typedef unsigned int ndt_u32;
typedef unsigned long long ndt_u64;
#define NDT_EXPORT __attribute__((visibility("default")))
#define NDT_RACERS 12u
#define NDT_RECORD_SIZE 192u

/* One latest native release per racer. Zero-filled private BSS; no game object
 * or game timing field is used as mutable storage. Sequence is even when
 * published and odd during a write. Readers also require lock == fault == 0.
 * events counts accepted captures, not all releases: contended writers skip.
 */
typedef struct {
    ndt_u32 lock, fault;
    ndt_u64 sequence, events;
    ndt_u64 producer, driver, role, physics, drift;
    ndt_u64 state, navigation, ratio, wrapper;
    ndt_u32 counter, racer, producer_state, drift_state, tier, role_flags;
    ndt_u32 role_bytes, active_remote;
    ndt_u32 gates[6];                 /* Raw W20..W25 at native 0x5D3EC. */
    ndt_u32 final_dot_bits, charge_bits, target_bits[3];
    ndt_u32 tick_valid;               /* ABI 1: zero; no unproven clock read. */
    ndt_u64 tick, reserved;
} NativeDrivingTraceRecord;

typedef struct {
    ndt_u64 magic;
    ndt_u32 abi, record_size, capacity, flags, site, continuation;
} NativeDrivingTraceConfig;

_Static_assert(sizeof(NativeDrivingTraceRecord) == NDT_RECORD_SIZE, "Trace record ABI");
_Static_assert(__builtin_offsetof(NativeDrivingTraceRecord, sequence) == 8, "Trace sequence ABI");
_Static_assert(__builtin_offsetof(NativeDrivingTraceRecord, gates) == 128, "Trace predicates ABI");
_Static_assert(__builtin_offsetof(NativeDrivingTraceRecord, tick) == 176, "Trace tick ABI");
_Static_assert(sizeof(NativeDrivingTraceConfig) == 32, "Trace config ABI");

NDT_EXPORT extern const NativeDrivingTraceConfig lab_native_driving_trace_config;
NDT_EXPORT extern NativeDrivingTraceRecord lab_native_driving_trace_records[NDT_RACERS];
NDT_EXPORT void lab_native_driving_trace_exit(void *producer,
    ndt_u32 w20, ndt_u32 w21, ndt_u32 w22, ndt_u32 w23,
    ndt_u32 w24, ndt_u32 w25, float final_dot);

#endif
