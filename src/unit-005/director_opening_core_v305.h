#ifndef HARDERAI_DIRECTOR_OPENING_CORE_V305_H
#define HARDERAI_DIRECTOR_OPENING_CORE_V305_H
/* Private optional opening target policy. These are supplied values only;
 * there are no native addresses, allocation, native slot or pace writes. */
typedef struct {
    dc_u32 abi,enabled,hold_ticks,fade_ticks,slot_cap,reserved[3];
} DirectorOpeningConfig;
typedef struct {
    DirectorOpeningConfig config;
    float capped_rung;
    dc_u32 reserved;
} DirectorOpeningContext;
typedef struct {
    dc_u32 status,native_slot,opening_slot,evaluated;
    float weight,normal_target,opening_target,normal_rung,opening_rung;
    float normal_wanted,opening_wanted,blended_wanted;
} DirectorOpeningOutput;
_Static_assert(sizeof(DirectorOpeningConfig)==32,"Opening policy ABI1");
_Static_assert(sizeof(DirectorOpeningContext)==40,"Opening context ABI1");
_Static_assert(sizeof(DirectorOpeningOutput)==48,"Opening output ABI1");
enum { DO_OFF=0,DO_HOLD=1,DO_FADE=2,DO_FINISHED=3,DO_ALREADY_CLOSE=4,DO_INVALID=5 };

static inline dc_u32 do_finite(float value){
    union {float f;dc_u32 u;}v={.f=value};return (v.u&0x7fffffffu)<0x7f800000u;
}
static inline float do_clamp(float x,float lo,float hi){return x<lo?lo:x>hi?hi:x;}
static inline dc_u32 do_config_valid(const DirectorOpeningConfig *c){
    return c&&c->abi==1&&c->enabled<=1&&c->hold_ticks<=36000&&
        c->fade_ticks&&c->fade_ticks<=36000&&c->hold_ticks+c->fade_ticks<=36000&&
        c->slot_cap<DC_CAPACITY&&!c->reserved[0]&&!c->reserved[1]&&!c->reserved[2];
}
/* Native state6 age, not time since attachment. No wall-clock or RNG source.
 * Quintic fade has zero first and second derivative at its endpoints. */
static inline float do_weight(const DirectorOpeningConfig *c,dc_u32 tick){
    if(tick<=c->hold_ticks)return 1.f;
    if(tick>=c->hold_ticks+c->fade_ticks)return 0.f;
    float x=(float)(tick-c->hold_ticks)/(float)c->fade_ticks;
    float smooth=x*x*x*(x*(x*6.f-15.f)+10.f);
    return do_clamp(1.f-smooth,0.f,1.f);
}
static inline float do_formation(const DirectorCoreConfig *c,const DirectorCoreInput *i,
    dc_u32 own,float gap,float target,float normal_wanted,dc_u32 active,
    const DirectorOpeningContext *context,DirectorOpeningOutput *out){
    DirectorOpeningOutput o={0};o.evaluated=1;o.normal_wanted=normal_wanted;
    o.opening_wanted=normal_wanted;o.blended_wanted=normal_wanted;
    o.native_slot=i->peers[own].slot;o.opening_slot=o.native_slot;
    o.normal_rung=i->native_rung;o.opening_rung=i->native_rung;
    o.normal_target=active?target:i->targets[o.native_slot];
    o.opening_target=o.normal_target;
    const DirectorOpeningConfig *p=&context->config;
    if(!do_config_valid(p)||context->reserved){o.status=DO_INVALID;goto done;}
    if(!p->enabled){o.status=DO_OFF;goto done;}
    o.weight=do_weight(p,i->tick);
    if(o.weight==0){o.status=DO_FINISHED;goto done;}
    o.opening_slot=o.native_slot<p->slot_cap?o.native_slot:p->slot_cap;
    float cap=i->targets[o.opening_slot];
    /* Preserve the existing best tiers and a closer director/Golden target.
     * Promotion only changes a lower native tier's looser target. */
    if(o.native_slot<=p->slot_cap||cap>=o.normal_target){o.status=DO_ALREADY_CLOSE;goto done;}
    if(!do_finite(context->capped_rung)||context->capped_rung<=0){o.status=DO_INVALID;goto done;}
    o.opening_target=cap;o.opening_rung=context->capped_rung;
    float fraction=do_clamp((gap-cap)/context->capped_rung,-1.f,1.f);
    float policy=fraction*(fraction>0?i->up:i->down);
    o.opening_wanted=i->native_formation+c->formation_authority*(policy-i->native_formation);
    o.blended_wanted=normal_wanted+o.weight*(o.opening_wanted-normal_wanted);
    if(!do_finite(o.opening_wanted)||!do_finite(o.blended_wanted)){
        o.status=DO_INVALID;o.blended_wanted=normal_wanted;goto done;
    }
    o.status=i->tick<=p->hold_ticks?DO_HOLD:DO_FADE;
done:
    *out=o;return o.blended_wanted;
}
dc_u32 lab_director_opening_step(const DirectorCoreConfig *,const DirectorCoreInput *,
    DirectorCoreState *,DirectorCoreOutput *,const DirectorOpeningContext *,DirectorOpeningOutput *);
#endif
