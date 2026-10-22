/* Private native18AAA4 target prediction for the ordinary full-contact case.
 * Role+D8 retains a projection/normalization branch even at displacement>=3.
 * Reproduce that arithmetic; do not relax vector equality or write game data.
 */
EXPORT const u32 lab_snaking_native_collision_target_config[4]={1,0x18aaa4,0x8d026c,0xd8};
static u32 nct_expected(const void *physics,u64 role,float out[3]){
    for(u32 i=0;i<3;i++)out[i]=f(physics,0x17c+4*i);
    if(!b((void *)role,0xd8))return 1;
    u64 base=(u64)lab_module_start-LAB_MAIN_TO_MODULE;
    if((base&4095)||base>=(1ull<<39)-0x1400000)return 0;
    float gravity[3],motion[3],projected[3];
    for(u32 i=0;i<3;i++){
        gravity[i]=-f(physics,0x27c+4*i);motion[i]=f(physics,0x2fc+4*i);
        if(!finite(gravity[i])||!finite(motion[i])||!finite(out[i]))return 0;
    }
    /* Native helper writes only the supplied stack result. The full collision
     * routine uses the same gravity and motion operands at18ACE8. */
    ((void (*)(float *,const float *,const float *))(base+0x8d026c))(projected,gravity,motion);
    for(u32 i=0;i<3;i++){
        if(!finite(projected[i]))return 0;
        volatile float delta=out[i]-projected[i],product=1.f*delta;
        out[i]=projected[i]+product;
    }
    volatile float x2=out[0]*out[0],y2=out[1]*out[1],z2=out[2]*out[2],xy=x2+y2;
    float length=root(xy+z2);if(!finite(length)||length<=0)return 0;
    u64 epsilon=q((void *)base,0x12f5158);
    if(!ptr(epsilon))return 0;
    float threshold=f((void *)epsilon,0);
    if(!finite(threshold)||threshold<0)return 0;
    if(length<threshold){for(u32 i=0;i<3;i++)out[i]=f(physics,0x17c+4*i);return 1;}
    float inverse=1.f/length;
    for(u32 i=0;i<3;i++){out[i]*=inverse;if(!finite(out[i]))return 0;}
    return 1;
}
