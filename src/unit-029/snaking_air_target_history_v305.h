/* Private proposal: retain the exact target used by each owned prediction.
 * Native course gravity may be refreshed before the next producer sample.
 * The prior target is checked bit-for-bit independently of that new value.
 * Existing owner/tick, normal interpolation, contact and route guards remain.
 * No native field is written. The caller holds the existing SnakeState lock. */
EXPORT const u32 lab_snaking_air_target_history_config[4]={1,12,12,0};
EXPORT float lab_snaking_air_target_history[12][3];

static float *ath_slot(const GroundNativeAxis *r){
    u64 offset=(u64)r-(u64)lab_snaking_ground_axis_proposal_records;
    if(offset>=sizeof(lab_snaking_ground_axis_proposal_records)||
       offset%sizeof(GroundNativeAxis))return 0;
    return lab_snaking_air_target_history[offset/sizeof(GroundNativeAxis)];
}
static u32 ath_prior_target(const void *physics,const GroundNativeAxis *r){
    float *target=ath_slot(r);if(!target)return 0;
    for(u32 i=0;i<3;i++)
        if(w(physics,0x32c+4*i)!=nga_bits(target[i]))return 0;
    return 1;
}
static u32 ath_publish(GroundNativeAxis *r,const float *target){
    float *slot=ath_slot(r);if(!slot)return 0;
    for(u32 i=0;i<3;i++)slot[i]=target[i];
    return 1;
}
