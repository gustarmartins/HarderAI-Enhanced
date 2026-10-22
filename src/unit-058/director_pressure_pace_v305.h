#ifndef HARDERAI_DIRECTOR_PRESSURE_PACE_V305_H
#define HARDERAI_DIRECTOR_PRESSURE_PACE_V305_H
/* Private proposal. Inputs are qualified native racing samples supplied by
 * the caller. Progress is measured in native order units, time in race ticks.
 * No native pointers are followed and no game object is written here. */
typedef unsigned int pp_u32;
typedef unsigned long long pp_u64;
#define PP_CAPACITY 12u
enum { PP_BOOST=1,PP_HIT=2,PP_OFFROAD=4,PP_AIR=8,PP_DRIFT=16,PP_RECOVERY=32,PP_REMOTE=64 };
enum { PP_INVALID=0,PP_WARMUP=1,PP_READY=2,PP_UNCLEAN=3,PP_RESET=4,PP_DUPLICATE=5 };
typedef struct {
    pp_u64 owner;
    float progress,speed,base_cap,normal_cap;
    pp_u32 racer,flags;
} PressurePaceSample;
typedef struct {
    pp_u32 abi,max_gap_ticks,warmup_ticks,max_age_ticks;
    float response_ticks,min_speed,min_cap_fraction,max_cap_fraction;
    float min_progress_fraction,max_progress_fraction,max_ratio_step;
    pp_u32 reserved[2];
} PressurePaceConfig;
typedef struct {
    pp_u64 epoch,owner;
    pp_u32 last_tick,last_clean_tick,clean_ticks,previous_clean;
    float progress,speed,ratio,baseline_rate,clean_ratio;
    pp_u32 ready,reserved;
} PressurePaceState;
typedef struct {
    pp_u32 status,clean,ready,clean_ticks;
    float progress_rate,baseline_rate,ratio,age_ticks;
} PressurePaceOutput;
typedef struct {
    pp_u32 abi,attackers,rotation_ticks,eligible_mask;
    float near_seconds,far_seconds,recovery_seconds,ticks_per_second;
    float lead_allowance_seconds,field_recovery_seconds;
} PressureSelectionConfig;
typedef struct {
    PressurePaceSample sample;
    PressurePaceOutput pace;
} PressurePeer;
typedef struct {
    pp_u32 valid,eligible_mask,selected_mask,cycle;
    float reference_rate;
    pp_u32 reserved[3];
} PressureSelection;
typedef struct {
    float native_base,native_catch,native_formation;
    float up,down,cu_min,cu_max,cu_ratio,cd_min,cd_max,cd_ratio;
    float catch_authority,formation_authority;
} PressureBudget;
typedef struct {
    pp_u32 valid,selected,action,reserved;
    float gap_seconds,closing_rate,requested_ratio,wanted_ratio;
    float catch_term,formation_term,positive_allowance,negative_allowance;
} PressureRequest;
enum { PP_NATIVE_PACE=0,PP_ATTACK_PACE=1,PP_CONTAIN_LEAD=2,PP_FIELD_RECOVERY=3 };
typedef struct {pp_u32 abi,max_gap_ticks;float response_ticks;pp_u32 reserved;} PressureBlendConfig;
typedef struct {
    pp_u64 epoch,owner,tune_revision;
    pp_u32 tick,flags,enabled,reserved;
    float native_base,native_catch,native_formation;
    PressureRequest request;
} PressureBlendInput;
typedef struct {
    pp_u64 epoch,owner,tune_revision;
    pp_u32 tick,initialized;
    float catch_term,formation_term,selected_weight;
    pp_u32 reserved;
} PressureBlendState;
typedef struct {
    pp_u32 status,dt;
    float ratio,catch_term,formation_term,selected_weight;
} PressureBlendOutput;
PressurePaceConfig lab_pressure_pace_default(void);
pp_u32 lab_pressure_pace_sample(const PressurePaceConfig *,pp_u64 epoch,pp_u32 tick,
    const PressurePaceSample *,PressurePaceState *,PressurePaceOutput *);
/* Reference is an array index; masks always use stable native racer IDs.
 * Selection never consults native formation slots or fixed leader positions. */
pp_u32 lab_pressure_select(const PressureSelectionConfig *,const PressurePeer *,pp_u32 count,
    pp_u32 reference,pp_u32 tick,pp_u32 seed,PressureSelection *);
/* A single desired component sum. Existing catch and formation count toward
 * the request; no additional independent attack multiplier is stacked. */
pp_u32 lab_pressure_request(const PressureSelectionConfig *,const PressureBudget *,
    const PressurePeer *self,const PressurePeer *reference,pp_u32 selected,PressureRequest *);
pp_u32 lab_pressure_blend(const PressureBlendConfig *,const PressureBlendInput *,
    PressureBlendState *,PressureBlendOutput *);
#endif
