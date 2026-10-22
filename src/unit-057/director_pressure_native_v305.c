#include "director_pressure_native_v305.h"
typedef unsigned char pn_u8;
static pp_u64 q(pp_u64 p,pp_u32 o){return *(const volatile pp_u64 *)(p+o);}
static pp_u32 w(pp_u64 p,pp_u32 o){return *(const volatile pp_u32 *)(p+o);}
static pp_u32 b(pp_u64 p,pp_u32 o){return *(const volatile pn_u8 *)(p+o);}
static float f(pp_u64 p,pp_u32 o){union{float f;pp_u32 u;}v={.u=w(p,o)};return v.f;}
static pp_u32 finite(float x){union{float f;pp_u32 u;}v={.f=x};return (v.u&0x7fffffffu)<0x7f800000u;}
static pp_u32 ptr(pp_u64 p){return p>=0x10000&&!(p&7)&&p<(1ull<<39)-0x1000;}
static pp_u32 same(const void *a,const void *z,pp_u32 n){
    for(pp_u32 k=0;k<n;k++)if(((const pn_u8 *)a)[k]!=((const pn_u8 *)z)[k])return 0;return 1;
}
static pp_u32 collect(const PressureNativeActor *a,PressureNativeCapture *out){
    if(!a||a->racer>=12||!ptr(a->role)||!ptr(a->physics)||!ptr(a->driver)||!ptr(a->wrapper)||!ptr(a->context)||!ptr(a->clock))return 0;
    if(q(a->role,0x28)!=a->physics||q(a->physics,0xf8)!=a->role||q(a->role,8)!=a->context||
       q(a->context,8)!=a->role||q(a->context,0x18)!=a->driver||q(a->wrapper,8)!=a->driver||
       w(a->context,0x40)!=a->racer||w(a->role,0xa8)!=a->racer||w(a->clock,0x38)!=6||w(a->clock,0x44)!=a->tick)return 0;
    PressureNativeCapture r={0};r.actor=*a;r.sample.owner=a->driver;r.sample.racer=a->racer;
    r.drift=q(a->physics,0x110);r.dash=q(a->physics,0x118);
    if(!ptr(r.drift)||!ptr(r.dash)||q(r.drift,8)!=a->role||q(r.dash,8)!=a->role)return 0;
    r.sample.progress=f(a->wrapper,0x2c);r.sample.speed=f(a->physics,0x37c);
    r.sample.base_cap=f(a->physics,0x420);r.sample.normal_cap=f(a->physics,0x428);
    r.role_flags=w(a->role,0x1cc);r.drift_state=w(r.drift,0x10);
    r.dash_on=b(r.dash,0x10);r.dash_frame=w(r.dash,0x14);r.star_ticks=w(a->role,0x1e4);
    r.air=w(a->physics,0x26c);r.contact=b(a->physics,0x200)|(b(a->physics,0x201)<<8);
    r.surface_veto_a=w(a->physics,0x5d4);r.surface_veto_b=w(a->physics,0x5d8);r.residual=f(a->physics,0x3c8);
    r.active=b(a->role,0xe1);r.remote=b(a->role,0xe7);
    if(!finite(r.sample.progress)||!finite(r.sample.speed)||!finite(r.sample.base_cap)||!finite(r.sample.normal_cap)||
       r.sample.base_cap<=0||r.sample.normal_cap<=0||!finite(r.residual)||r.active>1||r.remote>1||
       r.dash_on>1||b(a->physics,0x200)>1||b(a->physics,0x201)>1)return 0;
    if((r.dash_on&&(int)r.dash_frame>0)||(int)r.star_ticks>0)r.sample.flags|=PP_BOOST;
    if(r.drift_state)r.sample.flags|=PP_DRIFT;
    if(r.air||r.contact!=0x101||(r.role_flags&0x18))r.sample.flags|=PP_AIR;
    /* Preserve existing normal-driving vetoes without inventing exact item
     * or terrain names for these fields. Direct off-road classification is
     * still separate work; slowdown also fails the clean cap-fraction gate. */
    if(!r.active||(r.role_flags&0x01204000u)||r.surface_veto_a||r.surface_veto_b||r.residual!=0.f)r.sample.flags|=PP_RECOVERY;
    if(r.remote)r.sample.flags|=PP_REMOTE;
    if(q(a->role,0x28)!=a->physics||q(a->physics,0xf8)!=a->role||q(a->physics,0x110)!=r.drift||
       q(a->physics,0x118)!=r.dash||q(a->role,8)!=a->context||q(a->context,8)!=a->role||
       q(a->context,0x18)!=a->driver||q(a->wrapper,8)!=a->driver||q(r.drift,8)!=a->role||q(r.dash,8)!=a->role||
       w(a->role,0xa8)!=a->racer||w(a->context,0x40)!=a->racer||w(a->clock,0x38)!=6||w(a->clock,0x44)!=a->tick)return 0;
    *out=r;return 1;
}
pp_u32 lab_pressure_native_collect(const PressureNativeActor *actor,PressureNativeCapture *out){
    if(!out)return 0;PressureNativeCapture a,z;
    if(!collect(actor,&a)||!collect(actor,&z)||!same(&a,&z,sizeof(a)))return 0;
    *out=a;return 1;
}
