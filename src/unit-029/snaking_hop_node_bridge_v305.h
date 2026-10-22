/* Isolated hop-only local forecast across one qualified cached node.
 * Both BASE contracted tube intervals and SELECTED longitudinal intervals
 * must continuously cover the same four-update travel. No native stores. */
EXPORT const struct {u32 abi,max_edges,prediction_updates,reserved;float alignment_squared;}
lab_snaking_hop_node_bridge_config={1,2,4,0,.995f};
/* Effective bend policy; legacy descriptors remain for predecessor contracts. */
EXPORT const struct {u32 abi,max_edges,prediction_updates,current_cache_bound;float alignment_squared;}
lab_snaking_bend_envelope_config={1,16,4,1,.95f};
typedef struct {float lo,hi;u32 lo_open,hi_open;} HnbInterval;
static float hnb_dot(const float *a,const float *z){return a[0]*z[0]+a[1]*z[1]+a[2]*z[2];}
static u32 hnb_straight(const float *a,float a2,const float *z,float z2){
    float d=hnb_dot(a,z);
    return finite(d)&&d>0&&d*d>=lab_snaking_bend_envelope_config.alignment_squared*a2*z2;
}
/* Parameter t is distance traveled along a unit supplied motion axis.
 * Longitudinal bounds are inclusive at the shared internal node. The final
 * endpoint and radial clearance remain strict when coverage is accepted. */
static u32 hnb_interval(const float *delta,float length2,float projection,
    const float *offset,const float *motion,float clearance,float end_reserve,
    u32 radial,HnbInterval *out){
    float advance=hnb_dot(delta,motion),length=root(length2);
    float end=length2-end_reserve*length;
    if(!finite(advance)||advance<=0||!finite(end)||end<=0)return 0;
    float lo=-projection/advance,hi=(end-projection)/advance;
    u32 lo_open=0,hi_open=end_reserve>0;
    if(!finite(lo)||!finite(hi)||hi<=lo)return 0;
    if(radial){
        float a=0,b0=0,c=0;
        for(u32 i=0;i<3;i++){
            float r=offset[i]-projection/length2*delta[i];
            float v=motion[i]-advance/length2*delta[i];
            a+=v*v;b0+=r*v;c+=r*r;
        }
        c-=clearance*clearance;
        if(!finite(a)||!finite(b0)||!finite(c)||a<0||clearance<=0)return 0;
        if(a==0){if(c>=0)return 0;}
        else{
            float discriminant=b0*b0-a*c;
            if(!finite(discriminant)||discriminant<=0)return 0;
            float disc_root=root(discriminant);
            /* Stable quadratic roots for a*t*t+2*b0*t+c. */
            float qv=-b0-(b0<0?-disc_root:disc_root),r0,r1;
            if(qv==0)return 0;
            r0=qv/a;r1=c/qv;
            if(!finite(r0)||!finite(r1))return 0;
            if(r0>r1){float temporary=r0;r0=r1;r1=temporary;}
            if(r0>=lo){lo=r0;lo_open=1;}if(r1<=hi){hi=r1;hi_open=1;}
        }
    }
    if(!finite(lo)||!finite(hi)||hi<=lo)return 0;
    out->lo=lo;out->hi=hi;out->lo_open=lo_open;out->hi_open=hi_open;return 1;
}
#include "snaking_collinear_node_v305.h"

static u32 hnb_cover(HnbInterval current,HnbInterval next,float travel){
    /* A positive future interval cannot substitute for current containment.
     * Overlap/meeting of the two convex intervals covers the entire forecast. */
    return (current.lo<0||(current.lo==0&&!current.lo_open))&&current.hi>0&&
        (current.hi>travel||((next.lo<current.hi||
          (next.lo==current.hi&&!next.lo_open&&!current.hi_open))&&next.hi>travel));
}

/* The two processed selected segments share a node but their normal end
 * planes differ. Join their longitudinal ranges at the unique direction
 * bisector through that same node, avoiding an artificial wedge/gap. The
 * physical BASE tubes still need genuine interval overlap. No epsilon. */
static u32 hnb_selected_join(const float *first,float first2,const float *second,float second2,
    const float *node_offset,const float *axis,HnbInterval *a,HnbInterval *z){
    float normal[3],one=root(first2),two=root(second2);
    for(u32 i=0;i<3;i++)normal[i]=first[i]/one+second[i]/two;
    float advance=hnb_dot(normal,axis),projection=hnb_dot(normal,node_offset);
    if(!finite(advance)||advance<=0||!finite(projection))return 0;
    float cut=-projection/advance;
    if(!finite(cut)||cut<=a->lo||cut>=z->hi)return 0;
    a->hi=cut;a->hi_open=0;z->lo=cut;z->lo_open=0;return 1;
}

/* Private proposal: fill only the internal end-plane wedge of two qualified
 * base edges with their shared contracted node ball. The ball radius is the
 * smaller existing clearance; it is contained in both infinite radial tubes.
 * Current-edge containment, next-edge endpoint, selected path and both motion
 * forecasts still have to pass. No native stores or wider bend allowance. */
EXPORT const u32 lab_snaking_node_cap_config[4]={1,1,4,0};
static u32 hnc_sphere(const float *offset,const float *axis,float radius,HnbInterval *out){
    if(!finite(radius)||radius<=0)return 0;
    float a=hnb_dot(axis,axis),b0=hnb_dot(offset,axis);
    float c=hnb_dot(offset,offset)-radius*radius;
    if(!finite(a)||a<=0||!finite(b0)||!finite(c))return 0;
    float discriminant=b0*b0-a*c;
    if(!finite(discriminant)||discriminant<=0)return 0;
    float d=root(discriminant),qv=-b0-(b0<0?-d:d);
    if(!finite(qv)||qv==0)return 0;
    float lo=qv/a,hi=c/qv;
    if(!finite(lo)||!finite(hi))return 0;
    if(lo>hi){float temporary=lo;lo=hi;hi=temporary;}
    if(lo>=hi)return 0;
    out->lo=lo;out->hi=hi;out->lo_open=1;out->hi_open=1;return 1;
}
static u32 hnc_cover(HnbInterval first,HnbInterval second,float travel,
    const float *node_offset,const float *axis,float clearance,float next_clearance){
    if(hnb_cover(first,second,travel))return 1;
    /* A cap is an internal bridge only. It cannot admit a racer outside its
     * current finite tube, cross a second node, or substitute for the second
     * tube covering the rest of the forecast. Require actual open overlap. */
    if(!(first.lo<0||(first.lo==0&&!first.lo_open))||first.hi<=0||
       second.hi<=travel||second.lo<first.hi)return 0;
    float radius=clearance<next_clearance?clearance:next_clearance;
    HnbInterval cap;
    return hnc_sphere(node_offset,axis,radius,&cap)&&
        cap.lo<first.hi&&cap.hi>second.lo;
}

EXPORT const u32 lab_snaking_projection_trace_config[4]={1,28,255,8};
static u32 hnp_refused(u32 *detail,u32 stage,u32 mode){
    *detail=(*detail&0xffff0000u)|stage|(mode<<8);return 0;
}

#include "snaking_selected_boundary_v305.h"

/* For the cached successor the four-update interval proof below
 * supplies actual and pending clearance. Overlapping longitudinal projections
 * alone do not transfer current-edge ownership. The caller retains its current
 * cones, forward travel, both planes and the bounded adjacent bend. */
static u32 hnb_future_edge(const float *d,float n,float projection,const HopVectors *v){
    if(!finite(projection))return 0;
    float one=hf_dot(d,v->motion),two=hf_dot(d,v->pending);
    float up=hf_dot(d,v->up),target=hf_dot(d,v->target_up);
    return finite(n)&&n>=64.f&&finite(one)&&finite(two)&&one>0&&two>0&&
        finite(up)&&finite(target)&&up*up<=.0025f*n&&target*target<=.0025f*n;
}

static u32 hnb_projection(const void *physics,const void *route,void *course,void *native_base,
    u64 a,u64 z,u64 ba,u64 bz,const float *delta,float length2,float width,float speed,const HopVectors *v,u32 *__restrict detail,float *__restrict observed){
    u32 root_detail=0;
    if(hf_projection(physics,(void *)ba,delta,length2,width,speed,v,&root_detail,observed))return 1;
    *detail=root_detail<<16;
    float offset[3],projection=0;
    for(u32 i=0;i<3;i++){
        offset[i]=f(physics,0x2c+4*i)-f((void *)ba,0xc+4*i);
        if(!finite(offset[i]))return hnp_refused(detail,1,255);projection+=offset[i]*delta[i];
    }
    float travel=4.f*speed,clearance=width-lab_snaking_config.margin-2.f*speed;
    if(!finite(projection)||projection<0||projection>length2||clearance<=0)return hnp_refused(detail,2,255);
    u32 crosses=0;
    for(u32 mode=0;mode<2;mode++){
        float advance=hf_dot(mode?v->pending:v->motion,delta);
        if(!finite(advance)||advance<=0)return hnp_refused(detail,3,mode);
        if(projection+travel*advance>=length2)crosses=1;
    }
    if(!crosses)return hnp_refused(detail,4,255);
    /* Use only the cache successor for this occupied edge, with exact matching
     * singleton topology in selected and base views. No path search. */
    u64 following=scp_following(route,course,a,z);
    u32 key;
    if(!ptr(following)||following==a||following==z)return hnp_refused(detail,5,255);
    if(!successor((void *)z,&key))return hnp_refused(detail,6,255);
    if(be_point(course,key)!=following)return hnp_refused(detail,7,255);
    if(w((void *)following,0x8c)!=key)return hnp_refused(detail,8,255);
    u64 bf=be_point(native_base,key);
    if(!bf||bf==ba||bf==bz)return hnp_refused(detail,9,255);
    if(!be_edge((void *)bz,key))return hnp_refused(detail,10,255);
    float sd[3],sd2,sp,nsd[3],nsd2,nsp,nd[3],nd2,np;
    if(!hf_intent(physics,(void *)a,(void *)z,v,1,sd,&sd2,&sp))return hnp_refused(detail,11,255);
    if(!hf_intent(physics,(void *)z,(void *)following,v,0,nsd,&nsd2,&nsp))return hnp_refused(detail,12,255);
    if(!hf_intent(physics,(void *)bz,(void *)bf,v,0,nd,&nd2,&np))return hnp_refused(detail,13,255);
    if(!hf_edge(sd,sd2,v))return hnp_refused(detail,14,255);
    if(!hnb_future_edge(nsd,nsd2,nsp,v))return hnp_refused(detail,15,255);
    if(!hnb_future_edge(nd,nd2,np,v))return hnp_refused(detail,16,255);
    if(!hnb_straight(sd,sd2,nsd,nsd2))return hnp_refused(detail,17,255);
    if(!hnb_straight(delta,length2,nd,nd2))return hnp_refused(detail,18,255);
    float next_width=f((void *)bz,0x24),r=f((void *)bf,0x24);
    if(!finite(next_width)||!finite(r)||next_width<lab_snaking_config.minimum_width||
       r<lab_snaking_config.minimum_width||next_width>10000||r>10000)return hnp_refused(detail,19,255);
    if(r<next_width)next_width=r;
    float next_clearance=next_width-lab_snaking_config.margin-2.f*speed;
    if(next_clearance<=0)return hnp_refused(detail,20,255);
    float next_offset[3],selected_offset[3],next_selected_offset[3];
    for(u32 i=0;i<3;i++){
        next_offset[i]=f(physics,0x2c+4*i)-f((void *)bz,0xc+4*i);
        selected_offset[i]=f(physics,0x2c+4*i)-f((void *)a,0xc+4*i);
        next_selected_offset[i]=f(physics,0x2c+4*i)-f((void *)z,0xc+4*i);
        if(!finite(next_offset[i])||!finite(selected_offset[i])||!finite(next_selected_offset[i]))return hnp_refused(detail,21,255);
    }
    for(u32 mode=0;mode<2;mode++){
        const float *axis=mode?v->pending:v->motion;
        HnbInterval first_base,second_base,first_selected,second_selected;
        if(!hnb_interval(delta,length2,projection,offset,axis,clearance,0.f,1,&first_base))return hnp_refused(detail,22,mode);
        if(!hnb_interval(nd,nd2,np,next_offset,axis,next_clearance,0.f,1,&second_base))return hnp_refused(detail,23,mode);
        if(!hnb_interval(sd,sd2,sp,selected_offset,axis,0.f,0.f,0,&first_selected))return hnp_refused(detail,24,mode);
        if(!hnb_interval(nsd,nsd2,nsp,next_selected_offset,axis,0.f,0.f,0,&second_selected))return hnp_refused(detail,25,mode);
        if(!hnb_selected_join(sd,sd2,nsd,nsd2,next_selected_offset,axis,&first_selected,&second_selected))return hnp_refused(detail,26,mode);
        if(!hnc_cover(first_base,second_base,travel,next_offset,axis,clearance,next_clearance))return hnp_refused(detail,27,mode);
        if(!lab_snaking_selected_boundary_cover(first_selected,second_selected,travel))return hnp_refused(detail,28,mode);
    }
    return 1;
}
