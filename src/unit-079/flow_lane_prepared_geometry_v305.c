#include "flow_lane_prepared_geometry_v305.h"
_Static_assert(sizeof(FlowCoursePin)%8==0&&sizeof(FlowLaneMesh)%8==0,"Prepared key alignment");
static unsigned same(const void *a,const void *b,unsigned size){
    const flm_u64 *x=a,*y=b;
    for(unsigned k=0;k<size/8;k++)if(x[k]!=y[k])return 0;return 1;
}
static unsigned key(const FlowCoursePin *p,const FlowLaneMesh *m){
    return p&&m&&p->generation&&p->sequence>=4&&!(p->sequence&1)&&
        p->owner&&p->wrapper&&p->course_data==m->course_data&&p->header==m->header;
}
static unsigned references(const FlowPreparedGeometry *p,const unsigned *pairs,unsigned count){
    if(p->ref_count!=count)return 0;
    for(unsigned k=0;k<2*count;k++)if(p->pairs[k]!=pairs[k])return 0;return 1;
}
#include "flow_lane_obstacle_groups_impl_v305.h"
unsigned lab_flow_prepared_begin(const FlowCoursePin *pin,const FlowLaneMesh *mesh,
    const unsigned *pairs,unsigned count,FlowPreparedGeometry *p,FlowLaneDecodeCache *c){
    if(!p||!c||p->reserved||p->padding||p->state>FLPG_CHANGED||c->geometry_ready||
       !key(pin,mesh)||!pairs||!count||count>FLDC_ENTRIES)return 0;
    if(p->state!=FLPG_EMPTY&&same(&p->pin,pin,sizeof(*pin))){
        if(p->state==FLPG_CHANGED)return 0;
        if(!references(p,pairs,count))return 0;
        if(!same(&p->mesh,mesh,sizeof(*mesh))||!lab_flow_geometry_resume(c)){
            p->state=FLPG_CHANGED;p->mismatches++;return 0;
        }
        p->reuses++;return 1;
    }
    /* Reject old authority before following any old native cache binding.
     * Changed/recycled allocations use only the newly admitted mesh. */
    p->obstacle_groups.ready=0;
    p->state=FLPG_EMPTY;p->pin=*pin;p->mesh=*mesh;p->ref_count=count;
    for(unsigned k=0;k<2*count;k++)p->pairs[k]=pairs[k];
    if(!lab_flow_geometry_open(mesh,pairs,count,c)){
        p->state=FLPG_CHANGED;p->mismatches++;return 0;
    }
    flog_prepare(p,c);
    p->preparations++;p->state=FLPG_READY;return 1;
}
unsigned lab_flow_prepared_finish(FlowPreparedGeometry *p,FlowLaneDecodeCache *c){
    if(!c)return 0;
    if(!p||p->state!=FLPG_READY){c->geometry_ready=0;return 0;}
    if(!lab_flow_geometry_close(c)){p->state=FLPG_CHANGED;p->mismatches++;return 0;}
    return 1;
}
unsigned lab_flow_prepared_publish(const FlowCoursePin *pin,const FlowLaneMesh *mesh,
    const unsigned *pairs,unsigned count,unsigned *lock,FlowPreparedGeometry *p,FlowLaneDecodeCache *c){
    if(!lock)return 0;unsigned zero=0;
    if(!__atomic_compare_exchange_n(lock,&zero,1,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED))return 0;
    unsigned ready=lab_flow_prepared_begin(pin,mesh,pairs,count,p,c);
    if(ready)ready=lab_flow_prepared_finish(p,c);
    __atomic_store_n(lock,0,__ATOMIC_RELEASE);return ready;
}
