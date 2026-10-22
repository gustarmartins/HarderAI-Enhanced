#ifndef HARDERAI_DIRECTOR_PRESSURE_NATIVE_V305_H
#define HARDERAI_DIRECTOR_PRESSURE_NATIVE_V305_H
#include "director_pressure_pace_v305.h"
/* The caller must already own the current native race callback/allocation
 * lifetime. These actors must come from its qualified manager partition. */
typedef struct {
    pp_u64 role,physics,driver,wrapper,context,clock;
    pp_u32 racer,tick;
} PressureNativeActor;
typedef struct {
    PressureNativeActor actor;
    PressurePaceSample sample;
    pp_u64 drift,dash;
    pp_u32 role_flags,drift_state,dash_on,dash_frame,star_ticks,air,contact;
    pp_u32 surface_veto_a,surface_veto_b,active,remote;
    float residual;
} PressureNativeCapture;
_Static_assert(sizeof(PressureNativeActor)==56,"Pressure native actor ABI1");
_Static_assert(sizeof(PressureNativeCapture)==152,"Pressure native capture ABI1");
/* No native writes. Two exact reads reject observed ownership/state changes;
 * this is not an atomic scheduler snapshot or an independent lifetime lease. */
pp_u32 lab_pressure_native_collect(const PressureNativeActor *,PressureNativeCapture *);
#endif
