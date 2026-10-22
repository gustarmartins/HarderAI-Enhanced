/* Optional ABI extension: owned Banana/Bomb proximity for an eligible held Star.
 * This unit never writes game objects or requests/consumes an item. The caller
 * retains native item-use admission and request publication. Proximity lead is
 * an authored policy value, separate from the native primary collision radius.
 */
typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long long u64;
#define EXPORT __attribute__((visibility("default")))

/* Result bits: Banana1, Bomb body2, active/contact-triggered Bomb blast4. */
EXPORT const struct { u32 abi,max_pool_count;float reaction_lead; }
lab_star_item_hazard_config={1,64,90.f};

static u64 q(const void *p,u32 o){return *(const u64 *)((const u8 *)p+o);}
static u32 w(const void *p,u32 o){return *(const u32 *)((const u8 *)p+o);}
static u8 b(const void *p,u32 o){return *((const u8 *)p+o);}
static float f(const void *p,u32 o){return *(const float *)((const u8 *)p+o);}
static u32 ptr(u64 p){return p>=0x10000&&!(p&7)&&p<(1ull<<39)-0x2000;}

static u32 item_threat(const void *item,const void *race,const void *physics,
                       u32 racer,u32 kind,u64 base,u32 ordinary_bomb_mode){
    static const u32 vt[2]={0x11a75a8,0x11a8100};
    if(!ptr((u64)item)||q(item,0)!=base+vt[kind==9]||w(item,0x58)!=kind||
       (int)w(item,0x50)<0)return 0;
    u32 owner=w(item,0x5c),state=b(item,0x71);
    /* Retail F58 admits3/4/5/6. Other 2E8-special cases remain excluded. */
    if(owner>=12||state<3||state>6||b(item,0x1f0)||!w(item,0x2e8))return 0;
    u64 wrapper=q(item,0x40);
    if(!ptr(wrapper)||q((void *)wrapper,0)!=(u64)item)return 0;
    u64 collision=q((void *)wrapper,8);
    if(!ptr(collision)||(w((void *)collision,0x50)&0x100000u))return 0;
    u64 source=q(item,0x48);
    if(!ptr(source)||source!=q(race,0x50+8*owner)||
       q((void *)source,0)!=base+0x11b44f8)return 0;
    u64 source_role=q((void *)source,0x48),source_physics=q((void *)source,0x50);
    if(!ptr(source_role)||!ptr(source_physics)||w((void *)source_role,0xa8)!=owner||
       q((void *)source_role,0x28)!=source_physics||
       q((void *)source_physics,0xf8)!=source_role)return 0;
    u32 blast=kind==9&&(b(item,0x3d4)||b(item,0x3d5));
    /* Bomb Battle and unknown configuration refuse all Bomb classifications.
     * Native14298 bypasses self-protection for a live blast outside that mode. */
    if(kind==9&&!ordinary_bomb_mode)return 0;
    if(owner==racer&&!blast){
        int index=(int)w(item,b((void *)source_role,0xe7)?0x54:0x50);
        int protected_count=b(item,0x1cd)?3:10;
        if(index<protected_count)return 0;
    }
    float scale=f(item,0x128),lead=lab_star_item_hazard_config.reaction_lead;
    /* Native Banana scale1..2 while released; Bomb blast grows through18.
     * Smaller positive scales remain valid while held or transitioning. */
    if(!(scale>0.f&&scale<=(kind==2?2.f:18.f))||!(lead>=0.f&&lead<=10000.f))return 0;
    float radius=(kind==2?3.f:5.f)*scale+lead;
    float x=f(item,0x12c)+f(item,0x1d0),y=f(item,0x130)+f(item,0x1d4);
    float z=f(item,0x134)+f(item,0x1d8);
    /* Native primary sphere43830 includes this displacement when byte30 is set.
     * Kart pose is its physics position; this remains a proximity envelope. */
    if(b((void *)wrapper,0x30)){
        x+=f(item,0x12c)-f(item,0x138);
        y+=f(item,0x130)-f(item,0x13c);
        z+=f(item,0x134)-f(item,0x140);
    }
    x-=f(physics,0x2c);y-=f(physics,0x30);z-=f(physics,0x34);
    if(!(x*x+y*y+z*z<=radius*radius))return 0;
    return kind==2?1:(blast?4:2);
}

EXPORT u32 lab_star_item_hazard_decision(const void *planner,const void *scene,u64 base){
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
       (w((void *)role,0x1cc)&0x41604019u)||(int)w((void *)role,0x1e4)>0||
       (int)w((void *)role,0x248)>0||w((void *)role,0x1ec)==600)return 0;
    u32 racer=w((void *)role,0xa8);u64 slot=q((void *)inventory,0x58);
    if(racer>=12||!ptr(slot)||w((void *)slot,0x9c)!=8||
       (int)w((void *)inventory,0xc0)>0||b(planner,0x41)!=3)return 0;
    if(q(scene,0)!=base+0x125e158||!(b(scene,0x180)&4))return 0;
    u64 system=q(scene,0x1b0);
    if(!ptr(system)||q((void *)system,0)!=base+0x125a060)return 0;
    u64 race=q((void *)system,0x238),manager=q((void *)system,0x240);
    if(!ptr(race)||q((void *)race,0)!=base+0x11b4308||
       q((void *)race,0x50+8*racer)!=proxy||!ptr(manager)||
       q((void *)manager,0)!=base+0x11a7188||!b((void *)manager,0x170)||
       q((void *)manager,0x28)!=system+0x178||
       q((void *)race,0x28)!=system+0x178||q((void *)system,0x1a8)!=system)return 0;
    u32 types=w((void *)manager,0x38);u64 table=q((void *)manager,0x40);
    if(types<=2||types>64||!ptr(table))return 0;
    u32 ordinary_bomb_mode=0;
    static const u32 kinds[2]={2,9},pool_vtables[2]={0x11a79b8,0x11a8620};
    u32 result=0;
    for(u32 n=0;n<2;n++){
        u32 kind=kinds[n];if(kind>=types)continue;
        u64 pool=q((void *)table,8*kind);
        if(!ptr(pool)||q((void *)pool,0)!=base+pool_vtables[n]||w((void *)pool,0x38)!=kind)continue;
        u32 count=w((void *)pool,0x40);u64 nodes=q((void *)pool,0x48);
        if(count>lab_star_item_hazard_config.max_pool_count||(count&&!ptr(nodes)))continue;
        /* Query mode only for an owned nonempty Bomb pool, once per decision.
         * No stale hardcoded race assumption permits the self-hit exception. */
        if(kind==9&&count){
            const void *config=((const void *(*)(void))(base+0x87c244))();
            if(ptr((u64)config)){
                u32 mode=w(config,8),submode=w(config,12);
                /* +C is battle-only. Stale values cannot disable GP/VS/TA.
                 * Native3848C0 bounds battle rules0..4; rule3 is Bomb. */
                ordinary_bomb_mode=mode<3||(mode==3&&submode<=4&&submode!=3);
            }
        }
        for(u32 i=0;i<count;i++)result|=item_threat((void *)q((void *)nodes,0x20*i+0x10),
            (void *)race,(void *)physics,racer,kind,base,ordinary_bomb_mode);
    }
    return result;
}
