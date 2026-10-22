/* Scope only the exact native-normal predictor's status-word dependency.
 * Bits4/21 remain vetoed earlier; bit12 remains a history discontinuity.
 * Physical/native-state, pointer/owner, actual prediction, route and contact
 * checks still run independently. No native status word is written. */
EXPORT const u32 lab_snaking_normal_flag_scope_config[4]={1,0x00201010u,39,0};

static u32 nfs_changed(u32 before,u32 after,NgdFailure *failure){
    if(!((before^after)&lab_snaking_normal_flag_scope_config[1]))return 0;
    (void)ngd_equal(before,after,failure,39,0xffffffffu);
    return 1;
}
