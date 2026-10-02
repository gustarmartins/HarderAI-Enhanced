/* Optional cap floor after native and explicit/adaptive pace calculation.
 * Native physics owns these objects during this callback. No route, input,
 * drift admission/release, charge accumulator or timer is changed. */
typedef unsigned int u32;
typedef unsigned long long u64;
typedef unsigned char u8;
#define EXPORT __attribute__((visibility("default")))

#ifndef LAB_DRIFT_PACE_FLOOR_BITS
#error "Supply validated drift floor bits"
#endif
#ifndef LAB_DRIFT_PACE_STATES
#error "Supply validated drift state mask"
#endif

EXPORT const struct { u32 floor_bits; u32 states; } lab_drift_pace_config={
    LAB_DRIFT_PACE_FLOOR_BITS,LAB_DRIFT_PACE_STATES};

static u64 q(const void *p,u32 off){return *(const u64 *)((const u8 *)p+off);}
static u32 w(const void *p,u32 off){return *(const u32 *)((const u8 *)p+off);}
static u8 b(const void *p,u32 off){return *((const u8 *)p+off);}
static float f(const void *p,u32 off){return *(const float *)((const u8 *)p+off);}
static u32 pointer(u64 p){return p>=0x10000 && !(p&7) && p<(1ull<<39)-0x800;}
static u32 finite(float v){union{float f;u32 u;}x={.f=v};return (x.u&0x7fffffffu)<0x7f800000u;}

EXPORT u32 lab_drift_pace_apply(void *physics,void *role){
    if(!pointer((u64)physics) || !pointer((u64)role) ||
       q(physics,0xf8)!=(u64)role || q(role,0x28)!=(u64)physics ||
       w(role,0xa8)>=12 || !b(role,0xd2) || !b(role,0xe1) || b(role,0xe7) ||
       (w(role,0x1cc)&0x01200018u))return 0;
    u64 drift=q(physics,0x110);
    if(!pointer(drift) || q((void *)drift,8)!=(u64)role)return 0;
    u32 state=w((void *)drift,0x10);
    if(state>=32 || !(lab_drift_pace_config.states&(1u<<state)))return 0;
    union{u32 u;float f;}setting={.u=lab_drift_pace_config.floor_bits};
    float pre=f(physics,0x424),cap=f(physics,0x428),independent=f(physics,0x42c);
    if(!(setting.f>0 && setting.f<=1) || !finite(pre) || !finite(cap) || !finite(independent) ||
       pre<=0 || cap<0 || independent<0)return 0;
    float minimum=pre*setting.f;
    if(!finite(minimum))return 0;
    u32 changed=0;
    if(cap<minimum){*(float *)((u8 *)physics+0x428)=minimum;changed=1;}
    if(independent<minimum){*(float *)((u8 *)physics+0x42c)=minimum;changed=1;}
    return changed;
}
