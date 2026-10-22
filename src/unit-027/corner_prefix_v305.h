/* A rejected future plane may bound a qualified corner prefix. The caller
 * retains current occupation, both path views, forecast coverage, endpoint
 * reserves and charge feasibility. No time/distance is credited past it. */
EXPORT const u32 lab_corner_prefix_config[4]={1,7,2,0};
static u32 cp_fail(const ArcOwner *c,ArcOperandPayload *d,u32 detail,u32 *reason){
    if(reason)*reason=detail;
    return co_fail(c,d,detail);
}
