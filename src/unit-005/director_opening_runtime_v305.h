#ifndef HARDERAI_DIRECTOR_OPENING_RUNTIME_V305_H
#define HARDERAI_DIRECTOR_OPENING_RUNTIME_V305_H
#include "director_opening_core_v305.h"
typedef struct {dc_u64 sequence;DirectorOpeningConfig policy;} DirectorOpeningControl;
typedef struct {
    dc_u64 sequence,updates,epoch,owner,controller,clock,request_sequence,director_sequence;
    dc_u32 tick,reserved;
    DirectorOpeningContext context;
    DirectorOpeningOutput output;
} DirectorOpeningRecord;
_Static_assert(sizeof(DirectorOpeningControl)==40,"Opening control ABI1");
_Static_assert(sizeof(DirectorOpeningRecord)==160,"Opening record ABI1");
DR_EXPORT const dc_u32 lab_director_opening_config[8]={1,40,160,12,1200,600,2,0};
DR_EXPORT const DirectorOpeningConfig lab_director_opening_defaults={1,0,1200,600,2,{0,0,0}};
DR_EXPORT DirectorOpeningControl lab_director_opening_control;
DR_EXPORT DirectorOpeningRecord lab_director_opening_records[12];

static dc_u32 do_request(DirectorOpeningContext *out,dc_u64 *sequence){
    const volatile DirectorOpeningControl *r=&lab_director_opening_control;
    dc_u64 seq=__atomic_load_n(&r->sequence,__ATOMIC_ACQUIRE);
    if(seq&1)return 0;
    out->config=seq?r->policy:lab_director_opening_defaults;
    __atomic_thread_fence(__ATOMIC_ACQUIRE);
    if(seq!=__atomic_load_n(&r->sequence,__ATOMIC_ACQUIRE))return 0;
    *sequence=seq;return 1;
}
static dc_u32 do_requested(const DirectorOpeningContext *p){
    return do_config_valid(&p->config)&&p->config.enabled;
}
static dc_u32 do_request_same(dc_u64 sequence){
    return sequence==__atomic_load_n(&lab_director_opening_control.sequence,__ATOMIC_ACQUIRE);
}
/* Existing per-actor DirectorRuntimeRecord.lock serializes this writer. The
 * separate sequence and matching director_sequence bind both observations. */
static void do_publish(const DirectorCoreInput *i,const DirectorOpeningContext *context,
    const DirectorOpeningOutput *output,dc_u64 controller,dc_u64 clock,
    dc_u64 request,dc_u64 director_sequence){
    if(i->racer>=12)return;
    DirectorOpeningRecord *r=&lab_director_opening_records[i->racer];
    if((r->sequence&1)||r->sequence>~0ull-2||r->updates==~0ull)return;
    __atomic_add_fetch(&r->sequence,1,__ATOMIC_ACQ_REL);
    r->updates++;r->epoch=i->epoch;r->owner=i->owner;r->controller=controller;r->clock=clock;
    r->request_sequence=request;r->director_sequence=director_sequence;r->tick=i->tick;r->reserved=0;
    dr_copy(&r->context,context,sizeof(*context));dr_copy(&r->output,output,sizeof(*output));
    __atomic_add_fetch(&r->sequence,1,__ATOMIC_RELEASE);
}
#endif
