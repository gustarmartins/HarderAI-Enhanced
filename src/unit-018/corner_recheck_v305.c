/* Reevaluate native drift opportunities during a navigation interval.
 * Only the phase value loaded into a register changes. The actual navigation
 * counter, native chance cache, RNG, geometry, drift state and input stay native.
 */
typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long long u64;
#define EXPORT __attribute__((visibility("default")))
EXPORT const u32 lab_corner_recheck_config[2]={1,15};
static u64 q(const void *p,u32 o){return *(const u64 *)((const u8 *)p+o);}
static u32 w(const void *p,u32 o){return *(const u32 *)((const u8 *)p+o);}
static u8 b(const void *p,u32 o){return *((const u8 *)p+o);}
static u32 ptr(u64 p){return p>=0x10000&&!(p&7)&&p<(1ull<<39)-0x2000;}

EXPORT u32 lab_corner_recheck_phase(const void *producer,u32 phase){
    if(phase<15||!ptr((u64)producer)||b(producer,0x41)!=0)return phase;
    u64 driver=q(producer,0x70),navigation=q(producer,0x80),route=q(producer,0x88);
    if(!ptr(driver)||!ptr(navigation)||!ptr(route)||q((void *)navigation,0x58)!=driver||
       q((void *)navigation,0x60)!=route||w((void *)navigation,0xa0)!=phase)return phase;
    u64 state=q((void *)driver,0x50),proxy=q((void *)driver,0x88);
    if(!ptr(state)||!ptr(proxy)||q((void *)state,0x88)!=driver)return phase;
    u64 controller=q((void *)state,0xb0);
    if(!ptr(controller)||q((void *)controller,0x58)!=driver||
       q((void *)controller,0x70)!=(u64)producer)return phase;
    u64 role=q((void *)proxy,0x48),physics=q((void *)proxy,0x50);
    if(!ptr(role)||!ptr(physics)||q((void *)role,0x28)!=physics||q((void *)physics,0xf8)!=role||
       w((void *)role,0xa8)>=12||b((void *)role,0xd2)!=1||b((void *)role,0xd3)!=1||
       b((void *)role,0xe1)!=1||b((void *)role,0xe7)||
       (w((void *)role,0x1cc)&0x01204018u))return phase;
    return phase%15;
}
