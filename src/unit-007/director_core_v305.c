#include "director_core_v305.h"
#include "director_opening_core_v305.h"
/* Component director, private alpha. The native adapter owns all object reads,
 * concurrency, lifecycle qualification and final native limits. */
typedef union {float f;dc_u32 u;} DCBits;
static dc_u32 finite(float x){DCBits b={.f=x};return (b.u&0x7fffffffu)<0x7f800000u;}
static float absf(float x){return x<0?-x:x;}
static float clamp(float x,float lo,float hi){return x<lo?lo:x>hi?hi:x;}
static float maximum(float a,float b){return a>b?a:b;}
static dc_u32 mix(dc_u32 seed,dc_u32 ordinal,dc_u32 salt){
    dc_u32 x=seed^((ordinal+1)*0x9e3779b9u)^(salt*0x85ebca6bu);
    x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;return x^(x>>16);
}
static void clear(DirectorCoreState *s){
    volatile unsigned char *p=(volatile unsigned char *)s;
    for(dc_u32 j=0;j<sizeof(*s);j++)p[j]=0;
}
DirectorCoreConfig lab_director_core_default(void){
    DirectorCoreConfig c={DC_ABI,1,120,0,1.f,1.f,240.f,54.f,.2f,.1f};return c;
}
static dc_u32 config_valid(const DirectorCoreConfig *c){
    return c&&c->abi==DC_ABI&&c->enabled<=1&&!c->reserved&&
        c->max_gap_ticks&&c->max_gap_ticks<=3600&&
        finite(c->catch_authority)&&c->catch_authority>=0&&c->catch_authority<=1&&
        finite(c->formation_authority)&&c->formation_authority>=0&&c->formation_authority<=1&&
        finite(c->spacing)&&c->spacing>0&&c->spacing<=100000&&
        finite(c->response_ticks)&&c->response_ticks>0&&c->response_ticks<=36000&&
        finite(c->phase_variation)&&c->phase_variation>=0&&c->phase_variation<=.5f&&
        finite(c->depth_amplitude)&&c->depth_amplitude>=0&&c->depth_amplitude<1;
}
static float ramp(float x,float lo,float hi){return clamp((x-lo)/(hi-lo),0,1);}
static float directional(float magnitude,float lo,float hi){
    float neutral=ramp(0,lo,hi);
    if(neutral>=1)return clamp(magnitude/(hi-lo),0,1);
    return clamp((ramp(magnitude,lo,hi)-neutral)/(1-neutral),0,1);
}
static float human_catch(const DirectorCoreInput *i,float error){
    return i->cu_ratio*ramp(error,i->cu_min,i->cu_max)+
           i->cd_ratio*ramp(-error,i->cd_min,i->cd_max);
}
static float peer_catch(const DirectorCoreInput *i,float error){
    return error>=0?i->cu_ratio*directional(error,i->cu_min,i->cu_max):
                    i->cd_ratio*directional(-error,i->cd_min,i->cd_max);
}
/* Fixed 1500 racing-update cycle; duration jitter and phase permutation depend
 * only on explicit seed/cycle. Bounded work even when attaching late in a race.
 * Unlike the Python exploration, field events do not choose the next phase. */
static dc_u32 timed_phase(const DirectorCoreConfig *c,const DirectorCoreInput *i,dc_u32 *key){
    static const dc_u32 permutations[6][3]={{1,2,3},{1,3,2},{2,1,3},{2,3,1},{3,1,2},{3,2,1}};
    dc_u32 cycle=i->tick/1500u,t=i->tick%1500u;
    float a=(float)(mix(i->seed,cycle,2)&65535u)/65535.f*2-1;
    float b=(float)(mix(i->seed,cycle,3)&65535u)/65535.f*2-1;
    /* +/-120 at maximum variation keeps the third dwell >=180 updates. */
    dc_u32 d0=(dc_u32)(480.f+240.f*c->phase_variation*a);
    dc_u32 d1=(dc_u32)(600.f+240.f*c->phase_variation*b);
    dc_u32 region=t<d0?0:t<d0+d1?1:2;
    dc_u32 p=permutations[mix(i->seed,cycle,1)%6][region];
    *key=cycle*4u+region;return p;
}
/* Private descriptor: kind1 lap / kind2 section; count at8; reference+1 at16.
 * Input reserved[1] is float32 ordinal progress; state reserved remembers the
 * descriptor. Legacy0 remains the timer policy. Native limits and coefficients
 * do not change. A paused clock or reversing cannot rewind phase/cohort keys. */
static dc_u32 dp_descriptor(dc_u32 d){
    if(!d)return 1;
    dc_u32 kind=d&255u,count=(d>>8)&255u,reference=(d>>16)&255u;
    return !(d>>24)&&(kind==1||kind==2)&&count&&count<=127&&reference&&reference<=12;
}
static dc_u32 dp_input_valid(const DirectorCoreInput *i){
    if(!dp_descriptor(i->reserved[0]))return 0;
    if(!i->reserved[0])return i->reserved[1]==0;
    DCBits units={.u=i->reserved[1]};
    return finite(units.f)&&units.f>=0&&units.f<(float)((i->reserved[0]>>8)&255u);
}
static dc_u32 dp_state_valid(const DirectorCoreState *s){
    if(!dp_descriptor(s->reserved))return 0;
    return !s->reserved||(s->phase_key%4u<3&&s->phase_key/4u<((s->reserved>>8)&255u));
}
static dc_u32 phase(const DirectorCoreConfig *c,const DirectorCoreInput *i,
    const DirectorCoreState *s,dc_u32 reset,dc_u32 *key){
    if(!i->reserved[0])return timed_phase(c,i,key);
    static const dc_u32 permutations[6][3]={{1,2,3},{1,3,2},{2,1,3},{2,3,1},{3,1,2},{3,2,1}};
    DCBits units={.u=i->reserved[1]};dc_u32 cycle=(dc_u32)units.f;
    float t=units.f-(float)cycle;
    float a=(float)(mix(i->seed,cycle,2)&65535u)/65535.f*2-1;
    float b=(float)(mix(i->seed,cycle,3)&65535u)/65535.f*2-1;
    float d0=.32f+.16f*c->phase_variation*a,d1=.40f+.16f*c->phase_variation*b;
    dc_u32 region=t<d0?0:t<d0+d1?1:2;
    *key=cycle*4u+region;
    if(!reset&&(i->tick==s->last_tick||*key<s->phase_key))*key=s->phase_key;
    return permutations[mix(i->seed,*key/4u,1)%6][*key%4u];
}

static dc_u32 input_valid(const DirectorCoreInput *i,dc_u32 *index,dc_u64 *signature){
    if(!i||!i->epoch||!i->owner||!i->tune_revision||!i->count||i->count>DC_CAPACITY||
       i->racer>=DC_CAPACITY||i->flags&~7u||i->human_valid>1||!dp_input_valid(i))return 0;
    const float values[]={i->native_sum,i->native_base,i->native_catch,i->native_formation,
        i->native_rung,i->up,i->down,i->cu_min,i->cu_max,i->cu_ratio,i->cd_min,i->cd_max,
        i->cd_ratio,i->promotion_strength};
    for(dc_u32 k=0;k<sizeof(values)/sizeof(values[0]);k++)if(!finite(values[k]))return 0;
    if(i->native_rung<=0||i->cu_min>=i->cu_max||i->cd_min>=i->cd_max||
       !finite(i->cu_max-i->cu_min)||!finite(i->cd_max-i->cd_min)||
       i->promotion_strength<0||i->promotion_strength>1)return 0;
    float expected=(i->native_base+i->native_formation)+i->native_catch;
    if(!finite(expected)||absf(expected-i->native_sum)>1.e-5f*(1+absf(i->native_sum)))return 0;
    if(i->human_valid&&(!finite(i->human_progress)||!finite(i->human_weight)||
       i->human_weight<0||i->human_weight>1))return 0;
    for(dc_u32 k=0;k<DC_CAPACITY;k++)if(!finite(i->targets[k]))return 0;
    dc_u32 own=DC_CAPACITY,ids=0;dc_u64 sig=0;
    for(dc_u32 k=0;k<i->count;k++){
        const DirectorCorePeer *p=&i->peers[k];
        if(!p->owner||p->racer>=DC_CAPACITY||p->slot>=DC_CAPACITY||p->reserved||
           !finite(p->progress)||(ids&(1u<<p->racer)))return 0;
        ids|=1u<<p->racer;
        for(dc_u32 j=0;j<k;j++)if(i->peers[j].owner==p->owner)return 0;
        if(p->racer==i->racer){if(p->owner!=i->owner)return 0;own=k;}
        sig^=(p->owner*(0x9e3779b97f4a7c15ull+2ull*p->racer))^(1ull<<p->racer);
    }
    if(own==DC_CAPACITY)return 0;
    *index=own;*signature=sig;return 1;
}
static dc_u32 state_valid(const DirectorCoreState *s){
    return s->initialized==1&&dp_state_valid(s)&&s->epoch&&s->owner&&s->tune_revision&&
        finite(s->catch_term)&&finite(s->formation_term)&&finite(s->band_low)&&
        finite(s->band_high)&&s->band_low<=s->band_high&&s->cohort>=DC_LEAD&&s->cohort<=DC_FIELD;
}
static dc_u32 do_core_step(const DirectorCoreConfig *c,const DirectorCoreInput *i,
    DirectorCoreState *s,DirectorCoreOutput *out,
    const DirectorOpeningContext *opening,DirectorOpeningOutput *opening_output){
    if(!out)return DC_INVALID;
    DirectorCoreOutput o={0};o.status=DC_INVALID;
    if(i){o.sum=i->native_sum;o.catch_term=i->native_catch;o.formation_term=i->native_formation;}
    if(!i||!s||!config_valid(c)){*out=o;return o.status;}
    if(!c->enabled||(c->catch_authority==0&&c->formation_authority==0)){
        clear(s);o.status=DC_PASSTHROUGH;o.reset_reason=DC_RESET_DISABLED;*out=o;return o.status;
    }
    dc_u32 own;dc_u64 signature;
    if(!input_valid(i,&own,&signature)){*out=o;return o.status;}
    if((i->flags&DC_UNAVAILABLE)||((i->flags&DC_OFFROAD)&&!(i->flags&DC_BOOST))){
        clear(s);o.status=DC_PASSTHROUGH;o.reset_reason=DC_RESET_DISABLED;*out=o;return o.status;
    }
    dc_u32 order[DC_CAPACITY];
    for(dc_u32 k=0;k<i->count;k++){
        dc_u32 j=k;order[k]=k;
        while(j){
            const DirectorCorePeer *a=&i->peers[order[j-1]],*b=&i->peers[k];
            if(a->progress>b->progress||(a->progress==b->progress&&a->racer<b->racer))break;
            order[j]=order[j-1];j--;
        }
        order[j]=k;
    }
    dc_u32 position=0;for(dc_u32 k=0;k<i->count;k++)if(order[k]==own)position=k;
    float gap=i->peers[order[0]].progress-i->peers[own].progress;
    if(!finite(gap)){*out=o;return o.status;}
    dc_u32 reset=DC_RESET_NONE;
    if(!state_valid(s))reset=DC_RESET_FIRST;
    else if(s->epoch!=i->epoch||s->owner!=i->owner||s->seed!=i->seed)reset=DC_RESET_OWNER;
    else if(s->tune_revision!=i->tune_revision)reset=DC_RESET_TUNE;
    else if(s->roster_signature!=signature)reset=DC_RESET_ROSTER;
    else if(s->reserved!=i->reserved[0])reset=DC_RESET_CLOCK;
    else if(i->tick<s->last_tick||i->tick-s->last_tick>c->max_gap_ticks)reset=DC_RESET_CLOCK;
    DirectorCoreState next=*s;
    dc_u32 key,p=phase(c,i,s,reset,&key);
    float dt=reset?0:(float)(i->tick-s->last_tick),alpha=dt/(c->response_ticks+dt);
    if(reset||next.phase_key!=key){
        dc_u32 candidates=i->count>3?i->count-3:0;
        dc_u32 rotation=candidates?mix(i->seed,key,5)%candidates:0;
        next.cohort=position<3?DC_LEAD:DC_FIELD;
        if(position>=3&&candidates&&((position-3+candidates-rotation)%candidates)<2)next.cohort=DC_ATTACK;
    }
    float depth=0;
    for(dc_u32 k=0;k<DC_CAPACITY;k++)depth=maximum(depth,absf(i->targets[k]));
    /* Constant per-phase depth choice; band smoothing prevents a phase jump. */
    float wave=(float)(mix(i->seed,key,7)&65535u)/65535.f*2-1;
    depth*=1+c->depth_amplitude*wave;
    float lead_end=maximum(2*c->spacing,.12f*depth),chase_width=maximum(3*c->spacing,.18f*depth);
    float chase_start=maximum(lead_end+c->spacing,(p==DC_BATTLE?.45f:.22f)*depth);
    float point=i->targets[i->peers[own].slot],lo=point-.5f*c->spacing,hi=point+.5f*c->spacing;
    if(p!=DC_SPREAD){
        /* A new on-track leader must not be dragged toward its old loser-slot
         * target until the next scheduled cohort refresh. Escape feedback
         * still comes from the following CPU and authored CD. */
        if(position==0||next.cohort==DC_LEAD){lo=0;hi=lead_end;}
        else if(next.cohort==DC_ATTACK){lo=chase_start;hi=chase_start+chase_width;}
        else if(position){
            float ahead=i->peers[order[0]].progress-i->peers[order[position-1]].progress;
            lo=ahead+.5f*c->spacing;hi=ahead+1.5f*c->spacing;
        }
    }
    if(i->promotion_strength>0){
        lo*=1-i->promotion_strength;
        hi+=(lead_end-hi)*i->promotion_strength;
    }
    if(reset){next.band_low=lo;next.band_high=hi;next.catch_term=i->native_catch;next.formation_term=i->native_formation;}
    else {next.band_low+=alpha*(lo-next.band_low);next.band_high+=alpha*(hi-next.band_high);}
    float target=position==0&&(p!=DC_SPREAD||i->promotion_strength>0)?0:clamp(gap,next.band_low,next.band_high);
    float fraction=clamp((gap-target)/i->native_rung,-1,1);
    float policy_f=fraction*(fraction>0?i->up:i->down);
    dc_u32 earned=i->promotion_strength>0,active=p!=DC_SPREAD||earned;
    dc_u32 recipient=active&&(position==0||next.cohort==DC_LEAD||next.cohort==DC_ATTACK||earned);
    float hc=0,lc=0,nc=0;
    if(recipient){
        if(i->human_valid)hc=.25f*i->human_weight*human_catch(i,i->human_progress-i->peers[own].progress);
        lc=.5f*peer_catch(i,gap-target);
    }
    if(active&&i->count>1){
        float error=position?i->peers[order[position-1]].progress-i->peers[own].progress-c->spacing:
            -(i->peers[own].progress-i->peers[order[1]].progress-c->spacing);
        nc=.25f*peer_catch(i,error);
    }
    float dc=active?i->native_catch+c->catch_authority*(hc+lc+nc-i->native_catch):i->native_catch;
    float df=active?i->native_formation+c->formation_authority*(policy_f-i->native_formation):i->native_formation;
    if(opening&&opening_output)df=do_formation(c,i,own,gap,target,df,active,opening,opening_output);
    if(!(i->flags&DC_BOOST)){
        next.catch_term+=alpha*(dc-next.catch_term);
        next.formation_term+=alpha*(df-next.formation_term);
    }
    if(c->catch_authority==0)next.catch_term=i->native_catch;
    if(c->formation_authority==0)next.formation_term=i->native_formation;
    o.sum=(i->native_base+next.formation_term)+next.catch_term;
    if((next.catch_term==i->native_catch&&next.formation_term==i->native_formation)||reset)o.sum=i->native_sum;
    const float results[]={o.sum,next.catch_term,next.formation_term,next.band_low,next.band_high,gap,target,hc,lc,nc};
    for(dc_u32 k=0;k<sizeof(results)/sizeof(results[0]);k++)if(!finite(results[k])){
        o.status=DC_INVALID;o.sum=i->native_sum;o.catch_term=i->native_catch;o.formation_term=i->native_formation;
        *out=o;return DC_INVALID;
    }
    next.epoch=i->epoch;next.owner=i->owner;next.tune_revision=i->tune_revision;next.roster_signature=signature;
    next.initialized=1;next.last_tick=i->tick;next.seed=i->seed;next.phase_key=key;next.reserved=i->reserved[0];
    o.status=reset?DC_RESET:(i->flags&DC_BOOST)?DC_BOOST_HELD:DC_APPLIED;
    o.phase=p;o.cohort=earned?DC_EARNED:p==DC_SPREAD?DC_RECOVERY:position==0?DC_LEAD:next.cohort;o.reset_reason=reset;
    o.catch_term=next.catch_term;o.formation_term=next.formation_term;
    o.band_low=next.band_low;o.band_high=next.band_high;o.gap=gap;o.target=target;o.rung=i->native_rung;
    o.human_term=hc;o.leader_term=lc;o.local_term=nc;
    *s=next;*out=o;return o.status;
}

dc_u32 lab_director_core_step(const DirectorCoreConfig *c,const DirectorCoreInput *i,
    DirectorCoreState *s,DirectorCoreOutput *out){
    return do_core_step(c,i,s,out,0,0);
}
dc_u32 lab_director_opening_step(const DirectorCoreConfig *c,const DirectorCoreInput *i,
    DirectorCoreState *s,DirectorCoreOutput *out,
    const DirectorOpeningContext *opening,DirectorOpeningOutput *opening_output){
    if(opening_output){DirectorOpeningOutput empty={0};*opening_output=empty;}
    return do_core_step(c,i,s,out,opening,opening_output);
}
