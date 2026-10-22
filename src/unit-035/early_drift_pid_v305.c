/* Private early native drift-heading feedback. This predicate only chooses
 * an existing retail block. It writes no game or module storage, calls no
 * service, and does not manufacture charge, input, heading or PID gains.
 */
typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long long u64;
#define EXPORT __attribute__((visibility("default")))
EXPORT const u32 lab_early_drift_pid_config[6]={1,1,30,4,5,0x01204018u};
static u64 q(const void *p,u32 o){return *(const u64 *)((const u8 *)p+o);}
static u32 w(const void *p,u32 o){return *(const u32 *)((const u8 *)p+o);}
static u8 b(const void *p,u32 o){return *((const u8 *)p+o);}
static float f(const void *p,u32 o){return *(const float *)((const u8 *)p+o);}
static u32 ptr(u64 p){return p>=0x10000&&!(p&7)&&p<(1ull<<39)-0x2000;}
static u32 finite(float v){union {float f;u32 u;}x={.f=v};return (x.u&0x7fffffffu)<0x7f800000u;}
static u32 unit(const void *p,u32 o){
    float norm=0;
    for(u32 i=0;i<3;i++){float v=f(p,o+4*i);if(!finite(v))return 0;norm+=v*v;}
    return norm>=.81f&&norm<=1.21f;
}

EXPORT u32 lab_early_drift_pid_admit(const void *producer){
    if(!ptr((u64)producer)||b(producer,0x41)!=1)return 0;
    u32 counter=w(producer,0x44);
    /* Counter zero retains native priming. The ordinary early tail writes
     * previous blended motion at +120 before feedback may consume it. */
    if(counter<lab_early_drift_pid_config[1]||counter>=lab_early_drift_pid_config[2])return 0;
    u64 driver=q(producer,0x70),source=q(producer,0x80),route=q(producer,0x88);
    u64 packet=q(producer,0x98),target=q(producer,0xa0);
    if(!ptr(driver)||!ptr(source)||!ptr(route)||!ptr(packet)||!ptr(target)||
       q((void *)source,0x58)!=driver||q((void *)source,0x60)!=route)return 0;
    u64 state=q((void *)driver,0x50),adapter=q((void *)driver,0x88);
    if(!ptr(state)||!ptr(adapter)||q((void *)state,0x88)!=driver||
       q((void *)state,0xc0)!=packet)return 0;
    u64 nav=q((void *)state,0xb0);
    if(!ptr(nav)||q((void *)nav,0x28)!=state||q((void *)nav,0x58)!=driver||
       q((void *)nav,0x60)!=source||q((void *)nav,0x70)!=(u64)producer||
       q((void *)nav,0x78)!=packet||q((void *)nav,0x80)!=target)return 0;
    u64 other=q((void *)nav,0x68);
    /* Do not amplify avoidance/pursuit targets or a borrowed manual target. */
    if(!ptr(other)||q((void *)other,0x70)!=driver||b((void *)other,0x41)!=0||b((void *)target,0x28))return 0;
    for(u32 i=0;i<3;i++){
        float selected=f((void *)route,0x9c+4*i),original=f((void *)target,4*i);
        float effective=f((void *)target,0xc+4*i);
        if(!finite(selected)||!finite(original)||!finite(effective)||
           original!=selected||effective!=selected)return 0;
    }
    u64 role=q((void *)adapter,0x48),physics=q((void *)adapter,0x50);
    if(!ptr(role)||!ptr(physics)||q((void *)role,0x28)!=physics||q((void *)physics,0xf8)!=role||
       b((void *)role,0xd2)!=1||b((void *)role,0xd3)!=1||
       b((void *)role,0xe1)!=1||b((void *)role,0xe7)||w((void *)role,0xa8)>=12||
       (w((void *)role,0x1cc)&lab_early_drift_pid_config[5]))return 0;
    u64 ai=q((void *)driver,0x90),drift=q((void *)physics,0x110);
    if(!ptr(ai)||!ptr(drift)||q((void *)physics,0x100)!=ai||
       q((void *)ai,0x10)!=role||q((void *)drift,8)!=role)return 0;
    u32 ds=w((void *)drift,0x10),flags=w((void *)packet,4);
    if((ds!=4&&ds!=5)||(flags&0xbu)!=9u)return 0;
    if(!unit((void *)target,0x18)||!unit(producer,0x120)||
       !finite(f((void *)target,0x24))||!finite(f(producer,0x118))||
       !finite(f(producer,0x11c))||!finite(f(producer,0x12c))||f(producer,0x12c)<=0)return 0;
    return 1;
}
