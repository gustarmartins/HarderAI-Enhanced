#include "flow_lane_native_actuator_v305.h"
#include "flow_lane_runtime_v305.h"
#include "flow_lane_native_point_contact_v305.h"
#include "flow_lane_scope_v305.h"
#include "flow_lane_owner_contact_indices.h"
static unsigned course_equal(const FlowCoursePin *a,const FlowCoursePin *b){
    const unsigned char *x=(const unsigned char *)a,*y=(const unsigned char *)b;
    for(unsigned k=0;k<sizeof(*a);k++)if(x[k]!=y[k])return 0;return 1;
}
static float f(fln_u64 p,unsigned off){return *(const volatile float *)(p+off);}
static fln_u64 flow_q(fln_u64 p,unsigned off){return *(const volatile fln_u64 *)(p+off);}
static unsigned flow_w(fln_u64 p,unsigned off){return *(const volatile unsigned *)(p+off);}
static unsigned finite(float x){union{float f;unsigned u;}v={x};return (v.u&0x7fffffffu)<0x7f800000u;}
static float root(float x){float r;__asm__("fsqrt %s0,%s1":"=w"(r):"w"(x));return r;}
static unsigned domain(const float *p){return p[0]>=-3200&&p[0]<=-2850&&p[1]>=5450&&p[1]<=6450&&p[2]>=-80&&p[2]<=160;}
static unsigned direction(const float *a,const float *b,float *v,float *length){
    float d2=0;for(unsigned k=0;k<3;k++){v[k]=b[k]-a[k];d2+=v[k]*v[k];}
    if(!finite(d2)||d2<=0)return 0;*length=root(d2);
    for(unsigned k=0;k<3;k++)v[k]/=*length;return 1;
}
static unsigned motion(const FlowLaneNativeFrame *frame,const float *old,const float *next){
    float a[3],b[3],length;if(!direction(frame->proposal.position,old,a,&length)||!direction(frame->proposal.position,next,b,&length))return 0;
    float baseline=0,candidate=0;for(unsigned k=0;k<3;k++){baseline+=frame->motion[k]*a[k];candidate+=frame->motion[k]*b[k];}
    return baseline>=.985f&&candidate>=.965f;
}
static unsigned flow(const FlowLaneNativeFrame *frame,const float *center,const float *origin,FlowLaneFlowResult *result,FlowLaneActuatorResult *receipt,FlowLaneOwnedFlow *seal,FlowLaneDecodeCache *geometry){
    unsigned physical=0;
    /* Ordered flow-material subset of the complete prepared region. All
     * prisms remain covered by geometry preparation and final validation. */
    for(unsigned i=0;i<geometry->flow_count;i++){
        if(receipt->point_queries>=65536)return 0;receipt->point_queries++;
        FlowLanePointContact contact;
        if(!lab_flow_geometry_flow_point(geometry,i,center,frame->body_radius,&contact))return 0;
        if(contact.hit){physical=1;break;}
    }
    *result=(FlowLaneFlowResult){.status=FLF_EVALUATED,.mode=-1};
    if(!physical)return 1;
    if(receipt->flow_queries>=256)return 0;receipt->flow_queries++;
    if(lab_flow_owned_query(seal,origin,result)!=FLF_EVALUATED)return 0;
    /* Initial candidate only qualifies ordinary tangential mode1. */
    return !result->accepted||result->mode==1;
}
static const float *normal(const FlowLaneFacetWorkspace *route,FlowLanePrismRef ref){
    for(unsigned i=0;i<route->facets;i++)if(route->facet[i].ref.model==ref.model&&route->facet[i].ref.prism==ref.prism)return route->facet[i].normal;
    return 0;
}
static unsigned finish_point(FlowLaneActuatorWorkspace *work,float finish[3]){
    const FlowLaneNativeFrame *frame=&work->frame;
    FlowLaneFacetInput in={.local_offset_y=f(frame->physics,0x15c),.radius=frame->body_radius};
    for(unsigned k=0;k<3;k++){in.a[k]=in.b[k]=f(frame->following,0xc+4*k);in.up[k]=f(frame->following,0x90+4*k);}
    if(!domain(in.a)||lab_flow_lane_native_facet_route(&work->mesh,probe_refs,sizeof(probe_refs)/sizeof(probe_refs[0]),&in,&work->route,&work->decode_cache)!=FLFR_READY)return 0;
    const FlowLaneFacetSegment *last=&work->route.segment[work->route.segments-1];const float *up=normal(&work->route,last->ref);if(!up)return 0;
    for(unsigned k=0;k<3;k++)finish[k]=last->b[k]-in.local_offset_y*up[k];return domain(finish);
}
static unsigned path(FlowLaneActuatorWorkspace *work,const float target[3],const float finish[3],float offset,FlowLaneCostPath *cost,float terminal[3],FlowLaneActuatorResult *out){
    const FlowLaneNativeFrame *frame=&work->frame;const volatile float *local=(const volatile float *)(frame->physics+0x158);
    if(local[0]!=0||local[2]!=0||!finite(local[1])||local[1]<=0||local[1]>32||frame->body_radius+2*local[1]>128)return 0;
    float begin[3],up[3],end[3],join[3];for(unsigned k=0;k<3;k++){begin[k]=frame->proposal.position[k];up[k]=frame->up[k];join[k]=frame->body_center[k];}
    cost->offset=offset;cost->count=0;
    for(unsigned leg=0;leg<2;leg++){
        for(unsigned k=0;k<3;k++)end[k]=leg?finish[k]:target[k];
        if(!domain(begin)||!domain(end))return 0;
        FlowLaneFacetInput in={.local_offset_y=local[1],.radius=frame->body_radius};
        for(unsigned k=0;k<3;k++){in.a[k]=begin[k];in.b[k]=end[k];in.up[k]=up[k];}
        out->route_status=lab_flow_lane_native_facet_route(&work->mesh,probe_refs,sizeof(probe_refs)/sizeof(probe_refs[0]),&in,&work->route,&work->decode_cache);
        if(out->route_status!=FLFR_READY)return 0;
        {float error=0;for(unsigned k=0;k<3;k++){float d=work->route.segment[0].a[k]-join[k];error+=d*d;}if(error>.0001f)return 0;}
        if(lab_flow_lane_route_obstacles(&work->mesh,probe_refs,sizeof(probe_refs)/sizeof(probe_refs[0]),&work->route,frame->body_radius,&work->obstacles,&work->decode_cache,&work->prepared_geometry)!=1)return 0;
        const float *prior_normal=normal(&work->route,work->route.segment[0].ref);if(!prior_normal)return 0;
        for(unsigned i=0;i<work->route.segments;i++){
            const FlowLaneFacetSegment *s=&work->route.segment[i];
            if(!domain(s->a)||!domain(s->b)||lab_flow_lane_native_binding_dynamic(frame,s->a,s->b,frame->body_radius)!=1)return 0;
            const float *end_up=normal(&work->route,s->ref);if(!end_up)return 0;
            const float *start_up=s->seam?prior_normal:end_up;
            float origin_a[3],origin_b[3];
            for(unsigned k=0;k<3;k++){origin_a[k]=s->a[k]-local[1]*start_up[k];origin_b[k]=s->b[k]-local[1]*end_up[k];terminal[k]=origin_b[k];}
            prior_normal=end_up;
            float dir[3],length;
            if(!direction(origin_a,origin_b,dir,&length)){
                unsigned same=1;for(unsigned k=0;k<3;k++)if(origin_a[k]!=origin_b[k])same=0;
                if(same)continue;return 0;
            }
            if(cost->count==FLOW_NATIVE_COST_SEGMENTS)return 0;
            float center[3],origin[3];for(unsigned k=0;k<3;k++){center[k]=(s->a[k]+s->b[k])*.5f;origin[k]=(origin_a[k]+origin_b[k])*.5f;}
            FlowLaneFlowResult current;if(!flow(frame,center,origin,&current,out,&work->flow_seal,&work->decode_cache))return 0;
            FlowLaneCostSegment *v=&cost->segments[cost->count++];v->distance=length;v->flags=current.accepted?31:15;
            for(unsigned k=0;k<3;k++){v->direction[k]=dir[k];v->flow[k]=current.accepted?current.vector[k]:0;}
        }
        const FlowLaneFacetSegment *last=&work->route.segment[work->route.segments-1];const float *last_up=normal(&work->route,last->ref);if(!last_up)return 0;
        for(unsigned k=0;k<3;k++)up[k]=last_up[k];
        for(unsigned k=0;k<3;k++){join[k]=last->b[k];begin[k]=last->b[k]-local[1]*up[k];}
    }
    return cost->count!=0;
}
unsigned lab_flow_lane_native_actuate_observed(const FlowLaneNativeEntry *entry,FlowLaneGeometryCache *cache,FlowLaneActuatorWorkspace *work,FlowLaneActuatorResult *out,FlowLaneActor *actor){
    if(!out)return FLA_INVALID;*out=(FlowLaneActuatorResult){.status=FLA_LIFETIME};
    FlowLaneScopeLease scope={0};
    if(!entry||!cache||!work||work->reserved){out->status=FLA_INVALID;return FLA_INVALID;}
    unsigned expected=0;if(!__atomic_compare_exchange_n(&work->lock,&expected,1,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED)){out->status=FLA_BUSY;return out->status;}
    work->flow_seal.ready=0;
    FlowLaneNativeFrame *frame=&work->frame;out->status=FLA_OWNER;
    if(lab_flow_lane_native_binding_collect(entry,frame)!=FLN_COLLECTED)goto done;
    out->original_offset=out->selected_offset=frame->proposal.original_offset;out->status=FLA_LIFETIME;
    if(!lab_flow_lane_scope_pin(frame,&work->pin,&scope))goto done;
    if(actor)*actor=(FlowLaneActor){
        .role=frame->role,.physics=frame->physics,.driver=frame->driver,.context=frame->context,
        .race=frame->race,.system=frame->system,.clock=frame->clock,.route=frame->route,
        .current=frame->current,.following=frame->following,
        .lifecycle_generation=work->pin.generation,.scene_generation=work->pin.scene_generation,
        .valid=1,.racer=frame->racer,.tick=frame->tick,
        .current_key=frame->proposal.current_key,.following_key=frame->proposal.following_key};
    out->status=FLA_MESH;if(!lab_flow_mesh_open(frame->course_data,&work->mesh))goto done;
    out->status=FLA_COLD;if(!lab_flow_course_admit(&lab_flow_course_publication,frame->main_base,frame->static_wrapper,&work->mesh,&work->course_pin))goto done;
    if(actor)actor->course_generation=work->course_pin.generation;
    FlowLaneBaseOutput proposed;out->status=FLA_PROPOSAL;
    if(!lab_flow_lane_base_proposal(&frame->proposal,&proposed))goto done;
    float old[3];for(unsigned k=0;k<3;k++)old[k]=frame->proposal.native_center[k]+frame->proposal.native_side[k]*frame->proposal.original_offset;
    out->status=FLA_MOTION;if(flow_q(frame->physics,0x50)!=frame->physics+0x2c||!motion(frame,old,proposed.target))goto done;
    unsigned following_bits[6],previous_bits[3];float previous[3];
    for(unsigned k=0;k<3;k++){
        following_bits[k]=flow_w(frame->following,0xc+4*k);following_bits[k+3]=flow_w(frame->following,0x90+4*k);
        previous_bits[k]=flow_w(frame->physics,0x1ac+4*k);previous[k]=f(frame->physics,0x1ac+4*k);
    }
    out->status=FLA_ROUTE;
    if(!lab_flow_prepared_begin(&work->course_pin,&work->mesh,(const unsigned *)probe_refs,sizeof(probe_refs)/sizeof(probe_refs[0]),&work->prepared_geometry,&work->decode_cache))goto done;
    float finish[3],ends[2][3];if(!finish_point(work,finish))goto done;
    if(lab_flow_owned_open(frame,&work->flow_seal)!=FLF_EVALUATED)goto done;
    if(!path(work,old,finish,frame->proposal.original_offset,&work->paths[0],ends[0],out)||
       !path(work,proposed.target,finish,proposed.offset,&work->paths[1],ends[1],out))goto done;
    for(unsigned i=0;i<2;i++){
        float error=0;for(unsigned k=0;k<3;k++){float d=ends[i][k]-finish[k];error+=d*d;}
        if(error>.0001f)goto done;
    }
    out->status=FLA_COST;
    if(lab_flow_lane_native_cost_rank(frame->proposal.speed,previous,work->paths,2,&out->cost)!=1)goto done;
    FlowCoursePin current_course;
    out->status=FLA_CHANGED;
    if(!lab_flow_lane_scope_final(entry,frame,&work->pin,&scope)||!lab_flow_course_same(&lab_flow_course_publication,&work->course_pin)||
       !lab_flow_course_admit(&lab_flow_course_publication,frame->main_base,frame->static_wrapper,&work->mesh,&current_course)||
       !course_equal(&work->course_pin,&current_course))goto done;
    for(unsigned k=0;k<3;k++)if(following_bits[k]!=flow_w(frame->following,0xc+4*k)||following_bits[k+3]!=flow_w(frame->following,0x90+4*k)||previous_bits[k]!=flow_w(frame->physics,0x1ac+4*k))goto done;
    if(lab_flow_owned_close(&work->flow_seal)!=FLF_EVALUATED||
       !lab_flow_course_same(&lab_flow_course_publication,&work->course_pin))goto done;
    if(!lab_flow_prepared_finish(&work->prepared_geometry,&work->decode_cache)||
       !lab_flow_lane_scope_final(entry,frame,&work->pin,&scope)||
       !lab_flow_course_same(&lab_flow_course_publication,&work->course_pin))goto done;
    *(volatile float *)(frame->route+0x98)=proposed.offset;
    out->selected_offset=proposed.offset;out->status=FLA_COMMITTED;
done:
    work->decode_cache.geometry_ready=0;
    work->flow_seal.ready=0;
    __atomic_store_n(&work->lock,0,__ATOMIC_RELEASE);return out->status;
}

unsigned lab_flow_lane_native_actuate(const FlowLaneNativeEntry *entry,FlowLaneGeometryCache *cache,
    FlowLaneActuatorWorkspace *work,FlowLaneActuatorResult *out){
    return lab_flow_lane_native_actuate_observed(entry,cache,work,out,0);
}
