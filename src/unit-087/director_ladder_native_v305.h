#ifndef HARDERAI_DIRECTOR_LADDER_NATIVE_V305_H
#define HARDERAI_DIRECTOR_LADDER_NATIVE_V305_H
#include "director_pressure_pace_v305.h"
/* Private policy: saved geometry is already normalized for native CPU count.
 * The native FCRA latch has no duration. ease_ticks is our explicit duration.
 * No pointers, I/O, native stores, slot exchange or RNG consumption. */
typedef struct {
    pp_u32 abi,ease_ticks,cycle_ticks,max_gap_ticks;
    float strength,half_depth_fraction;
    pp_u32 reserved[2];
} LadderConfig;
typedef struct {
    pp_u64 epoch,owner,tune_revision;
    pp_u32 tick,seed,slot,enabled;
    float native_scale,gap,native_rung,up,down;
    float increments[12];
} LadderInput;
typedef struct {
    pp_u64 epoch,owner,tune_revision;
    pp_u32 tick,start_tick,initialized,reserved;
    float from_scale,to_scale,scale;
    pp_u32 reserved2;
} LadderState;
typedef struct {
    pp_u32 status,cohort,cycle,reserved;
    float fraction,pulse,scale,depth,gap,target,error,half_strength,formation;
    float increments[12],targets[12];
} LadderOutput;
enum { DL_OFF=0,DL_READY=1,DL_RESET=2,DL_INVALID=3,DL_DUPLICATE=4 };
enum { DL_FRONT=0,DL_MIDDLE=1,DL_REAR=2 };
_Static_assert(sizeof(LadderConfig)==32,"Ladder config ABI1");
_Static_assert(sizeof(LadderInput)==112,"Ladder input ABI1");
_Static_assert(sizeof(LadderState)==56,"Ladder state ABI1");
_Static_assert(sizeof(LadderOutput)==148,"Ladder output ABI1");
pp_u32 lab_ladder_step(const LadderConfig *,const LadderInput *,LadderState *,LadderOutput *);
/* Retain the measured-pressure action and budget. Formation now comes from
 * this actor's moving target; catch fills only the remaining authored budget.
 * No second independent speed multiplier or hidden global ceiling is added. */
pp_u32 lab_ladder_pressure(const PressureBudget *,const PressurePeer *,const PressurePeer *,
    const LadderOutput *,PressureRequest *);
#endif
