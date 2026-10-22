/* Passive capture at the native maneuver termination, before state/counter
 * reset. No input, steering, charge, tier, boost or native object writes.
 * Recovery/finished flags are observations rather than capture refusals.
 */
#include "native_driving_trace_v305.h"

NDT_EXPORT const NativeDrivingTraceConfig lab_native_driving_trace_config = {
    0x3130305652444148ull, 1, NDT_RECORD_SIZE, NDT_RACERS, 0, 0x5d3ec, 0x5d3f0
};
NDT_EXPORT __attribute__((aligned(64)))
NativeDrivingTraceRecord lab_native_driving_trace_records[NDT_RACERS];

static ndt_u64 q(const void *p, ndt_u32 o) {
    return *(const volatile ndt_u64 *)((const ndt_u8 *)p + o);
}
static ndt_u32 w(const void *p, ndt_u32 o) {
    return *(const volatile ndt_u32 *)((const ndt_u8 *)p + o);
}
static ndt_u8 b(const void *p, ndt_u32 o) {
    return *((const volatile ndt_u8 *)p + o);
}
static ndt_u32 ptr(ndt_u64 p) {
    return p >= 0x10000 && !(p & 7) && p < (1ull << 39) - 0x2000;
}

NDT_EXPORT void lab_native_driving_trace_exit(void *producer,
    ndt_u32 w20, ndt_u32 w21, ndt_u32 w22, ndt_u32 w23,
    ndt_u32 w24, ndt_u32 w25, float final_dot) {
    if (!ptr((ndt_u64)producer)) return;
    ndt_u64 driver = q(producer, 0x70);
    if (!ptr(driver)) return;
    ndt_u64 adapter = q((void *)driver, 0x88), state = q((void *)driver, 0x50);
    if (!ptr(adapter) || !ptr(state) || q((void *)state, 0x88) != driver) return;
    ndt_u64 navigation = q((void *)state, 0xb0), ratio = q((void *)state, 0x90);
    if (!ptr(navigation) || !ptr(ratio) ||
        q((void *)navigation, 0x70) != (ndt_u64)producer ||
        q((void *)navigation, 0x58) != driver || q((void *)ratio, 0x58) != driver) return;
    ndt_u64 role = q((void *)adapter, 0x48), physics = q((void *)adapter, 0x50);
    ndt_u64 wrapper = q((void *)ratio, 0x48);
    if (!ptr(role) || !ptr(physics) || !ptr(wrapper) || q((void *)wrapper, 8) != driver ||
        q((void *)role, 0x28) != physics || q((void *)physics, 0xf8) != role ||
        b((void *)role, 0xd2) != 1 || b((void *)role, 0xd3) != 1 || b((void *)role, 0xe7)) return;
    ndt_u32 racer = w((void *)role, 0xa8);
    ndt_u64 drift = q((void *)physics, 0x110);
    if (racer >= NDT_RACERS || !ptr(drift) || q((void *)drift, 8) != role) return;

    NativeDrivingTraceRecord *s = &lab_native_driving_trace_records[racer];
    ndt_u32 expected = 0;
    if (!__atomic_compare_exchange_n(&s->lock, &expected, 1, 0,
                                     __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) return;
    if (s->fault) goto done;
    if ((s->sequence & 1) || s->sequence > ~0ull - 2 || s->events == ~0ull) {
        __atomic_store_n(&s->fault, 1, __ATOMIC_RELEASE);
        goto done;
    }
    /* Native callers own these objects for the duration of this callback.
     * Recheck the graph before publishing so a changed owner is not given the
     * identity of the original chain. The host must still pin race lifetime.
     */
    if (q(producer, 0x70) != driver || q((void *)driver, 0x88) != adapter ||
        q((void *)driver, 0x50) != state || q((void *)state, 0x88) != driver ||
        q((void *)state, 0xb0) != navigation || q((void *)state, 0x90) != ratio ||
        q((void *)navigation, 0x70) != (ndt_u64)producer ||
        q((void *)navigation, 0x58) != driver || q((void *)ratio, 0x58) != driver ||
        q((void *)ratio, 0x48) != wrapper || q((void *)wrapper, 8) != driver ||
        q((void *)adapter, 0x48) != role || q((void *)adapter, 0x50) != physics ||
        q((void *)role, 0x28) != physics || q((void *)physics, 0xf8) != role ||
        q((void *)physics, 0x110) != drift || q((void *)drift, 8) != role ||
        w((void *)role, 0xa8) != racer || b((void *)role, 0xd2) != 1 ||
        b((void *)role, 0xd3) != 1 || b((void *)role, 0xe7)) goto done;

    __atomic_add_fetch(&s->sequence, 1, __ATOMIC_ACQ_REL);
    s->events++;
    s->producer = (ndt_u64)producer; s->driver = driver; s->role = role;
    s->physics = physics; s->drift = drift; s->state = state;
    s->navigation = navigation; s->ratio = ratio; s->wrapper = wrapper;
    s->counter = w(producer, 0x44); s->racer = racer;
    s->producer_state = b(producer, 0x41); s->drift_state = w((void *)drift, 0x10);
    s->tier = w((void *)drift, 0x8c); s->role_flags = w((void *)role, 0x1cc);
    s->role_bytes = w((void *)role, 0xd0);
    s->active_remote = b((void *)role, 0xe1) | ((ndt_u32)b((void *)role, 0xe7) << 8);
    s->gates[0] = w20; s->gates[1] = w21; s->gates[2] = w22;
    s->gates[3] = w23; s->gates[4] = w24; s->gates[5] = w25;
    union {float f; ndt_u32 u;} dot = {.f = final_dot};
    s->final_dot_bits = dot.u;
    s->charge_bits = w((void *)drift, 0x70);
    for (ndt_u32 i = 0; i < 3; i++) s->target_bits[i] = w((void *)drift, 0x80 + 4*i);
    s->tick_valid = 0; s->tick = 0; s->reserved = 0;
    __atomic_add_fetch(&s->sequence, 1, __ATOMIC_RELEASE);
done:
    __atomic_store_n(&s->lock, 0, __ATOMIC_RELEASE);
}
