#ifndef HARDERAI_DIRECTOR_PRESSURE_RUNTIME_V305_H
#define HARDERAI_DIRECTOR_PRESSURE_RUNTIME_V305_H
#include "director_pressure_native_v305.h"
typedef struct {
    dc_u32 enabled,count,reference,cpu_mask;
    DirectorCoreConfig policy;
    PressureNativeCapture actors[12];
} DirectorPressureContext;
typedef struct {
    dc_u64 sequence,updates,epoch,owner,controller,clock,director_sequence,reference_owner,tune_revision;
    dc_u32 tick,count,reference,cpu_mask,status,phase,active_history,reserved;
    PressurePaceState states[12];
    PressurePeer peers[12];
    PressureBlendState blend;
    PressureSelection selection;
    PressureRequest request;
    PressureBlendOutput output;
} DirectorPressureRecord;
_Static_assert(sizeof(DirectorPressureContext)==1880,"Pressure context ABI1");
_Static_assert(sizeof(DirectorPressureRecord)==1792,"Pressure record ABI1");
enum { DPR_COLD=1,DPR_ACTIVE=2,DPR_OPENING=3,DPR_NO_REFERENCE=4,DPR_BOOST=5,DPR_UNAVAILABLE=6 };
DR_EXPORT const dc_u32 lab_director_pressure_config[8]={1,sizeof(DirectorPressureRecord),12,30,720,600,54,0};
#ifndef LAB_PRESSURE_DEFAULT
#define LAB_PRESSURE_DEFAULT 1
#endif
DR_EXPORT dc_u32 lab_director_pressure_enabled=LAB_PRESSURE_DEFAULT;
DR_EXPORT DirectorPressureRecord lab_director_pressure_records[12];

static dc_u32 pr_begin(DirectorPressureContext *p){
    p->enabled=__atomic_load_n(&lab_director_pressure_enabled,__ATOMIC_ACQUIRE);
    if(p->enabled>1)return 0;
    p->reference=12;
    /* Keep one coherent policy copy inside both compared native snapshots. */
    for(dc_u32 k=0;k<sizeof(p->policy)/4;k++)((dc_u32 *)&p->policy)[k]=dr_w((dc_u64)&lab_director_config.policy,4*k);
    return 1;
}
static dc_u32 pr_same(const DirectorPressureContext *p){
    if(p->enabled!=__atomic_load_n(&lab_director_pressure_enabled,__ATOMIC_ACQUIRE))return 0;
    if(p->enabled)for(dc_u32 k=0;k<sizeof(p->policy)/4;k++)
        if(((const dc_u32 *)&p->policy)[k]!=dr_w((dc_u64)&lab_director_config.policy,4*k))return 0;
    return 1;
}
static dc_u64 pr_revision(const DirectorCoreInput *i,const DirectorPressureContext *p){
    dc_u64 hash=i->tune_revision;
    for(dc_u32 k=0;k<sizeof(p->policy)/4;k++)hash=dr_hashword(hash,((const dc_u32 *)&p->policy)[k]);
    return hash?hash:1;
}
static void pr_seed_blend(PressureBlendState *s,const DirectorCoreInput *i,dc_u64 revision,const DirectorCoreOutput *out){
    *s=(PressureBlendState){i->epoch,i->owner,revision,i->tick,1,out->catch_term,out->formation_term,0,0};
}
DR_EXPORT const dc_u32 lab_director_turn_calibration_config[4]={1,16,4,5};
/* The existing per-actor director lock owns both records. All calculation is
 * private scratch until the caller repeats its native lifetime checks. */
static dc_u32 pr_prepare(const DirectorPressureContext *p,const DirectorCoreInput *i,
    const DirectorOpeningOutput *opening,const LadderOutput *ladder,dc_u64 parent_sequence,DirectorPressureRecord *n,
    DirectorCoreState *state,DirectorCoreOutput *out){
    if(!p->enabled||!p->policy.enabled||out->status==DC_PASSTHROUGH||
       i->racer>=12||p->count>12||!p->count)return 0;
    const DirectorPressureRecord *old=&lab_director_pressure_records[i->racer];
    if((old->sequence&1)||old->sequence>~0ull-2||old->updates==~0ull)return 0;
    dc_u64 revision=pr_revision(i,p);
    dc_u64 reference=p->reference<p->count?p->actors[p->reference].sample.owner:0;
    dc_u32 reset=old->epoch!=i->epoch||old->owner!=i->owner||old->tune_revision!=revision||
        old->count!=p->count||old->reference_owner!=reference||old->cpu_mask!=p->cpu_mask||
        old->director_sequence!=parent_sequence||i->tick<old->tick;
    if(reset){dr_zero(n,sizeof(*n));n->sequence=old->sequence;n->updates=old->updates;}
    else dr_copy(n,old,sizeof(*n));
    n->epoch=i->epoch;n->owner=i->owner;n->tune_revision=revision;n->reference_owner=reference;
    n->tick=i->tick;n->count=p->count;n->reference=p->reference;n->cpu_mask=p->cpu_mask;n->phase=out->phase;n->reserved=0;
    n->status=DPR_COLD;n->request=(PressureRequest){0};n->selection=(PressureSelection){0};n->output=(PressureBlendOutput){0};
    PressurePaceConfig pace=lab_pressure_pace_default();
    for(dc_u32 k=0;k<p->count;k++){
        /* Private config word0 allows only PP_DRIFT; native state4/5 is
         * required on this qualified actor before enabling that exception. */
        pace.reserved[0]=(p->actors[k].drift_state==4||p->actors[k].drift_state==5)?PP_DRIFT:0;
        n->peers[k].sample=p->actors[k].sample;
        lab_pressure_pace_sample(&pace,i->epoch,i->tick,&n->peers[k].sample,&n->states[k],&n->peers[k].pace);
    }
    PressureSelectionConfig selection={1,out->phase==DC_BATTLE?3:2,600,p->cpu_mask,.3f,6.f,20.f,60.f,2.f,60.f};
    dc_u32 own=12;
    for(dc_u32 k=0;k<p->count;k++)if(n->peers[k].sample.racer==i->racer&&n->peers[k].sample.owner==i->owner)own=k;
    if(own==12)return 0;
    if(!reference)n->status=DPR_NO_REFERENCE;
    else if(lab_pressure_select(&selection,n->peers,p->count,p->reference,i->tick,i->seed,&n->selection)){
        /* Native-progress spread gives breathing room; individual containment
         * and gradual distant recovery still use the same authored allowance. */
        if(out->phase==DC_SPREAD)n->selection.selected_mask=0;
        dc_u32 selected=(n->selection.selected_mask>>i->racer)&1;
        if(i->promotion_strength>0){
            selected=1;selection.recovery_seconds*=1.f-.25f*i->promotion_strength;
            selection.far_seconds+=3.f*i->promotion_strength;
        }
        PressureBudget budget={i->native_base,i->native_catch,i->native_formation,
            i->up,i->down,i->cu_min,i->cu_max,i->cu_ratio,i->cd_min,i->cd_max,i->cd_ratio,
            p->policy.catch_authority,p->policy.formation_authority};
        lab_pressure_request(&selection,&budget,&n->peers[own],&n->peers[p->reference],selected,&n->request);
        if(ladder&&ladder->status!=DL_OFF&&n->request.valid&&
           !lab_ladder_pressure(&budget,&n->peers[own],&n->peers[p->reference],ladder,&n->request))return 0;
    }
    /* The already-qualified opening preset owns its complete hold/fade. Keep
     * learning clean pace; seed the blend from its actual outgoing correction. */
    if(opening->status==DO_HOLD||opening->status==DO_FADE){
        n->status=DPR_OPENING;pr_seed_blend(&n->blend,i,revision,out);return 1;
    }
    if(!n->active_history&&!n->request.valid){pr_seed_blend(&n->blend,i,revision,out);return 1;}
    if(!n->blend.initialized)pr_seed_blend(&n->blend,i,revision,out);
    if(n->request.valid)n->active_history=1;
    PressureBlendInput blend={i->epoch,i->owner,revision,i->tick,n->peers[own].sample.flags,1,0,
        i->native_base,i->native_catch,i->native_formation,n->request};
    PressureBlendConfig response={1,p->policy.max_gap_ticks,p->policy.response_ticks,0};
    if(lab_pressure_blend(&response,&blend,&n->blend,&n->output)==PP_INVALID)return 0;
    out->sum=n->output.ratio;out->catch_term=n->output.catch_term;out->formation_term=n->output.formation_term;
    /* The pressure record owns the new explanation. Do not label the old
     * formation-band proposal or catch decomposition as its active cause. */
    out->band_low=0;out->band_high=0;out->target=0;
    if(ladder&&ladder->status!=DL_OFF&&n->request.valid){
        out->target=ladder->target;out->gap=ladder->gap;out->rung=ladder->half_strength;
    }
    out->human_term=0;out->leader_term=0;out->local_term=0;
    state->catch_term=out->catch_term;state->formation_term=out->formation_term;
    out->cohort=n->request.selected?(i->promotion_strength>0?DC_EARNED:DC_ATTACK):
        n->request.action==PP_FIELD_RECOVERY?DC_RECOVERY:DC_FIELD;
    state->cohort=out->cohort==DC_EARNED?DC_ATTACK:out->cohort==DC_RECOVERY?DC_FIELD:out->cohort;
    if(n->peers[own].sample.flags&~(PP_BOOST|PP_DRIFT))n->status=DPR_UNAVAILABLE;
    else if(n->peers[own].sample.flags&PP_BOOST)n->status=DPR_BOOST;
    else if(n->request.valid)n->status=DPR_ACTIVE;
    return 1;
}
static void pr_publish(const DirectorCoreInput *i,const DirectorPressureRecord *n,
    dc_u64 controller,dc_u64 clock,dc_u64 director_sequence){
    DirectorPressureRecord *r=&lab_director_pressure_records[i->racer];
    __atomic_add_fetch(&r->sequence,1,__ATOMIC_ACQ_REL);
    /* Sequence/updates stay monotonic even when race or reference changes. */
    dr_copy((unsigned char *)r+16,(const unsigned char *)n+16,sizeof(*n)-16);
    r->updates++;r->controller=controller;r->clock=clock;r->director_sequence=director_sequence;
    __atomic_add_fetch(&r->sequence,1,__ATOMIC_RELEASE);
}
#endif
