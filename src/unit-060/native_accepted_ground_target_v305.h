#ifndef HARDERAI_NATIVE_ACCEPTED_GROUND_TARGET_V305_H
#define HARDERAI_NATIVE_ACCEPTED_GROUND_TARGET_V305_H
/* Exact native query operands, copied under the existing race/actor lease.
 * No new telemetry storage and no writes to native physics. */
typedef struct {
    unsigned actual[3],target[3],displacement_bits,tick;
} NativeAcceptedGroundTarget;
_Static_assert(sizeof(NativeAcceptedGroundTarget)==32,"Accepted native query operand ABI");
unsigned lab_native_ground_target_accepted(const void *,unsigned long long,
    unsigned,unsigned,NativeAcceptedGroundTarget *);
#endif
