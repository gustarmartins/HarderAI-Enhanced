#ifndef HARDERAI_DIRECTOR_LADDER_RUNTIME_V305_H
#define HARDERAI_DIRECTOR_LADDER_RUNTIME_V305_H
#include "director_ladder_native_v305.h"
typedef struct {
    LadderInput input;
    dc_u64 manager,parameters,parameter_owner;
    dc_u32 applied,counter,trigger,scale_bits;
    dc_u32 geometry[60];
} DirectorLadderContext;
typedef struct {
    dc_u64 sequence,updates,director_sequence,controller,parameters,clock;
    dc_u32 requested,blended;
    dc_u32 native_applied,native_counter,native_trigger,native_scale_bits;
    LadderInput input;
    LadderState state;
    LadderOutput output;
} DirectorLadderRecord;
_Static_assert(sizeof(DirectorLadderRecord)==392,"Ladder runtime record ABI1");
DR_EXPORT const LadderConfig lab_director_ladder_config={1,900,1200,120,.4f,.1f,{0,0}};
DR_EXPORT dc_u32 lab_director_ladder_enabled=1;
DR_EXPORT DirectorLadderRecord lab_director_ladder_records[12];

static dc_u32 dl_close(float a,float b){
    if(!dr_finite(a)||!dr_finite(b))return 0;
    float d=a-b;if(d<0)d=-d;
    float magnitude=b<0?-b:b;
    return d<=.0001f*(magnitude>1.f?magnitude:1.f);
}
static float dl_field(const DirectorLadderContext *p,dc_u32 offset){
    FloatBits bits={.u=p->geometry[(offset-0x14)/4]};return bits.f;
}
static dc_u32 dl_collect(DirectorLadderContext *p,dc_u64 params,dc_u64 manager,dc_u64 group,const DirectorCoreInput *i){
    LadderInput *n=&p->input;
    n->enabled=__atomic_load_n(&lab_director_ladder_enabled,__ATOMIC_ACQUIRE);
    if(n->enabled>1)return 0;
    if(!n->enabled)return 1;
    p->manager=manager;p->parameters=params;p->parameter_owner=dr_q(group,0x70);
    if(!dr_ptr(p->parameter_owner)||dr_q(p->parameter_owner,0x18)!=params||dr_q(manager,0x78)!=params)return 0;
    p->applied=dr_b(manager,0x236);p->counter=dr_w(manager,0x238);
    p->trigger=dr_w(manager,0x23c);p->scale_bits=dr_w(manager,0x240);
    FloatBits native_scale={.u=p->scale_bits};
    if(p->applied>1||!dr_finite(native_scale.f)||native_scale.f<=0)return 0;
    for(dc_u32 k=0;k<60;k++)p->geometry[k]=dr_w(params,0x14+4*k);
    n->epoch=i->epoch;n->owner=i->owner;n->tune_revision=i->tune_revision;
    n->tick=i->tick;n->seed=i->seed;n->native_rung=i->native_rung;n->up=i->up;n->down=i->down;
    /* Saved values include native CPU-count normalization. Never normalize the
     * count again and never treat the current FCRA ladder as the saved basis. */
    float saved_depth=dl_field(p,0xf0),active_depth=dl_field(p,0x70);
    if(!dr_finite(saved_depth)||saved_depth<=0||!dr_finite(active_depth)||active_depth<=0)return 0;
    n->native_scale=active_depth/saved_depth;
    if(!dr_finite(n->native_scale)||n->native_scale<=0||
       !dl_close(n->native_scale,p->applied?native_scale.f:1.f)||
       dl_field(p,0x74)!=i->up||dl_field(p,0x78)!=i->down)return 0;
    float sum=0;
    for(dc_u32 k=0;k<12;k++){
        float gap=dl_field(p,0x94+4*k);n->increments[k]=gap;sum+=gap;
        if(!dr_finite(gap)||(k?gap<=0:gap!=0)||!dl_close(sum,dl_field(p,0xc4+4*k))||
           !dl_close(gap*n->native_scale,dl_field(p,0x14+4*k))||
           !dl_close(sum*n->native_scale,i->targets[k])||dl_field(p,0x44+4*k)!=i->targets[k])return 0;
    }
    dc_u32 found=0;float leader=0,own=0;
    for(dc_u32 k=0;k<i->count;k++){
        const DirectorCorePeer *peer=&i->peers[k];
        if(!k||peer->progress>leader)leader=peer->progress;
        if(peer->racer==i->racer&&peer->owner==i->owner){found++;n->slot=peer->slot;own=peer->progress;}
    }
    if(found!=1)return 0;n->gap=leader-own;return 1;
}
static dc_u32 dl_same(const DirectorLadderContext *p,dc_u64 group){
    if(p->input.enabled!=__atomic_load_n(&lab_director_ladder_enabled,__ATOMIC_ACQUIRE))return 0;
    if(!p->input.enabled)return 1;
    if(dr_q(group,0x68)!=p->manager||dr_q(group,0x70)!=p->parameter_owner||
       dr_q(p->parameter_owner,0x18)!=p->parameters||dr_q(p->manager,0x78)!=p->parameters||
       dr_b(p->manager,0x236)!=p->applied||dr_w(p->manager,0x238)!=p->counter||
       dr_w(p->manager,0x23c)!=p->trigger||dr_w(p->manager,0x240)!=p->scale_bits)return 0;
    for(dc_u32 k=0;k<60;k++)if(dr_w(p->parameters,0x14+4*k)!=p->geometry[k])return 0;
    return 1;
}
static dc_u32 dl_prepare(const DirectorLadderContext *p,const DirectorCoreInput *i,
    dc_u64 parent_sequence,DirectorLadderRecord *n){
    const DirectorLadderRecord *old=&lab_director_ladder_records[i->racer];
    if((old->sequence&1)||old->sequence>~0ull-2||old->updates==~0ull)return 0;
    dr_zero(n,sizeof(*n));n->input=p->input;n->state=old->state;
    n->native_applied=p->applied;n->native_counter=p->counter;
    n->native_trigger=p->trigger;n->native_scale_bits=p->scale_bits;
    if(!p->input.enabled){n->output.status=DL_OFF;n->state=(LadderState){0};return 1;}
    if(old->director_sequence!=parent_sequence)n->state=(LadderState){0};
    return lab_ladder_step(&lab_director_ladder_config,&n->input,&n->state,&n->output)!=DL_INVALID;
}
static void dl_publish(const DirectorCoreInput *i,const DirectorLadderRecord *n,
    dc_u64 controller,dc_u64 parameters,dc_u64 clock,dc_u64 director_sequence){
    DirectorLadderRecord *r=&lab_director_ladder_records[i->racer];
    __atomic_add_fetch(&r->sequence,1,__ATOMIC_ACQ_REL);
    dr_copy((unsigned char *)r+16,(const unsigned char *)n+16,sizeof(*n)-16);
    r->updates++;r->controller=controller;r->parameters=parameters;r->clock=clock;r->director_sequence=director_sequence;
    __atomic_add_fetch(&r->sequence,1,__ATOMIC_RELEASE);
}
#endif
