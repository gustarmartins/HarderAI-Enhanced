#include "director_pressure_pace_v305.h"
typedef union {float f;pp_u32 u;} PPBits;
static pp_u32 finite(float x){PPBits b={.f=x};return (b.u&0x7fffffffu)<0x7f800000u;}
static float clamp(float x,float lo,float hi){return x<lo?lo:x>hi?hi:x;}
static float minf(float a,float b){return a<b?a:b;}
static float maxf(float a,float b){return a>b?a:b;}
static void zero(void *p,pp_u32 size){for(pp_u32 k=0;k<size;k++)((unsigned char *)p)[k]=0;}
PressurePaceConfig lab_pressure_pace_default(void){
    PressurePaceConfig c={1,6,30,720,120.f,.5f,.94f,1.06f,.25f,4.f,.01f,{0,0}};return c;
}
static pp_u32 config_valid(const PressurePaceConfig *c){
    return c&&c->abi==1&&c->max_gap_ticks&&c->max_gap_ticks<=120&&c->warmup_ticks&&c->warmup_ticks<=3600&&
        c->max_age_ticks&&c->max_age_ticks<=3600&&finite(c->response_ticks)&&c->response_ticks>0&&
        finite(c->min_speed)&&c->min_speed>0&&finite(c->min_cap_fraction)&&c->min_cap_fraction>0&&
        finite(c->max_cap_fraction)&&c->max_cap_fraction>=c->min_cap_fraction&&
        finite(c->min_progress_fraction)&&c->min_progress_fraction>0&&
        finite(c->max_progress_fraction)&&c->max_progress_fraction>=c->min_progress_fraction&&
        finite(c->max_ratio_step)&&c->max_ratio_step>=0&&c->max_ratio_step<=.1f&&
        !(c->reserved[0]&~PP_DRIFT)&&!c->reserved[1];
}
static pp_u32 sample_valid(const PressurePaceSample *i){
    return i&&i->owner&&i->racer<PP_CAPACITY&&!(i->flags&~127u)&&finite(i->progress)&&
        finite(i->speed)&&i->speed>=0&&finite(i->base_cap)&&i->base_cap>0&&
        finite(i->normal_cap)&&i->normal_cap>0&&finite(i->normal_cap/i->base_cap)&&i->normal_cap/i->base_cap>0;
}
pp_u32 lab_pressure_pace_sample(const PressurePaceConfig *c,pp_u64 epoch,pp_u32 tick,
    const PressurePaceSample *i,PressurePaceState *s,PressurePaceOutput *out){
    if(!out)return PP_INVALID;PressurePaceOutput o={0};
    if(!epoch||!s||!config_valid(c)||!sample_valid(i)){*out=o;return PP_INVALID;}
    float ratio=i->normal_cap/i->base_cap;
    pp_u32 clean=!(i->flags&~c->reserved[0])&&i->speed>=c->min_speed&&i->speed>=c->min_cap_fraction*i->normal_cap&&
        i->speed<=c->max_cap_fraction*i->normal_cap;
    pp_u32 reset=s->epoch!=epoch||s->owner!=i->owner||s->reserved||s->ready>1||s->previous_clean>1||
        !finite(s->progress)||!finite(s->speed)||!finite(s->ratio)||s->ratio<=0||
        !finite(s->baseline_rate)||s->baseline_rate<0||!finite(s->clean_ratio)||s->clean_ratio<=0||s->clean_ticks>c->warmup_ticks||
        s->last_clean_tick>s->last_tick||tick<s->last_tick||tick-s->last_tick>c->max_gap_ticks;
    if(!reset&&tick==s->last_tick){
        o.status=PP_DUPLICATE;o.ready=s->ready&&tick-s->last_clean_tick<=c->max_age_ticks;
        o.clean=clean;o.clean_ticks=s->clean_ticks;o.baseline_rate=s->baseline_rate;o.ratio=o.ready?s->clean_ratio:ratio;
        o.age_ticks=(float)(tick-s->last_clean_tick);*out=o;return o.status;
    }
    PressurePaceState next=*s;
    if(reset){zero(&next,sizeof(next));next.epoch=epoch;next.owner=i->owner;next.clean_ratio=ratio;o.status=PP_RESET;}
    else{
        pp_u32 dt=tick-s->last_tick;
        float rate=(i->progress-s->progress)/(float)dt;
        /* Require clean endpoints and a slowly changing effective cap ratio.
         * Use its endpoint mean; a large step starts a new interval. Exact
         * equality would never warm up under continuously adjusted CU/F.CORR.
         * Progress-rate bounds reject reversals/warps and implausible transport. */
        float difference=maxf(ratio,s->ratio)-minf(ratio,s->ratio);
        pp_u32 interval=clean&&s->previous_clean&&difference<=c->max_ratio_step*minf(ratio,s->ratio)&&rate>0&&finite(rate)&&
            rate>=minf(i->speed,s->speed)*c->min_progress_fraction&&
            rate<=maxf(i->speed,s->speed)*c->max_progress_fraction;
        float baseline=rate/(.5f*s->ratio+.5f*ratio);
        if(interval&&finite(baseline)&&baseline>0){
            float alpha=(float)dt/(c->response_ticks+(float)dt);
            next.baseline_rate=next.clean_ticks?next.baseline_rate+alpha*(baseline-next.baseline_rate):baseline;
            next.clean_ticks=dt>=c->warmup_ticks-next.clean_ticks?c->warmup_ticks:next.clean_ticks+dt;
            next.last_clean_tick=tick;next.ready=next.clean_ticks>=c->warmup_ticks;
            next.clean_ratio=ratio;
            o.progress_rate=rate;o.status=next.ready?PP_READY:PP_WARMUP;
        }else{o.status=PP_UNCLEAN;}
        if(tick-next.last_clean_tick>c->max_age_ticks){next.ready=0;next.clean_ticks=0;next.baseline_rate=0;}
    }
    next.last_tick=tick;next.progress=i->progress;next.speed=i->speed;next.ratio=ratio;next.previous_clean=clean;
    o.clean=clean;o.ready=next.ready;o.clean_ticks=next.clean_ticks;o.baseline_rate=next.baseline_rate;
    o.ratio=next.ready?next.clean_ratio:ratio;o.age_ticks=(float)(tick-next.last_clean_tick);*s=next;*out=o;return o.status;
}
static pp_u32 selection_config(const PressureSelectionConfig *c){
    return c&&c->abi==1&&c->attackers&&c->attackers<=3&&c->rotation_ticks&&c->rotation_ticks<=36000&&!(c->eligible_mask&~0xfffu)&&
        finite(c->near_seconds)&&c->near_seconds>=0&&finite(c->far_seconds)&&c->far_seconds>c->near_seconds&&
        finite(c->recovery_seconds)&&c->recovery_seconds>0&&finite(c->ticks_per_second)&&c->ticks_per_second>0&&
        finite(c->lead_allowance_seconds)&&c->lead_allowance_seconds>=c->near_seconds&&
        finite(c->field_recovery_seconds)&&c->field_recovery_seconds>=c->recovery_seconds;
}
static pp_u32 ready(const PressurePeer *p){
    /* A recent clean estimate survives drift/boost states. The sampler never
     * learns those speeds and exposes the last clean cap ratio, so an item
     * does not become an instruction to grant matching ordinary pace. */
    return sample_valid(&p->sample)&&p->pace.ready==1&&finite(p->pace.baseline_rate)&&
        p->pace.baseline_rate>0&&finite(p->pace.ratio)&&p->pace.ratio>0&&
        !(p->sample.flags&~(PP_BOOST|PP_DRIFT))&&p->sample.speed>0;
}
static pp_u32 mix(pp_u32 seed,pp_u32 cycle,pp_u32 racer){
    pp_u32 x=seed^((cycle+1)*0x9e3779b9u)^((racer+1)*0x85ebca6bu);
    x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;return x^(x>>16);
}
pp_u32 lab_pressure_select(const PressureSelectionConfig *c,const PressurePeer *peers,pp_u32 count,
    pp_u32 reference,pp_u32 tick,pp_u32 seed,PressureSelection *out){
    if(!out)return 0;PressureSelection o={0};
    if(!selection_config(c)||!peers||!count||count>PP_CAPACITY||reference>=count||!ready(&peers[reference])){*out=o;return 0;}
    o.reference_rate=peers[reference].pace.baseline_rate*peers[reference].pace.ratio;
    if(!finite(o.reference_rate)||o.reference_rate<=0){*out=o;return 0;}
    o.cycle=tick/c->rotation_ticks;pp_u32 ids=0,candidates[PP_CAPACITY],scores[PP_CAPACITY],n=0;
    for(pp_u32 k=0;k<count;k++){
        const PressurePeer *p=&peers[k];
        if(!sample_valid(&p->sample)){zero(out,sizeof(*out));return 0;}
        pp_u32 bit=1u<<p->sample.racer;
        if(ids&bit){zero(out,sizeof(*out));return 0;}ids|=bit;
        for(pp_u32 j=0;j<k;j++)if(peers[j].sample.owner==p->sample.owner){zero(out,sizeof(*out));return 0;}
        if(k==reference||!(c->eligible_mask&bit)||!ready(p))continue;
        float gap=(peers[reference].sample.progress-p->sample.progress)/(o.reference_rate*c->ticks_per_second);
        if(!finite(gap)||gap<=0||gap>=c->far_seconds)continue;
        o.eligible_mask|=bit;pp_u32 score=mix(seed,o.cycle,p->sample.racer),at=n;
        while(at&&(score>scores[at-1]||(score==scores[at-1]&&p->sample.racer<candidates[at-1]))){
            scores[at]=scores[at-1];candidates[at]=candidates[at-1];at--;
        }
        scores[at]=score;candidates[at]=p->sample.racer;n++;
    }
    for(pp_u32 k=0;k<n&&k<c->attackers;k++)o.selected_mask|=1u<<candidates[k];
    o.valid=1;*out=o;return 1;
}
static float ramp(float x,float lo,float hi){return clamp((x-lo)/(hi-lo),0,1);}
pp_u32 lab_pressure_request(const PressureSelectionConfig *c,const PressureBudget *b,
    const PressurePeer *self,const PressurePeer *reference,pp_u32 selected,PressureRequest *out){
    if(!out)return 0;PressureRequest o={0};
    if(!b||!selection_config(c)||selected>1||!ready(self)||!ready(reference)){*out=o;return 0;}
    const float *values=(const float *)b;
    for(pp_u32 k=0;k<sizeof(*b)/sizeof(float);k++)if(!finite(values[k])){*out=o;return 0;}
    if(b->native_base<=0||b->up<0||b->down<0||b->cu_ratio<0||b->cd_ratio>0||b->cu_min>=b->cu_max||
       b->cd_min>=b->cd_max||b->catch_authority<0||b->catch_authority>1||b->formation_authority<0||b->formation_authority>1){*out=o;return 0;}
    float rate=reference->pace.baseline_rate*reference->pace.ratio;
    float gap=reference->sample.progress-self->sample.progress;
    o.gap_seconds=gap/(rate*c->ticks_per_second);
    selected=selected&&gap>0&&o.gap_seconds<c->far_seconds;
    o.selected=selected;
    o.action=selected?PP_ATTACK_PACE:o.gap_seconds< -c->lead_allowance_seconds?PP_CONTAIN_LEAD:
        o.gap_seconds>=c->far_seconds?PP_FIELD_RECOVERY:PP_NATIVE_PACE;
    float desired_gap=c->near_seconds*rate*c->ticks_per_second;
    if(o.action==PP_ATTACK_PACE)o.closing_rate=maxf(0,gap-desired_gap)/(c->recovery_seconds*c->ticks_per_second);
    else if(o.action==PP_CONTAIN_LEAD)o.closing_rate=minf(0,gap+c->lead_allowance_seconds*rate*c->ticks_per_second)/(c->recovery_seconds*c->ticks_per_second);
    else if(o.action==PP_FIELD_RECOVERY)o.closing_rate=maxf(0,gap-c->far_seconds*rate*c->ticks_per_second)/(c->field_recovery_seconds*c->ticks_per_second);
    o.requested_ratio=o.action?(rate+o.closing_rate)/self->pace.baseline_rate:
        (b->native_base+b->native_formation)+b->native_catch;
    if(o.action==PP_ATTACK_PACE||o.action==PP_FIELD_RECOVERY)o.requested_ratio=maxf(b->native_base,o.requested_ratio);
    float up=b->cu_ratio*ramp(gap,b->cu_min,b->cu_max),down=b->cd_ratio*ramp(-gap,b->cd_min,b->cd_max);
    float cl=b->native_catch+b->catch_authority*(down-b->native_catch);
    float ch=b->native_catch+b->catch_authority*(up-b->native_catch);
    float fl=b->native_formation+b->formation_authority*(-b->down-b->native_formation);
    float fh=b->native_formation+b->formation_authority*(b->up-b->native_formation);
    o.positive_allowance=ch+fh;o.negative_allowance=cl+fl;
    if(!o.action){o.catch_term=b->native_catch;o.formation_term=b->native_formation;o.wanted_ratio=o.requested_ratio;}
    else{
        float combined=clamp(o.requested_ratio-b->native_base,cl+fl,ch+fh);
        o.catch_term=clamp(combined-b->native_formation,cl,ch);
        o.formation_term=clamp(combined-o.catch_term,fl,fh);
        o.wanted_ratio=(b->native_base+o.formation_term)+o.catch_term;
    }
    if(!finite(o.wanted_ratio)||!finite(o.closing_rate)||!finite(o.requested_ratio)||
       !finite(o.positive_allowance)||!finite(o.negative_allowance)||!finite(o.gap_seconds)){
        zero(out,sizeof(*out));return 0;
    }
    o.valid=1;*out=o;return 1;
}

pp_u32 lab_pressure_blend(const PressureBlendConfig *c,const PressureBlendInput *i,
    PressureBlendState *s,PressureBlendOutput *out){
    if(!out)return PP_INVALID;PressureBlendOutput o={0};
    if(!c||c->abi!=1||!c->max_gap_ticks||c->max_gap_ticks>3600||c->reserved||
       !finite(c->response_ticks)||c->response_ticks<=0||!i||!s||!i->epoch||!i->owner||!i->tune_revision||
       i->enabled>1||i->reserved||(i->flags&~127u)||!finite(i->native_base)||
       !finite(i->native_catch)||!finite(i->native_formation)||i->request.valid>1||
       (i->request.valid&&(!finite(i->request.catch_term)||!finite(i->request.formation_term)||i->request.selected>1||
        i->request.action>3||i->request.reserved||!finite(i->request.wanted_ratio)||
        i->request.wanted_ratio!=(i->native_base+i->request.formation_term)+i->request.catch_term))){*out=o;return PP_INVALID;}
    o.catch_term=i->native_catch;o.formation_term=i->native_formation;
    o.ratio=(i->native_base+o.formation_term)+o.catch_term;
    if(!finite(o.ratio)){*out=(PressureBlendOutput){0};return PP_INVALID;}
    if(!i->enabled||(i->flags&~(PP_BOOST|PP_DRIFT))){zero(s,sizeof(*s));o.status=PP_UNCLEAN;*out=o;return o.status;}
    pp_u32 reset=s->initialized!=1||s->reserved||s->epoch!=i->epoch||s->owner!=i->owner||
        s->tune_revision!=i->tune_revision||s->tick>i->tick||i->tick-s->tick>c->max_gap_ticks||
        !finite(s->catch_term)||!finite(s->formation_term)||!finite(s->selected_weight)||s->selected_weight<0||s->selected_weight>1;
    PressureBlendState n=*s;
    if(reset){
        zero(&n,sizeof(n));n.initialized=1;n.epoch=i->epoch;n.owner=i->owner;n.tune_revision=i->tune_revision;
        n.tick=i->tick;n.catch_term=i->native_catch;n.formation_term=i->native_formation;o.status=PP_RESET;
    }else{
        o.dt=i->tick-s->tick;float alpha=(float)o.dt/(c->response_ticks+(float)o.dt);
        if(!(i->flags&PP_BOOST)){
            float catch_term=i->request.valid?i->request.catch_term:i->native_catch;
            float formation=i->request.valid?i->request.formation_term:i->native_formation;
            n.catch_term+=alpha*(catch_term-n.catch_term);n.formation_term+=alpha*(formation-n.formation_term);
            n.selected_weight+=alpha*((i->request.valid&&i->request.selected?1.f:0.f)-n.selected_weight);
        }
        n.tick=i->tick;o.status=!o.dt?PP_DUPLICATE:(i->flags&PP_BOOST)?PP_UNCLEAN:PP_READY;
    }
    o.catch_term=n.catch_term;o.formation_term=n.formation_term;o.selected_weight=n.selected_weight;
    o.ratio=(i->native_base+o.formation_term)+o.catch_term;
    if(!finite(o.ratio)){*out=(PressureBlendOutput){0};return PP_INVALID;}
    *s=n;*out=o;return o.status;
}
