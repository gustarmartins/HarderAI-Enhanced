/* Private staged include after Golden's checked native collector.
 * All native reads below occur at the same main-thread item-exit lifetime.
 * AI workers receive only an atomic scalar plan; they never consume evidence. */
#ifndef LAB_GOLDEN_PROMOTION_BRIDGE_V305_H
#define LAB_GOLDEN_PROMOTION_BRIDGE_V305_H
#include "golden_promotion_policy_v305.h"
#include "golden_promotion_publication_v305.h"
EXPORT const GoldenPromotionConfig lab_golden_promotion_config={
    GP_ABI,LAB_GOLDEN_PROMOTION_ENABLED,120,120,2,45,66,720,120,120,360,0,600.f,600.f};
EXPORT GoldenPromotionState lab_golden_promotion_state;
EXPORT GoldenPromotionPublication lab_golden_promotion_publication;
/* Main-boundary counters: qualified, invalidated, pending, active, released,
 * publication refused. They do not count actual Golden physical boosts. */
EXPORT u64 lab_golden_promotion_counters[6];

static void golden_policy_invalidate(void){
    lab_golden_promotion_clear(&lab_golden_promotion_state);
    lab_golden_promotion_unpublish(&lab_golden_promotion_publication);
    lab_golden_promotion_counters[1]++;
}
static u64 golden_tune(const GSnapshot *v){
    u64 fm=gq(v->group,0x68);
    if(!ptr(fm)||gq(fm,0x28)!=v->group)return 0;
    u64 params=gq(fm,0x78),owner=gq(v->group,0x70);
    if(!ptr(params)||!ptr(owner)||gq(owner,0x18)!=params)return 0;
    u64 hash=1469598103934665603ull;
    for(u32 off=0;off<=0x10;off+=4)hash=(hash^gw(params,off))*1099511628211ull;
    for(u32 off=0x74;off<=0x90;off+=4)
        if(off!=0x7c&&off!=0x80&&off!=0x88&&off!=0x8c)hash=(hash^gw(params,off))*1099511628211ull;
    for(u32 off=0x94;off<=0x100;off+=4)hash=(hash^gw(params,off))*1099511628211ull;
    return hash?hash:1;
}
static void golden_policy_commit(const GSnapshot *v){
    GoldenPromotionFrame frame;zero(&frame,sizeof(frame));
    frame.epoch=v->epoch;frame.generation=v->generation;frame.tick=v->tick;
    frame.continuous=1;frame.tune_revision=golden_tune(v);
    if(!frame.tune_revision||!v->roster_mask){golden_policy_invalidate();return;}
    for(u32 i=0;i<v->count;i++)if(v->roster_mask&(1u<<i)){
        const GOwner *a=&v->owners[i];GoldenPromotionCPU *p=&frame.cpus[frame.count++];
        p->driver=a->driver;p->role=a->role;p->inventory=a->inventory;
        p->racer=i;p->rank=a->rank;p->progress=a->progress;
        p->flags=(v->mask&(1u<<i))?0:GP_UNAVAILABLE;
        GoldenBoundaryEvidence *e=&frame.evidence[frame.evidence_count];
        if(lab_golden_boundary_take(i,v->epoch,v->generation,a->driver,a->role,a->inventory,v->tick,e))
            frame.evidence_count++;
    }
    GoldenPromotionOutput output;
    u32 result=lab_golden_promotion_step(&lab_golden_promotion_config,&frame,
                                        &lab_golden_promotion_state,&output);
    lab_golden_promotion_counters[0]++;
    if(result==GP_INVALID||result==GP_DISABLED){lab_golden_promotion_unpublish(&lab_golden_promotion_publication);return;}
    if(result==GP_PENDING)lab_golden_promotion_counters[2]++;
    if(result==GP_ACTIVE)lab_golden_promotion_counters[3]++;
    if(result==GP_RELEASING)lab_golden_promotion_counters[4]++;
    GoldenPromotionView value;zero(&value,sizeof(value));
    /* Match the existing director's epoch convention in both maintained builds. */
    value.epoch=v->generation;
    value.generation=v->generation;value.tick=v->tick;value.mask=v->mask;
    for(u32 i=0;i<v->count;i++)if(v->mask&(1u<<i)){
        const GOwner *a=&v->owners[i];GoldenPromotionCell *p=&value.actors[i];
        p->driver=a->driver;p->role=a->role;p->inventory=a->inventory;
        if(output.racer==i&&output.driver==a->driver&&output.role==a->role&&output.inventory==a->inventory){
            GFloat f={.value=output.strength};p->strength_bits=f.word;
        }
    }
    if(!lab_golden_promotion_publish(&lab_golden_promotion_publication,&value)){
        lab_golden_promotion_counters[5]++;golden_policy_invalidate();
    }
}
#endif
