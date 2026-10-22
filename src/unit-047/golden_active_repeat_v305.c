/* At the native boost gate, an already-active Golden may request another use.
 * Native item selection, duration, inventory and request consumption stay native. */
typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long long u64;
static u64 q(const void *p,u32 o){return *(const u64 *)((const u8 *)p+o);}
static int w(const void *p,u32 o){return *(const int *)((const u8 *)p+o);}
static u8 b(const void *p,u32 o){return *((const u8 *)p+o);}
static int ptr(u64 p){return p>=0x10000 && !(p&7) && p<(1ull<<39)-0x1000;}
__attribute__((visibility("default")))
u32 lab_golden_repeat_allowed(const void *planner){
    if(!ptr((u64)planner))return 0;
    u64 driver=q(planner,0x90),inventory=q(planner,0xa8);
    if(!ptr(driver)||!ptr(inventory))return 0;
    u64 adapter=q((void *)driver,0x88),slot=q((void *)inventory,0x58);
    if(!ptr(adapter)||!ptr(slot))return 0;
    u64 role=q((void *)adapter,0x48);
    if(!ptr(role)||b((void *)role,0xd2)!=1||b((void *)role,0xd3)!=1||
       b((void *)role,0xe1)!=1||b((void *)role,0xe7)||
       ((u32)w((void *)role,0x1cc)&0x01204018u))return 0;
    return w((void *)slot,0x9c)==11 && w((void *)inventory,0xf8)>0 &&
           w((void *)inventory,0x108)>w((void *)inventory,0x104);
}
