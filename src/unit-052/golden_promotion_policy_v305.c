#include "golden_promotion_policy_v305.h"
/* Earned ambition, not a speed bonus. The director may use strength to widen
 * this CPU's target band/catch eligibility under the authored CU/CD terms. */
typedef union {float f;u32 u;} GPBits;
_Static_assert(sizeof(GoldenBoundaryEvidence)==96,"Golden boundary scalar ABI");
_Static_assert(sizeof(GoldenPromotionConfig)==56,"Golden promotion configuration ABI");
_Static_assert(sizeof(GoldenPromotionCPU)==48,"Golden promotion CPU ABI");
_Static_assert(sizeof(GoldenPromotionFrame)==1768,"Golden promotion frame ABI");
_Static_assert(sizeof(GoldenPromotionState)==1200,"Golden promotion state ABI");
_Static_assert(sizeof(GoldenPromotionOutput)==48,"Golden promotion output ABI");
static void zero(void *p,u32 n){for(u32 j=0;j<n;j++)((volatile unsigned char *)p)[j]=0;}
static u32 scalar(float x){GPBits b={.f=x};return (b.u&0x7fffffffu)<0x7f800000u&&x>=-1.e9f&&x<=1.e9f;}
static u32 same_owner(const GoldenPromotionCPU *a,const GoldenPromotionCPU *b){
    return a->driver==b->driver&&a->role==b->role&&a->inventory==b->inventory;
}
GoldenPromotionConfig lab_golden_promotion_default(void){
    GoldenPromotionConfig c={GP_ABI,0,120,120,2,45,66,720,120,120,360,0,600.f,600.f};return c;
}
void lab_golden_promotion_clear(GoldenPromotionState *s){if(s)zero(s,sizeof(*s));}
static u32 config_valid(const GoldenPromotionConfig *c){
    if(!c||c->abi!=GP_ABI||c->enabled>1||c->reserved||!c->max_gap_ticks||c->max_gap_ticks>120||
       !c->max_event_age_ticks||c->max_event_age_ticks>120||!c->min_rank_gain||c->min_rank_gain>11||
       !scalar(c->min_progress_gain)||c->min_progress_gain<0||!scalar(c->lost_distance)||c->lost_distance<0)return 0;
    const u32 durations[]={c->sustain_ticks,c->attack_ticks,c->hold_ticks,c->release_ticks,
        c->lost_grace_ticks,c->cooldown_ticks};
    for(u32 j=0;j<6;j++)if(!durations[j]||durations[j]>36000)return 0;
    return c->attack_ticks<=c->hold_ticks;
}
static u32 same_config(const GoldenPromotionConfig *a,const GoldenPromotionConfig *b){
    const unsigned char *x=(const unsigned char *)a,*y=(const unsigned char *)b;
    for(u32 j=0;j<sizeof(*a);j++)if(x[j]!=y[j])return 0;return 1;
}
static u32 frame_valid(const GoldenPromotionFrame *f,u32 *mask,u32 index[GP_CAPACITY]){
    if(!f||!f->epoch||!f->generation||!f->tune_revision||f->continuous>1||!f->count||
       f->count>GP_CAPACITY||f->evidence_count>GP_CAPACITY)return 0;
    u32 ranks=0,ids=0,events=0;
    for(u32 j=0;j<f->count;j++){
        const GoldenPromotionCPU *p=&f->cpus[j];
        if(!p->driver||!p->role||!p->inventory||p->racer>=GP_CAPACITY||p->rank>=GP_CAPACITY||
           p->reserved||(p->flags&~GP_UNAVAILABLE)||!scalar(p->progress)||
           (ranks&(1u<<p->rank))||(ids&(1u<<p->racer)))return 0;
        for(u32 k=0;k<j;k++)if(p->driver==f->cpus[k].driver||p->role==f->cpus[k].role||
                               p->inventory==f->cpus[k].inventory)return 0;
        ranks|=1u<<p->rank;ids|=1u<<p->racer;index[p->racer]=j;
    }
    for(u32 j=0;j<f->evidence_count;j++){
        u32 r=f->evidence[j].racer;
        if(r>=GP_CAPACITY||(events&(1u<<r)))return 0;
        events|=1u<<r;
    }
    *mask=ids;return 1;
}
static void baseline(GoldenPromotionState *s,const GoldenPromotionConfig *c,
                     const GoldenPromotionFrame *f,u32 mask){
    /* A clock rewind inside the same generation cannot reopen old evidence. */
    u32 fence=f->tick;
    if(s->initialized&&s->epoch==f->epoch&&s->generation==f->generation){
        if(s->last_tick>fence)fence=s->last_tick;
        if(s->baseline_tick>fence)fence=s->baseline_tick;
    }
    zero(s,sizeof(*s));s->epoch=f->epoch;s->generation=f->generation;s->tune_revision=f->tune_revision;
    s->initialized=1;s->last_tick=f->tick;s->baseline_tick=fence;s->roster_mask=mask;s->config=*c;
    s->winner=GP_NO_RACER;
    for(u32 j=0;j<f->count;j++)s->owners[f->cpus[j].racer]=f->cpus[j];
}
static float smooth(u32 elapsed,u32 duration){
    float x=elapsed>=duration?1.f:(float)elapsed/(float)duration;return x*x*(3.f-2.f*x);
}
static u32 remaining(u32 n,u32 dt){return n>dt?n-dt:0;}
static void finish(GoldenPromotionState *s,const GoldenPromotionConfig *c){
    s->winner=GP_NO_RACER;s->strength=0;s->releasing=0;s->release_age=0;s->cooldown=c->cooldown_ticks;
}
static void publish(const GoldenPromotionState *s,GoldenPromotionOutput *o){
    for(u32 r=0;r<GP_CAPACITY;r++)o->pending_count+=s->records[r].pending;
    if(s->winner<GP_CAPACITY){
        const GoldenPromotionCPU *p=&s->owners[s->winner];
        o->driver=p->driver;o->role=p->role;o->inventory=p->inventory;
        o->racer=s->winner;o->strength=s->strength;o->status=s->releasing?GP_RELEASING:GP_ACTIVE;
    }else o->status=s->cooldown?GP_COOLDOWN:o->pending_count?GP_PENDING:GP_IDLE;
}
static u32 event_valid(const GoldenBoundaryEvidence *e,const GoldenPromotionFrame *f,
                      const GoldenPromotionState *s,const GoldenPromotionConfig *c,
                      const GoldenPromotionCPU *p){
    const GoldenPromotionRecord *r=&s->records[e->racer];
    return e->epoch==f->epoch&&e->generation==f->generation&&e->driver==p->driver&&
        e->role==p->role&&e->inventory==p->inventory&&e->rank_contract==GOLDEN_ITEM_EXIT_RANK_V1&&
        e->rank_site==0x3678&&e->uses&&e->first_sequence&&
        e->expiry_sequence>e->first_sequence&&e->first_sequence>r->seen_sequence&&
        e->start_tick>=s->baseline_tick&&e->start_tick<e->end_tick&&e->end_tick<=f->tick&&
        f->tick-e->end_tick<=c->max_event_age_ticks&&e->end_tick>r->seen_tick&&
        e->start_rank<GP_CAPACITY&&e->end_rank<GP_CAPACITY&&scalar(e->start_progress)&&scalar(e->end_progress);
}
u32 lab_golden_promotion_step(const GoldenPromotionConfig *c,const GoldenPromotionFrame *f,
                            GoldenPromotionState *s,GoldenPromotionOutput *out){
    if(!out)return GP_INVALID;
    GoldenPromotionOutput o={0};o.status=GP_INVALID;o.racer=GP_NO_RACER;
    if(!s){*out=o;return o.status;}
    u32 mask=0,index[GP_CAPACITY]={0};
    if(!config_valid(c)||!frame_valid(f,&mask,index)){
        lab_golden_promotion_clear(s);*out=o;return o.status;
    }
    if(!c->enabled){lab_golden_promotion_clear(s);o.status=GP_DISABLED;*out=o;return o.status;}
    u32 reset=0;
    if(!s->initialized)reset=GP_FIRST;
    else if(s->epoch!=f->epoch||s->generation!=f->generation||s->tune_revision!=f->tune_revision)reset=GP_SCOPE;
    else if(!same_config(&s->config,c))reset=GP_CONFIG;
    else if(s->roster_mask!=mask)reset=GP_ROSTER;
    else for(u32 j=0;j<f->count;j++)if(!same_owner(&s->owners[f->cpus[j].racer],&f->cpus[j])){reset=GP_ROSTER;break;}
    if(!reset&&(f->tick<s->last_tick||f->tick-s->last_tick>c->max_gap_ticks))reset=GP_CLOCK;
    if(!f->continuous)reset=GP_CONTINUITY;
    if(reset){baseline(s,c,f,mask);o.status=GP_RESET;o.reason=reset;*out=o;return o.status;}
    u32 dt=f->tick-s->last_tick;
    if(!dt){publish(s,&o);*out=o;return o.status;}
    s->last_tick=f->tick;
    /* Events that arrive during a cooldown are consumed, even on its final
     * step. They cannot sit queued until the cooldown expires. */
    u32 blocked=s->winner<GP_CAPACITY||s->cooldown;
    s->cooldown=remaining(s->cooldown,dt);
    float leader=f->cpus[0].progress;
    for(u32 j=1;j<f->count;j++)if(f->cpus[j].progress>leader)leader=f->cpus[j].progress;
    for(u32 r=0;r<GP_CAPACITY;r++)if(s->records[r].pending){
        GoldenPromotionRecord *v=&s->records[r];const GoldenPromotionCPU *p=&f->cpus[index[r]];
        if(p->flags||p->progress<v->end_progress||p->rank>v->start_rank||v->start_rank-p->rank<c->min_rank_gain)
            v->pending=0;
        else if(v->retained_ticks<c->sustain_ticks)v->retained_ticks+=dt;
    }
    for(u32 j=0;j<f->evidence_count;j++){
        const GoldenBoundaryEvidence *e=&f->evidence[j];
        if(!(mask&(1u<<e->racer)))continue;
        const GoldenPromotionCPU *p=&f->cpus[index[e->racer]];
        if(!event_valid(e,f,s,c,p))continue;
        GoldenPromotionRecord *v=&s->records[e->racer];
        v->seen_tick=e->end_tick;v->seen_sequence=e->expiry_sequence;
        v->pending=0;
        if(blocked||p->flags||e->start_rank<e->end_rank||e->start_rank-e->end_rank<c->min_rank_gain||
           e->end_progress-e->start_progress<c->min_progress_gain||p->progress<e->end_progress||
           p->rank>e->start_rank||e->start_rank-p->rank<c->min_rank_gain)continue;
        v->pending=1;v->retained_ticks=0;v->start_rank=e->start_rank;v->end_tick=e->end_tick;v->end_progress=e->end_progress;
    }
    u32 chosen=GP_NO_RACER,best_gain=0,best_tick=0;
    if(!blocked)for(u32 r=0;r<GP_CAPACITY;r++){
        GoldenPromotionRecord *v=&s->records[r];
        if(!v->pending||v->retained_ticks<c->sustain_ticks)continue;
        u32 gain=v->start_rank-f->cpus[index[r]].rank;
        if(chosen==GP_NO_RACER||gain>best_gain||(gain==best_gain&&v->end_tick<best_tick)){
            chosen=r;best_gain=gain;best_tick=v->end_tick;
        }
    }
    if(chosen<GP_CAPACITY){
        s->winner=chosen;s->winner_rank=f->cpus[index[chosen]].rank;s->age=0;s->lost_ticks=0;
        s->initial_gap=leader-f->cpus[index[chosen]].progress;s->strength=0;s->releasing=0;s->release_age=0;
        for(u32 r=0;r<GP_CAPACITY;r++)s->records[r].pending=0;
    }else if(s->winner<GP_CAPACITY){
        const GoldenPromotionCPU *p=&f->cpus[index[s->winner]];
        if(p->flags)finish(s,c);
        else{
            u32 competitive=p->rank<=s->winner_rank+1&&leader-p->progress<=s->initial_gap+c->lost_distance;
            u32 old_age=s->age,old_lost=s->lost_ticks;
            s->age+=dt;s->lost_ticks=competitive?0:s->lost_ticks+dt;
            if(s->releasing)s->release_age+=dt;
            else{
                u32 to_release=dt+1;
                if(s->age>=c->hold_ticks)to_release=c->hold_ticks-old_age;
                if(!competitive&&s->lost_ticks>=c->lost_grace_ticks){
                    u32 to_lost=c->lost_grace_ticks-old_lost;if(to_lost<to_release)to_release=to_lost;
                }
                if(to_release<=dt){
                    s->releasing=1;s->release_age=dt-to_release;
                    s->release_strength=smooth(old_age+to_release,c->attack_ticks);
                }
            }
            if(s->releasing){
                s->strength=s->release_strength*(1.f-smooth(s->release_age,c->release_ticks));
                if(s->release_age>=c->release_ticks){
                    u32 excess=s->release_age-c->release_ticks;
                    finish(s,c);s->cooldown=remaining(s->cooldown,excess);
                }
            }else s->strength=smooth(s->age,c->attack_ticks);
        }
    }
    publish(s,&o);*out=o;return o.status;
}
