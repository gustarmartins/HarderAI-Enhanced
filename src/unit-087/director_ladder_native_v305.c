#include "director_ladder_native_v305.h"
typedef union {float f;pp_u32 u;} DLBits;
static pp_u32 finite(float x){DLBits b={.f=x};return (b.u&0x7fffffffu)<0x7f800000u;}
static float maxf(float a,float b){return a>b?a:b;}
static float absf(float x){return x<0?-x:x;}
static float clamp(float x,float lo,float hi){return x<lo?lo:x>hi?hi:x;}
static float smooth(float u){u=clamp(u,0,1);return u*u*(3.f-2.f*u);}
static pp_u32 mix(pp_u32 x){x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;return x^(x>>16);}
static float scale_at(const LadderConfig *c,const LadderState *s,pp_u32 tick){
    float u=tick<=s->start_tick?0:(float)(tick-s->start_tick)/(float)c->ease_ticks;
    return s->from_scale+(s->to_scale-s->from_scale)*smooth(u);
}
pp_u32 lab_ladder_step(const LadderConfig *c,const LadderInput *i,LadderState *s,LadderOutput *out){
    if(!out)return DL_INVALID;LadderOutput o={0};
    if(!c||!i||!s||c->abi!=1||!c->ease_ticks||c->ease_ticks>36000||!c->cycle_ticks||
       c->cycle_ticks>36000||!c->max_gap_ticks||c->max_gap_ticks>3600||c->reserved[0]||c->reserved[1]||
       !finite(c->strength)||c->strength<0||c->strength>=1||!finite(c->half_depth_fraction)||
       c->half_depth_fraction<=0||c->half_depth_fraction>1||!i->epoch||!i->owner||!i->tune_revision||
       i->slot>=12||i->enabled>1||!finite(i->native_scale)||i->native_scale<=0||!finite(i->gap)||
       !finite(i->native_rung)||i->native_rung<=0||!finite(i->up)||i->up<0||!finite(i->down)||i->down<0){
        o.status=DL_INVALID;*out=o;return o.status;
    }
    float total=0;
    for(pp_u32 k=0;k<12;k++){
        if(!finite(i->increments[k])||(k?i->increments[k]<=0:i->increments[k]!=0)){
            o.status=DL_INVALID;*out=o;return o.status;
        }
        total+=i->increments[k];
    }
    if(!finite(total)||total<=0){o.status=DL_INVALID;*out=o;return o.status;}
    if(!i->enabled){*s=(LadderState){0};*out=o;return DL_OFF;}
    pp_u32 reset=s->initialized!=1||s->epoch!=i->epoch||s->owner!=i->owner||s->tune_revision!=i->tune_revision||
        s->reserved||s->reserved2||s->start_tick>s->tick||s->tick>i->tick||i->tick-s->tick>c->max_gap_ticks||
        !finite(s->scale)||s->scale<=0||!finite(s->from_scale)||s->from_scale<=0||
        !finite(s->to_scale)||s->to_scale<=0;
    LadderState n=*s;
    if(reset){
        n=(LadderState){i->epoch,i->owner,i->tune_revision,i->tick,i->tick,1,0,
            i->native_scale,i->native_scale,i->native_scale,0};o.status=DL_RESET;
    }else if(i->tick==s->tick){o.status=DL_DUPLICATE;}
    else{
        n.scale=scale_at(c,s,i->tick);
        if(i->native_scale!=s->to_scale){
            n.from_scale=n.scale;n.to_scale=i->native_scale;n.start_tick=i->tick;
        }
        n.tick=i->tick;o.status=DL_READY;
    }
    o.cycle=i->tick/c->cycle_ticks;
    o.cohort=(mix(i->seed)%3+o.cycle%3)%3;
    o.fraction=(float)(i->tick%c->cycle_ticks)/(float)c->cycle_ticks;
    float u=o.fraction,v=1.f-u;o.pulse=16.f*u*u*v*v;o.scale=n.scale;o.depth=total*n.scale;
    if(!finite(o.depth)||o.depth<=0){o=(LadderOutput){.status=DL_INVALID};*out=o;return o.status;}
    float weighted=0;
    for(pp_u32 k=1;k<12;k++){
        pp_u32 band=k<=4?DL_FRONT:k<=7?DL_MIDDLE:DL_REAR;
        o.increments[k]=i->increments[k]*(band==o.cohort?1.f-c->strength*o.pulse:1.f);
        weighted+=o.increments[k];
    }
    if(!finite(weighted)||weighted<=0){o=(LadderOutput){.status=DL_INVALID};*out=o;return o.status;}
    float running=0;
    for(pp_u32 k=1;k<12;k++){
        o.increments[k]=o.increments[k]/weighted*o.depth;running+=o.increments[k];o.targets[k]=running;
        if(!finite(running)||o.increments[k]<=0||running<=o.targets[k-1]){
            o=(LadderOutput){.status=DL_INVALID};*out=o;return o.status;
        }
    }
    o.gap=i->gap;o.target=o.targets[i->slot];o.error=o.gap-o.target;
    float local=o.increments[i->slot?i->slot:1];
    float rung=i->native_rung*(n.scale/i->native_scale);
    o.half_strength=maxf(rung,maxf(2.f*local,o.depth*c->half_depth_fraction));
    /* Divide after scaling both terms, avoiding overflow in h+abs(error). */
    float norm=maxf(o.half_strength,absf(o.error));
    float fraction=(o.error/norm)/(o.half_strength/norm+absf(o.error)/norm);
    o.formation=(o.error>=0?i->up:i->down)*fraction;
    if(!finite(o.error)||!finite(o.half_strength)||!finite(o.formation)){
        o=(LadderOutput){.status=DL_INVALID};*out=o;return o.status;
    }
    *s=n;*out=o;return o.status;
}
pp_u32 lab_ladder_pressure(const PressureBudget *b,const PressurePeer *self,const PressurePeer *reference,
    const LadderOutput *g,PressureRequest *r){
    if(!b||!self||!reference||!g||!r||!r->valid||r->valid>1||r->selected>1||r->action>3||r->reserved||
       g->status==DL_OFF||g->status==DL_INVALID||g->status>DL_DUPLICATE||g->reserved||
       !finite(g->formation)||!finite(self->sample.progress)||!finite(reference->sample.progress)||
       !finite(r->requested_ratio))return 0;
    const float *values=(const float *)b;
    for(pp_u32 k=0;k<sizeof(*b)/4;k++)if(!finite(values[k]))return 0;
    if(b->native_base<=0||b->up<0||b->down<0||b->cu_ratio<0||b->cd_ratio>0||b->cu_min>=b->cu_max||
       b->cd_min>=b->cd_max||b->catch_authority<0||b->catch_authority>1||
       b->formation_authority<0||b->formation_authority>1||g->formation< -b->down||g->formation>b->up)return 0;
    float gap=reference->sample.progress-self->sample.progress;
    if(!finite(gap))return 0;
    float up=b->cu_ratio*clamp((gap-b->cu_min)/(b->cu_max-b->cu_min),0,1);
    float down=b->cd_ratio*clamp((-gap-b->cd_min)/(b->cd_max-b->cd_min),0,1);
    float low=b->native_catch+b->catch_authority*(down-b->native_catch);
    float high=b->native_catch+b->catch_authority*(up-b->native_catch);
    PressureRequest n=*r;
    n.formation_term=b->native_formation+b->formation_authority*(g->formation-b->native_formation);
    n.catch_term=r->action?clamp(r->requested_ratio-b->native_base-n.formation_term,low,high):b->native_catch;
    n.wanted_ratio=(b->native_base+n.formation_term)+n.catch_term;
    if(!r->action)n.requested_ratio=n.wanted_ratio;
    n.positive_allowance=high+n.formation_term;n.negative_allowance=low+n.formation_term;
    if(!finite(n.formation_term)||!finite(n.catch_term)||!finite(n.wanted_ratio)||
       !finite(n.positive_allowance)||!finite(n.negative_allowance))return 0;
    *r=n;return 1;
}
