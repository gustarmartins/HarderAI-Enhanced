/* Private landing-order probe. No native field writes or angular relaxation.
 * A newly published contact/target may precede the next 182990 invocation.
 * Restrict this alternative to the saturated ordinary ground blend (.1).
 * Full caller identity, unit, environment and corridor gates still apply.
 */
EXPORT const u32 lab_snaking_landing_prediction_config[4]={1,3,0x182990,0};
static u32 nlp_landed_prediction(const void *physics,const GroundNativeAxis *r,
                                u32 contact,float out[3],u32 retained){
    if(contact!=3||(r->contact!=0&&r->contact!=1)||!r->air||
       r->ds!=2||r->landing||!finite(r->displacement)||r->displacement<3.f)
        return 0;
    float displacement=f(physics,0x568);
    if(!finite(displacement)||displacement<3.f||(!retained&&!ngs_native_ground_target(physics)))return 0;
    /* Independently retain the old exact prediction contract. Corrupting its
     * stored expected bits must not become an alternate landing admission. */
    float old_target[3],old_expected[3];
    if(r->expected_world){
        if(!nga_world(physics,r->mode,r->selector,r->flags,old_target))return 0;
    }else for(u32 i=0;i<3;i++)old_target[i]=r->target[i];
    if(!finite(r->coefficient)||!nga_interpolate(r->actual,old_target,r->coefficient,old_expected))return 0;
    for(u32 i=0;i<3;i++)if(nga_bits(old_expected[i])!=nga_bits(r->expected[i]))return 0;
    float target[3];for(u32 i=0;i<3;i++)target[i]=f(physics,0x32c+4*i);
    if(!nga_interpolate(r->actual,target,.1f,out))return 0;
    for(u32 i=0;i<3;i++)if(nga_bits(out[i])!=w(physics,0x320+4*i))return 0;
    return 1;
}
