/* Held-Star projectile defense, after the stock item-use eligibility check.
 * Read-only borrowing of the same current-scene pools as the qualified Horn
 * policy. Native request publication, inventory and Star lifetime are intact.
 * Optional item-hazard extension borrows qualified Banana/Bomb pools. Course
 * objects and nearby racers do not independently trigger this policy.
 */
typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long long u64;
#define EXPORT __attribute__((visibility("default")))
extern u32 lab_star_item_hazard_decision(const void *,const void *,u64);

EXPORT const struct {
    u32 abi,held_selector,planner_state,blue_circle_start,blue_circle_end;
    u32 blue_down_end,max_pool_count;
    float shell_distance;
} lab_star_defense_config={1,8,3,54,85,4,64,90.f};

static u64 q(const void *p,u32 o){return *(const u64 *)((const u8 *)p+o);}
static u32 w(const void *p,u32 o){return *(const u32 *)((const u8 *)p+o);}
static u8 b(const void *p,u32 o){return *((const u8 *)p+o);}
static float f(const void *p,u32 o){return *(const float *)((const u8 *)p+o);}
static u32 ptr(u64 p){return p>=0x10000&&!(p&7)&&p<(1ull<<39)-0x2000;}
static u32 nearby(const void *item,const void *physics){
    float x=f(item,0x12c)-f(physics,0x2c);
    float y=f(item,0x130)-f(physics,0x30);
    float z=f(item,0x134)-f(physics,0x34);
    float radius=lab_star_defense_config.shell_distance;
    /* Includes height; NaN/overflow refuse. This is proximity, not trajectory. */
    return x*x+y*y+z*z<=radius*radius;
}

#include "star_own_green_v305.h"

static u32 close_shells(const void *table,const void *physics,u32 racer,u64 base){
    static const u32 object_vtables[2]={0x11ab3a8,0x11aad98};
    static const u32 pool_vtables[2]={0x11ab6e0,0x11ab288};
    for(u32 kind=0;kind<2;kind++){
        u64 pool=q(table,kind*8);
        if(!ptr(pool)||q((void *)pool,0)!=base+pool_vtables[kind]||
           w((void *)pool,0x38)!=kind)continue;
        u32 count=w((void *)pool,0x40);u64 nodes=q((void *)pool,0x48);
        if(count>lab_star_defense_config.max_pool_count||(count&&!ptr(nodes)))continue;
        for(u32 i=0;i<count;i++){
            u64 item=q((void *)nodes,0x20*i+0x10);
            if(!ptr(item)||q((void *)item,0)!=base+object_vtables[kind]||
               w((void *)item,0x58)!=kind||(int)w((void *)item,0x50)<0||
               w((void *)item,0x5c)==racer)continue;
            u32 state=b((void *)item,0x71);
            if((state==3||state==4)&&nearby((void *)item,physics))return 1;
        }
    }
    return 0;
}

EXPORT u32 lab_star_defense_decision(const void *planner,const void *scene,u64 base){
    if(!ptr((u64)planner)||!ptr((u64)scene)||!base||(base&4095)||
       base>=(1ull<<39)-0x1400000)return 0;
    u64 driver=q(planner,0x90),inventory=q(planner,0xa8);
    if(!ptr(driver)||!ptr(inventory))return 0;
    u64 state=q((void *)driver,0x50),proxy=q((void *)driver,0x88);
    if(!ptr(state)||!ptr(proxy)||q((void *)state,0x88)!=driver||
       q((void *)state,0xa8)!=(u64)planner||q((void *)proxy,0)!=base+0x11b44f8)return 0;
    u64 role=q((void *)proxy,0x48),physics=q((void *)proxy,0x50);
    if(!ptr(role)||!ptr(physics)||q((void *)role,0x28)!=physics||
       q((void *)physics,0xf8)!=role||b((void *)role,0xd2)!=1||
       b((void *)role,0xd3)!=1||b((void *)role,0xe1)!=1||b((void *)role,0xe7)||
       (w((void *)role,0x1cc)&0x01204018u)||(int)w((void *)role,0x1e4)>0)return 0;
    u32 racer=w((void *)role,0xa8);u64 slot=q((void *)inventory,0x58);
    if(racer>=12||!ptr(slot)||w((void *)slot,0x9c)!=lab_star_defense_config.held_selector||
       (int)w((void *)inventory,0xc0)>0||b(planner,0x41)!=lab_star_defense_config.planner_state)return 0;
    if(q(scene,0)!=base+0x125e158||!(b(scene,0x180)&4))return 0;
    u64 system=q(scene,0x1b0);
    if(!ptr(system)||q((void *)system,0)!=base+0x125a060)return 0;
    u64 race=q((void *)system,0x238),manager=q((void *)system,0x240);
    if(!ptr(race)||q((void *)race,0)!=base+0x11b4308||
       q((void *)race,0x50+8*racer)!=proxy||!ptr(manager)||
       q((void *)manager,0)!=base+0x11a7188||!b((void *)manager,0x170)||
       q((void *)manager,0x28)!=system+0x178||
       q((void *)race,0x28)!=system+0x178||q((void *)system,0x1a8)!=system)return 0;
    if(lab_star_item_hazard_decision(planner,scene,base))return 1;
    u32 types=w((void *)manager,0x38);u64 table=q((void *)manager,0x40);
    if(types<=5||types>64||!ptr(table))return 0;
    u64 pool=q((void *)table,5*8);
    if(!ptr(pool)||q((void *)pool,0)!=base+0x11aad08||w((void *)pool,0x38)!=5)return 0;
    u32 count=w((void *)pool,0x40);u64 nodes=q((void *)pool,0x48);
    if(count>lab_star_defense_config.max_pool_count||(count&&!ptr(nodes)))return 0;
    for(u32 i=0;i<count;i++){
        u64 item=q((void *)nodes,0x20*i+0x10);
        if(!ptr(item)||q((void *)item,0)!=base+0x11aa898||
           w((void *)item,0x58)!=5||(int)w((void *)item,0x50)<0||
           b((void *)item,0x71)!=3||w((void *)item,0x4c8)!=racer)continue;
        u32 phase=b((void *)item,0x399),tick=w((void *)item,0x39c);
        if((phase==5&&tick>=lab_star_defense_config.blue_circle_start&&
            tick<=lab_star_defense_config.blue_circle_end)||
           (phase==6&&tick<lab_star_defense_config.blue_down_end))return 1;
    }
    if(close_shells((void *)table,(void *)physics,racer,base))return 1;
    return lab_star_own_green_decision((void *)table,(void *)race,(void *)physics,racer,base);
}

EXPORT u32 lab_star_defense_current(const void *planner,u64 base){
    if(!ptr((u64)planner)||!base||(base&4095)||base>=(1ull<<39)-0x1400000)return 0;
    u64 inventory=q(planner,0xa8);if(!ptr(inventory))return 0;
    u64 slot=q((void *)inventory,0x58);
    if(!ptr(slot)||w((void *)slot,0x9c)!=lab_star_defense_config.held_selector)return 0;
    const void *scene=((const void *(*)(void))(base+0x7f41fc))();
    return lab_star_defense_decision(planner,scene,base);
}
