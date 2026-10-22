#include "item_rank_fallback_v305.h"
/* Private prototype: first-place Horn retention and Blue wind-up response.
 * Read-only decision over objects borrowed DURING a native item-planner call.
 * Caller must preserve native item-use eligibility and scene lifetime.
 * No cache, singleton, cross-frame item pointer or inventory write.
 * Includes nearby Green/Red shells and vulnerable racers. Not yet installed.
 */
typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long long u64;
static u64 q(const void *p,u32 o){return *(const u64 *)((const u8 *)p+o);}
static u32 w(const void *p,u32 o){return *(const u32 *)((const u8 *)p+o);}
static u8 b(const void *p,u32 o){return *((const u8 *)p+o);}
static float f(const void *p,u32 o){return *(const float *)((const u8 *)p+o);}
static u32 ptr(u64 p){return p>=0x10000 && !(p&7) && p<(1ull<<39)-0x2000;}
static u32 nearby(const void *a,u32 ao,const void *c,u32 co,float radius){
    float x=f(a,ao)-f(c,co),y=f(a,ao+4)-f(c,co+4),z=f(a,ao+8)-f(c,co+8);
    /* NaN/overflow fail the comparison; use all three axes on stacked tracks. */
    return x*x+y*y+z*z<=radius*radius;
}
static u32 immune(const void *role){
    /* Same predicate as native Horn collision's proxy140FBC ->1739A4. */
    return (int)w(role,0x1e4)>0 || (w(role,0x1cc)&0x40600001u) ||
           (int)w(role,0x248)>0 || w(role,0x1ec)==600;
}

static u32 close_shells(const void *table,const void *physics,u32 racer,u64 base){
    static const u32 object_vtables[2]={0x11ab3a8,0x11aad98};
    static const u32 pool_vtables[2]={0x11ab6e0,0x11ab288};
    for(u32 kind=0;kind<2;kind++){
        u64 pool=q(table,kind*8);
        if(!ptr(pool)||q((void *)pool,0)!=base+pool_vtables[kind]||
           w((void *)pool,0x38)!=kind)continue;
        u32 count=w((void *)pool,0x40);u64 nodes=q((void *)pool,0x48);
        if(count>64||(count&&!ptr(nodes)))continue;
        for(u32 i=0;i<count;i++){
            u64 item=q((void *)nodes,0x20*i+0x10);
            if(!ptr(item)||q((void *)item,0)!=base+object_vtables[kind]||
               w((void *)item,0x58)!=kind||(int)w((void *)item,0x50)<0||
               w((void *)item,0x5c)==racer)continue;
            u32 state=b((void *)item,0x71);
            if((state==3||state==4)&&nearby((void *)item,0x12c,physics,0x2c,90.f))return 1;
        }
    }
    return 0;
}

static u32 close_racers(const void *race,const void *physics,u32 racer,u64 base){
    for(u32 other=0;other<12;other++){
        if(other==racer)continue;
        u64 proxy=q(race,0x50+8*other);
        if(!ptr(proxy)||q((void *)proxy,0)!=base+0x11b44f8)continue;
        u64 role=q((void *)proxy,0x48),body=q((void *)proxy,0x50);
        if(!ptr(role)||!ptr(body)||q((void *)role,0x28)!=body||
           q((void *)body,0xf8)!=role||w((void *)role,0xa8)!=other||immune((void *)role))continue;
        /* 60 is an authored activation distance within native maximum radius90.
         * Native collision eligibility/teams and actual consumption still run. */
        if(nearby((void *)body,0x2c,physics,0x2c,60.f))return 1;
    }
    return 0;
}

enum { HORN_NATIVE=0, HORN_KEEP=1, HORN_USE=2 };

__attribute__((visibility("default")))
u32 lab_horn_blue_decision(const void *planner,const void *scene,u64 base){
    if(!ptr((u64)planner)||!ptr((u64)scene)||!base||(base&4095)||
       base>=(1ull<<39)-0x1400000)return HORN_NATIVE;
    u64 driver=q(planner,0x90),inventory=q(planner,0xa8);
    if(!ptr(driver)||!ptr(inventory))return HORN_NATIVE;
    u64 state=q((void *)driver,0x50),proxy=q((void *)driver,0x88);
    u64 order=q((void *)driver,0x70),wrapper=q((void *)driver,0x78);
    if(!ptr(state)||!ptr(proxy)||!ptr(order)||(wrapper&&!ptr(wrapper))||
       q((void *)state,0x88)!=driver||q((void *)state,0xa8)!=(u64)planner||
       (wrapper&&q((void *)wrapper,8)!=driver)||q((void *)proxy,0)!=base+0x11b44f8)
        return HORN_NATIVE;
    u64 role=q((void *)proxy,0x48),physics=q((void *)proxy,0x50);
    if(!ptr(role)||!ptr(physics)||q((void *)role,0x28)!=physics||
       q((void *)physics,0xf8)!=role||b((void *)role,0xd2)!=1||
       b((void *)role,0xd3)!=1||b((void *)role,0xe1)!=1||b((void *)role,0xe7)||
       (w((void *)role,0x1cc)&0x01204018u))return HORN_NATIVE;
    u32 racer=w((void *)role,0xa8);
    u64 slot=q((void *)inventory,0x58);
    if(racer>=12||!ptr(slot)||w((void *)slot,0x9c)!=16||
       (int)w((void *)inventory,0xc0)>0||b(planner,0x41)!=2||w(planner,0x44)<6)
        return HORN_NATIVE;
    /* Bind the borrowed item owner to the CURRENT scene's racer proxy. */
    if(q(scene,0)!=base+0x125e158||!(b(scene,0x180)&4))return HORN_NATIVE;
    u64 system=q(scene,0x1b0);
    if(!ptr(system)||q((void *)system,0)!=base+0x125a060)return HORN_NATIVE;
    u64 race=q((void *)system,0x238),manager=q((void *)system,0x240);
    if(!ptr(race)||q((void *)race,0)!=base+0x11b4308||
       q((void *)race,0x50+8*racer)!=proxy||!ptr(manager)||
       q((void *)manager,0)!=base+0x11a7188||!b((void *)manager,0x170)||
       q((void *)manager,0x28)!=system+0x178||
       q((void *)race,0x28)!=system+0x178||q((void *)system,0x1a8)!=system)
        return HORN_NATIVE;
    u32 types=w((void *)manager,0x38);
    u64 table=q((void *)manager,0x40);
    if(types<=5||types>64||!ptr(table))return HORN_NATIVE;
    u64 pool=q((void *)table,5*8);
    if(!ptr(pool)||q((void *)pool,0)!=base+0x11aad08||
       w((void *)pool,0x38)!=5)return HORN_NATIVE;
    u32 count=w((void *)pool,0x40);
    u64 nodes=q((void *)pool,0x48);
    /* Bound malformed storage; this does not change the game's pool capacity. */
    if(count>64||(count&&!ptr(nodes)))return HORN_NATIVE;
    for(u32 i=0;i<count;i++){
        u64 item=q((void *)nodes,0x20*i+0x10);
        if(!ptr(item)||q((void *)item,0)!=base+0x11aa898||
           w((void *)item,0x58)!=5||(int)w((void *)item,0x50)<0||
           b((void *)item,0x71)!=3||w((void *)item,0x4c8)!=racer)continue;
        u32 phase=b((void *)item,0x399),tick=w((void *)item,0x39c);
        /* Circle 54 begins native wind-up; 85 schedules Down. Down's fourth
         * update schedules Burst. Use the latched victim, not the route target. */
        if((phase==5&&tick>=54&&tick<=85)||(phase==6&&tick<4))return HORN_USE;
    }
    if(close_shells((void *)table,(void *)physics,racer,base)||
       close_racers((void *)race,(void *)physics,racer,base))return HORN_USE;
    if(lab_item_ordered_rank((void *)driver,(void *)proxy,(void *)role,base)==0)return HORN_KEEP;
    return HORN_NATIVE;
}

__attribute__((visibility("default")))
u32 lab_horn_blue_current(const void *planner,u64 base){
    if(!ptr((u64)planner)||!base||(base&4095)||base>=(1ull<<39)-0x1400000)
        return HORN_NATIVE;
    u64 inventory=q(planner,0xa8);
    if(!ptr(inventory))return HORN_NATIVE;
    u64 slot=q((void *)inventory,0x58);
    if(!ptr(slot)||w((void *)slot,0x9c)!=16)return HORN_NATIVE;
    /* The stock item functions also resolve scene component4 synchronously.
     * No pointer escapes this call. Native entry eligibility already passed. */
    const void *scene=((const void *(*)(void))(base+0x7f41fc))();
    return lab_horn_blue_decision(planner,scene,base);
}
