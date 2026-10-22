/* Private opt-in: compensate native drift turn during owned custom snaking.
 * Called only after the original heading dispatch's scope and geometry guards.
 * Ordinary CPU drifts retain their original scalar and .01 change ceiling.
 * No R, primary inward input, charge, lifetime or physics stores are added. */
EXPORT const struct {
    u32 abi,scope;
    float delta_max;
} lab_snaking_heading_compensation_config={1,2,.04f};

EXPORT float lab_snaking_heading_compensation_value(float original,float error,float correction,
                                                    float clear_slack,float speed){
    if(!finite(original)||!finite(error)||!finite(correction)||!finite(clear_slack)||
       !finite(speed)||speed<=0||speed>lab_bounded_drift_heading_config.max_speed||
       clear_slack<=0||absf(original)>(lab_bounded_drift_heading_config.selected_max+.08f)||
       absf(correction)>.08f)return original;
    float h=(float)lab_bounded_drift_heading_config.horizon;
    float delta=lab_bounded_drift_heading_config.clearance_fraction*clear_slack/
                (speed*h*(h+1.f)*.5f);
    if(delta>lab_snaking_heading_compensation_config.delta_max)
        delta=lab_snaking_heading_compensation_config.delta_max;
    float desired=clamp(error*lab_bounded_drift_heading_config.gain,
                        lab_bounded_drift_heading_config.selected_max);
    float candidate=clamp(desired-correction,(lab_bounded_drift_heading_config.selected_max+.08f));
    candidate=original+clamp(candidate-original,delta);
    if(!finite(candidate)||absf(desired-(candidate+correction))>=absf(desired-(original+correction)))return original;
    return candidate;
}
