/* Retail71580 ordered branches. The unordered native racer-index return is
 * intentionally not a race placement. Both rank sources must remain borrowed
 * within a qualified current CPU item-planner call. */
typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long long u64;
#define EXPORT __attribute__((visibility("default")))
static u64 q(const void *p,u32 o){return *(const u64 *)((const u8 *)p+o);}
static u32 w(const void *p,u32 o){return *(const u32 *)((const u8 *)p+o);}
static u8 b(const void *p,u32 o){return *((const u8 *)p+o);}
static u32 ptr(u64 p){return p>=0x10000&&!(p&7)&&p<(1ull<<39)-0x2000;}
EXPORT const u32 lab_item_rank_fallback_config[12]={1,0x70,0x78,0x65,0x40,0x48,0x78,0x2a,0x71580,0x711b4,0x140924,12};

EXPORT int lab_item_ordered_rank(const void *driver,const void *proxy,const void *role,u64 base){
    if(!ptr((u64)driver)||!ptr((u64)proxy)||!ptr((u64)role)||(base&4095)||base>(1ull<<39)-0x1400000||
       q(driver,0x88)!=(u64)proxy||q(proxy,0x48)!=(u64)role)return -1;
    u32 racer=w(role,0xa8);if(racer>=12)return -1;
    u64 order=q(driver,0x70),wrapper=q(driver,0x78);
    if(!ptr(order)||!b((void *)order,0x65))return -1;
    if(wrapper){
        if(!ptr(wrapper)||q((void *)wrapper,8)!=(u64)driver)return -1;
        u32 rank=w((void *)wrapper,0x40);return rank<12?(int)rank:-1;
    }
    /* Native virtual driver+E8 calls711B4; its proxy virtual0 calls140924,
     * which returns this exact role+A8 racer index. No native call is made. */
    u64 table=q(driver,0),proxy_table=q(proxy,0);
    if(!ptr(table)||!ptr(proxy_table)||q((void *)table,0xe8)!=base+0x711b4||
       q((void *)proxy_table,0)!=base+0x140924)return -1;
    u32 count=w((void *)order,0x40);u64 rows=q((void *)order,0x48);
    if(count>12||!ptr(rows))return -1;
    /* Exact unsigned CSEL-HI branch: an out-of-count index selects row0. */
    u32 index=count>racer?racer:0;
    u32 rank=b((void *)rows,0x78*index+0x2a);
    return rank<12?(int)rank:-1;
}
