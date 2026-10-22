/* Successful blue legs return to the existing opportunity-check cadence.
 * A failed leg still gets its original slot/gap cooldown. Native steering,
 * charge, timers, route reservation and every later entry gate remain intact.
 * Other active boosts/Star retain the prior wait until a combined boost/route
 * planner has qualified their overlap. This helper never writes game objects. */
EXPORT const u32 lab_snaking_success_recheck_config[4]={1,6,3,0};

EXPORT u32 lab_snaking_success_recheck(void *physics,void *role,u32 racer,u32 previous){
    if(racer>=12||!previous||!ptr((u64)physics)||!ptr((u64)role)||
       q(physics,0xf8)!=(u64)role||q(role,0x28)!=(u64)physics||
       b(role,0xd0)||b(role,0xd2)!=1||b(role,0xd3)!=1||
       !b(role,0xe1)||b(role,0xe7)||w(role,0x1e4)||
       (w(role,0x1cc)&0x01204018u))return previous;
    u64 dash=q(physics,0x118);
    if(!ptr(dash)||q((void *)dash,8)!=(u64)role)return previous;
    u32 on=b((void *)dash,0x10);
    if(on>1)return previous;
    if(on){
        /* Exact native blue kind3; do not preempt a dominant stronger kind. */
        if(w((void *)dash,0x7c)!=3)return previous;
        u32 remaining=w((void *)dash,0x14),kind=w((void *)dash,0x28);
        u32 duration=w((void *)dash,0x90);
        if(!remaining||!kind||!duration||duration>36000||
           remaining>duration||kind>duration)return previous;
    }
    u32 retry=lab_snaking_success_recheck_config[1]+racer%lab_snaking_success_recheck_config[2];
    return previous<retry?previous:retry;
}
