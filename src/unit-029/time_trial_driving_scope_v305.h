/* Private solo Time Attack driving eligibility. No stores, cached pointers,
 * catch manager or fabricated formation wrapper. Called from a native-owned
 * movement callback; this predicate is not a host-side lifetime lease. */
static u64 tts_q(u64 p,u32 o){return *(const volatile u64 *)(p+o);}
static u32 tts_w(u64 p,u32 o){return *(const volatile u32 *)(p+o);}
static u32 tts_b(u64 p,u32 o){return *(const volatile u8 *)(p+o);}
static u32 tts_ptr(u64 p){return p>=0x10000 && !(p&7) && p<(1ull<<39)-0x3000;}

static u32 tts_admitted(u64 base,u64 producer){
    if((base&4095) || base>(1ull<<39)-0x1400000 || !tts_ptr(producer))return 0;
    u64 root=tts_q(base,0x12f6388);if(!tts_ptr(root))return 0;
    u64 single=tts_q(root,0);if(!tts_ptr(single))return 0;
    u64 service=tts_q(single,0x28);if(!tts_ptr(service))return 0;
    u64 holder=tts_q(service,0x240);if(!tts_ptr(holder))return 0;
    u64 config=tts_q(holder,0x10);if(!tts_ptr(config))return 0;
    if(tts_w(config,0)||tts_w(config,4)||tts_w(config,8)!=2)return 0;
    u32 engine=tts_w(config,0x10);if(engine!=2 && engine!=3)return 0;
    u64 link=tts_q(single,0x20);if(!tts_ptr(link))return 0;
    u64 scene=tts_q(link,8);
    if(!tts_ptr(scene)||tts_q(scene,0)!=base+0x125e158||!(tts_b(scene,0x180)&4))return 0;
    u64 system=tts_q(scene,0x1b0);
    if(!tts_ptr(system)||tts_q(system,0)!=base+0x125a060||tts_w(system,0xb8)!=4)return 0;
    u64 race=tts_q(system,0x238);
    if(!tts_ptr(race)||tts_q(race,0)!=base+0x11b4308||tts_w(race,0xc0)!=1)return 0;
    u64 group=tts_q(race,0xb0),contexts=tts_q(race,0xc8);
    if(!tts_ptr(group)||!tts_ptr(contexts)||tts_q(group,0x28)!=race||
       tts_w(group,0x58)!=1||tts_q(group,0x68))return 0;
    u64 context=tts_q(contexts,0);if(!tts_ptr(context)||tts_w(context,0x40))return 0;
    u64 driver=tts_q(context,0x18),role=tts_q(context,8),drivers=tts_q(group,0x60);
    if(!tts_ptr(driver)||!tts_ptr(role)||!tts_ptr(drivers)||tts_q(drivers,0)!=driver||
       tts_q(driver,0x60)!=group||tts_q(role,8)!=context||tts_w(role,0xa8)||
       tts_w(role,0xd0)!=0x01010001u||tts_b(role,0xe1)!=1||tts_b(role,0xe7))return 0;
    u64 state=tts_q(driver,0x50),adapter=tts_q(driver,0x88),physics=tts_q(role,0x28);
    if(!tts_ptr(state)||!tts_ptr(adapter)||!tts_ptr(physics)||tts_q(state,0x88)!=driver||
       tts_b(state,0x61)!=2||tts_q(adapter,0x48)!=role||tts_q(adapter,0x50)!=physics||
       tts_q(physics,0xf8)!=role)return 0;
    u64 navigation=tts_q(state,0xb0),ratio=tts_q(state,0x90),ai=tts_q(driver,0x90);
    if(!tts_ptr(navigation)||!tts_ptr(ratio)||!tts_ptr(ai)||
       tts_q(navigation,0x58)!=driver||tts_q(navigation,0x70)!=producer||
       tts_q(producer,0x70)!=driver||tts_q(ratio,0)!=base+0x11ae660||
       tts_q(ratio,0x28)!=state||tts_q(ratio,0x58)!=driver||
       tts_q(ratio,0x40)||tts_q(ratio,0x48)||tts_q(ai,0x10)!=role||
       tts_q(physics,0x100)!=ai||tts_q(role,0x18)!=ai||tts_b(ai,0x18)!=1)return 0;
    u64 clocks=tts_q(system,0x218);if(!tts_ptr(clocks))return 0;
    u64 clock=tts_q(clocks,0x50);if(!tts_ptr(clock)||tts_w(clock,0x38)!=6)return 0;
    u32 tick=tts_w(clock,0x44);if(!tick || tick>=0x80000000u)return 0;
    /* Recheck roots and treatment; no ownership claim survives this call. */
    return tts_q(base,0x12f6388)==root && tts_q(root,0)==single &&
        tts_q(single,0x28)==service && tts_q(service,0x240)==holder && tts_q(holder,0x10)==config &&
        !tts_w(config,0) && !tts_w(config,4) && tts_w(config,8)==2 && tts_w(config,0x10)==engine &&
        tts_q(single,0x20)==link && tts_q(link,8)==scene && tts_q(scene,0x1b0)==system &&
        tts_q(system,0x238)==race && tts_q(race,0xb0)==group && tts_q(race,0xc8)==contexts &&
        tts_q(contexts,0)==context && tts_q(context,0x18)==driver && tts_q(context,8)==role &&
        tts_w(role,0xd0)==0x01010001u && tts_b(role,0xe1)==1 && !tts_b(role,0xe7) &&
        tts_q(driver,0x50)==state && tts_q(state,0xb0)==navigation &&
        tts_q(navigation,0x70)==producer && tts_b(state,0x61)==2 &&
        tts_q(system,0x218)==clocks && tts_q(clocks,0x50)==clock &&
        tts_w(clock,0x38)==6 && tts_w(clock,0x44)==tick;
}
