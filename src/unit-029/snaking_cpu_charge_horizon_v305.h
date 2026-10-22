/* Planner-only qualification of the already installed native CPU comparison.
 * Keep retail .71 for humans/remotes, and never assign native charge/state. */
extern const float lab_cpu_inward_charge_threshold;
EXPORT const struct {u32 abi,cpu_offset,remote_offset,reserved;float native_minimum;}
lab_snaking_cpu_charge_horizon_config={1,0xd2,0xe7,0,.71f};
static u32 sch_inward(const void *role,float inward){
    if(!finite(inward))return 0;
    float minimum=.71f;
    if(b(role,0xd2)&&!b(role,0xe7)){
        minimum=lab_cpu_inward_charge_threshold;
        /* A changed or invalid dependency is unqualified, even if its new
         * threshold might accept this input. The frozen native hook uses .70. */
        if(!finite(minimum)||minimum!=.70f)return 0;
    }
    return inward>=minimum;
}
