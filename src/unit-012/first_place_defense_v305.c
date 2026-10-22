#include "item_rank_fallback_v305.h"
extern const unsigned char lab_module_start[];
/* First-place local CPU single-item defense. Native hold admission, item
 * collision/consumption, lifecycle and physical item ownership remain intact.
 * Numeric held selectors0/1/2 map to Banana/Green/Red object selectors2/0/1.
 * This does not implement Super Horn threat detection. */
typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long long u64;
static u64 q(const void *p,u32 o){return *(const u64 *)((const u8 *)p+o);}
static u32 w(const void *p,u32 o){return *(const u32 *)((const u8 *)p+o);}
static u8 b(const void *p,u32 o){return *((const u8 *)p+o);}
static u32 ptr(u64 p){return p>=0x10000 && !(p&7) && p<(1ull<<39)-0x2000;}

__attribute__((visibility("default")))
u32 lab_first_place_protection(const void *planner,u32 trailing_only){
    if(!ptr((u64)planner)||trailing_only>1)return 0;
    u64 driver=q(planner,0x90),inventory=q(planner,0xa8);
    if(!ptr(driver)||!ptr(inventory))return 0;
    u64 adapter=q((void *)driver,0x88),state=q((void *)driver,0x50);
    u64 order=q((void *)driver,0x70),wrapper=q((void *)driver,0x78);
    if(!ptr(adapter)||!ptr(state)||!ptr(order)||(wrapper&&!ptr(wrapper))||
       q((void *)state,0x88)!=driver||q((void *)state,0xa8)!=(u64)planner||
       (wrapper&&q((void *)wrapper,8)!=driver))return 0;
    u64 role=q((void *)adapter,0x48),physics=q((void *)adapter,0x50);
    if(!ptr(role)||!ptr(physics)||q((void *)role,0x28)!=physics||q((void *)physics,0xf8)!=role||
       b((void *)role,0xd2)!=1||b((void *)role,0xd3)!=1||b((void *)role,0xe1)!=1||
       b((void *)role,0xe7)||(w((void *)role,0x1cc)&0x01204018u)||w((void *)role,0xa8)>=12)return 0;
    /* Match the ordinary ordered-wrapper branch of native rank getter71580.
     * No fallback to racer index when native race ordering is unavailable. */
    if(lab_item_ordered_rank((void *)driver,(void *)adapter,(void *)role,(u64)lab_module_start-LAB_MAIN_TO_MODULE)!=0)return 0;
    if((int)w((void *)inventory,0xc0)>0 && w((void *)inventory,0xc4)==1)
        return w((void *)inventory,0xc8)<=2;
    if(trailing_only)return 0;
    u64 slot=q((void *)inventory,0x58);
    return ptr(slot)&&w((void *)slot,0x9c)<=2;
}
