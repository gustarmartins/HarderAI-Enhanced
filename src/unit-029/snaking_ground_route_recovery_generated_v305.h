/* Owned grounded route recovery. Plane, positive travel, native nodes,
 * adjacent bends, finite widths and full actual/pending travel remain checked.
 * No native object writes. Successful path coverage is not KCL clearance. */
EXPORT const u32 lab_snaking_ground_route_recovery_config[4]={1,16,48,0};
typedef struct {float sd[3],bd[3],sn,bn,sp,bp;HnbInterval s[2],b[2];} SgrEdge;

static u32 sgr_frame(const float *d,float n,float projection,const HopVectors *v){
    float one=hf_dot(d,v->motion),two=hf_dot(d,v->pending);
    float up=hf_dot(d,v->up),target=hf_dot(d,v->target_up);
    if(!finite(n)||n<64||!finite(one)||!finite(two)||one<=0||two<=0||
       !finite(up)||!finite(target)||up*up>.0025f*n||target*target>.0025f*n)return 0;
    /* Full finite-tube coverage below replaces the angle proxy. */
    (void)projection;return 1;
}
static u32 sgr_cover(const SgrEdge *edges,u32 count,u32 mode,u32 base,float required){
    float end=0;u32 end_open=0,started=0;
    for(u32 i=0;i<count;i++){
        HnbInterval x=base?edges[i].b[mode]:edges[i].s[mode];
        if(x.hi<0)continue;
        if(x.lo>end||(x.lo==end&&(x.lo_open||end_open)))return 0;
        if(x.hi>end){end=x.hi;end_open=x.hi_open;started=1;}
        if(started&&end>required)return 1;
    }
    return 0;
}
static u32 sgr_node(const void *p){
    return b(p,0x2e4)<=1&&!b(p,0x370)&&w(p,0x30)<=10&&w(p,0x34)<=10;
}
static u32 sgr_future(const void *physics,void *course,void *base,u64 a,u64 z,
                      float speed,float required,const HopVectors *v){
    if(!finite(required)||required<=64.f||!finite(speed)||speed<=2.f||
       speed>lab_snaking_config.maximum_speed)return 0;
    SgrEdge edges[16];u64 seen[17];seen[0]=a;u32 used=1;float previous_clearance=0;
    for(u32 segment=0;segment<16;segment++){
        u32 ak=w((void *)a,0x8c),zk=w((void *)z,0x8c),key;
        if(be_point(course,ak)!=a||be_point(course,zk)!=z||
           !sgr_node((void *)a)||!sgr_node((void *)z)||
           !successor((void *)a,&key)||key!=zk)return 0;
        for(u32 i=0;i<used;i++)if(seen[i]==z)return 0;seen[used++]=z;
        u64 ba=be_point(base,ak),bz=be_point(base,zk);
        if(!ba||!bz||!sgr_node((void *)ba)||!sgr_node((void *)bz)||!be_edge((void *)ba,zk))return 0;
        SgrEdge *e=&edges[segment];
        if(!hf_intent(physics,(void *)a,(void *)z,v,0,e->sd,&e->sn,&e->sp)||
           !hf_intent(physics,(void *)ba,(void *)bz,v,0,e->bd,&e->bn,&e->bp)||
           !sgr_frame(e->sd,e->sn,e->sp,v)||!sgr_frame(e->bd,e->bn,e->bp,v))return 0;
        if(segment&&(!hnb_straight(edges[segment-1].sd,edges[segment-1].sn,e->sd,e->sn)||
                     !hnb_straight(edges[segment-1].bd,edges[segment-1].bn,e->bd,e->bn)))return 0;
        float width=f((void *)ba,0x24),other=f((void *)bz,0x24);
        if(!finite(width)||!finite(other)||width<lab_snaking_config.minimum_width||
           other<lab_snaking_config.minimum_width||width>10000||other>10000)return 0;
        if(other<width)width=other;
        float clearance=width-lab_snaking_config.margin-2.f*speed;if(clearance<=0)return 0;
        float so[3],bo[3];
        for(u32 i=0;i<3;i++){
            float p=f(physics,0x2c+4*i);so[i]=p-f((void *)a,0xc+4*i);bo[i]=p-f((void *)ba,0xc+4*i);
            if(!finite(so[i])||!finite(bo[i]))return 0;
        }
        for(u32 mode=0;mode<2;mode++){
            const float *axis=mode?v->pending:v->motion;
            if(!hnb_interval(e->sd,e->sn,e->sp,so,axis,0,0,0,&e->s[mode])||
               !hnb_interval(e->bd,e->bn,e->bp,bo,axis,clearance,0,1,&e->b[mode]))return 0;
            if(segment&&!hnb_selected_join(edges[segment-1].sd,edges[segment-1].sn,e->sd,e->sn,
                so,axis,&edges[segment-1].s[mode],&e->s[mode]))return 0;
            if(segment)scn_join(edges[segment-1].bd,e->bd,bo,axis,
                previous_clearance,clearance,&edges[segment-1].b[mode],&e->b[mode]);
        }
        previous_clearance=clearance;
        if(sgr_cover(edges,segment+1,0,0,required)&&sgr_cover(edges,segment+1,1,0,required)&&
           sgr_cover(edges,segment+1,0,1,required)&&sgr_cover(edges,segment+1,1,1,required))return 1;
        if(!successor((void *)z,&key))return 0;
        a=z;z=be_point(course,key);if(!z)return 0;
    }
    return 0;
}

/* A contracted shared-node ball may bridge only internal, closed
 * longitudinal end planes. Radial exits remain open and cannot be bridged.
 * This reuses the existing native-node cap geometry, with no error epsilon. */
static u32 shg_join(const float *first,const float *second,const float *offset,
    const float *axis,float clearance,float next_clearance,HnbInterval *a,HnbInterval *z){
    if(scn_join(first,second,offset,axis,clearance,next_clearance,a,z))return 1;
    if(a->hi_open||z->lo_open||!finite(a->hi)||!finite(z->lo)||a->hi>=z->lo)return 0;
    float radius=clearance<next_clearance?clearance:next_clearance;
    HnbInterval cap;
    if(!hnc_sphere(offset,axis,radius,&cap)||cap.lo>=a->hi||cap.hi<=z->lo)return 0;
    float cut=a->hi+(z->lo-a->hi)*.5f;
    if(!finite(cut)||cut<=a->lo||cut>=z->hi)return 0;
    a->hi=cut;z->lo=cut;return 1;
}
static u32 shg_frame(const void *a,const void *z,const float *d,float n,
    float projection,const HopVectors *v,u32 future){
    if(sgr_frame(d,n,projection,v))return 1;
    if(!future||!finite(n)||n<64.f)return 0;
    float one[3],two[3];if(!sfp_normals(a,z,v,one,two))return 0;
    float up=hf_dot(d,one),target=hf_dot(d,two);
    float actual=hf_dot(d,v->motion),pending=hf_dot(d,v->pending);
    return finite(up)&&finite(target)&&up*up<=.0025f*n&&target*target<=.0025f*n&&
           finite(actual)&&finite(pending)&&actual>0&&pending>0;
}
static u32 shg_future(const void *physics,void *course,void *base,u64 a,u64 z,
                      float speed,float required,const HopVectors *v){
    if(!finite(required)||required<=64.f||!finite(speed)||speed<=2.f||
       speed>lab_snaking_config.maximum_speed)return 0;
    SgrEdge edges[16];u64 seen[17];seen[0]=a;u32 used=1;float previous_clearance=0;
    for(u32 segment=0;segment<16;segment++){
        u32 ak=w((void *)a,0x8c),zk=w((void *)z,0x8c),key;
        if(be_point(course,ak)!=a||be_point(course,zk)!=z||
           !sgr_node((void *)a)||!sgr_node((void *)z)||
           !successor((void *)a,&key)||key!=zk)return 0;
        for(u32 i=0;i<used;i++)if(seen[i]==z)return 0;seen[used++]=z;
        u64 ba=be_point(base,ak),bz=be_point(base,zk);
        if(!ba||!bz||!sgr_node((void *)ba)||!sgr_node((void *)bz)||!be_edge((void *)ba,zk))return 0;
        SgrEdge *e=&edges[segment];
        if(!sfp_walk_intent(physics,(void *)a,(void *)z,v,0,segment!=0,e->sd,&e->sn,&e->sp)||
           !sfp_walk_intent(physics,(void *)ba,(void *)bz,v,0,segment!=0,e->bd,&e->bn,&e->bp)||
           !shg_frame((void *)a,(void *)z,e->sd,e->sn,e->sp,v,segment)||!shg_frame((void *)ba,(void *)bz,e->bd,e->bn,e->bp,v,segment))return 0;
        if(segment&&(!hnb_straight(edges[segment-1].sd,edges[segment-1].sn,e->sd,e->sn)||
                     !hnb_straight(edges[segment-1].bd,edges[segment-1].bn,e->bd,e->bn)))return 0;
        float width=f((void *)ba,0x24),other=f((void *)bz,0x24);
        if(!finite(width)||!finite(other)||width<lab_snaking_config.minimum_width||
           other<lab_snaking_config.minimum_width||width>10000||other>10000)return 0;
        if(other<width)width=other;
        float clearance=width-lab_snaking_config.margin-2.f*speed;if(clearance<=0)return 0;
        float so[3],bo[3];
        for(u32 i=0;i<3;i++){
            float p=f(physics,0x2c+4*i);so[i]=p-f((void *)a,0xc+4*i);bo[i]=p-f((void *)ba,0xc+4*i);
            if(!finite(so[i])||!finite(bo[i]))return 0;
        }
        for(u32 mode=0;mode<2;mode++){
            const float *axis=mode?v->pending:v->motion;
            if(!hnb_interval(e->sd,e->sn,e->sp,so,axis,0,0,0,&e->s[mode])||
               !hnb_interval(e->bd,e->bn,e->bp,bo,axis,clearance,0,1,&e->b[mode]))return 0;
            if(segment&&!hnb_selected_join(edges[segment-1].sd,edges[segment-1].sn,e->sd,e->sn,
                so,axis,&edges[segment-1].s[mode],&e->s[mode]))return 0;
            if(segment)shg_join(edges[segment-1].bd,e->bd,bo,axis,
                previous_clearance,clearance,&edges[segment-1].b[mode],&e->b[mode]);
        }
        previous_clearance=clearance;
        if(sgr_cover(edges,segment+1,0,0,required)&&sgr_cover(edges,segment+1,1,0,required)&&
           sgr_cover(edges,segment+1,0,1,required)&&sgr_cover(edges,segment+1,1,1,required))return 1;
        if(!successor((void *)z,&key))return 0;
        a=z;z=be_point(course,key);if(!z)return 0;
    }
    return 0;
}
static u32 sgr_route(void *driver,void *physics,void *route,u32 ticks,const SnakeHopFrame *frame){
    if(!ticks||ticks>lab_snaking_config.leg_deadline)return 0;
    HopVectors vectors;if(!df_vectors(physics,frame,&vectors))return 0;
    u64 course=q(driver,0x80),base=be_course((u64)lab_module_start-LAB_MAIN_TO_MODULE,driver,route);
    if(!ptr(course)||!base||course!=frame->course||base!=frame->base_course||
       frame->physics!=(u64)physics||q(physics,0x110)!=frame->drift)return 0;
    float speed=f(physics,0x37c);if(!finite(speed)||speed<=2.f)return 0;
    for(u32 o=0x424;o<=0x430;o+=4){float cap=f(physics,o);if(!finite(cap)||cap<0)return 0;if(cap>speed)speed=cap;}
    if(speed>lab_snaking_config.maximum_speed)return 0;
    u64 a=q(route,0x60),z=q(route,0x58);
    if(!be_span(a,0x374)||!be_span(z,0x374)||a==z||
       be_point((void *)course,w((void *)a,0x8c))!=a||be_point((void *)course,w((void *)z,0x8c))!=z)return 0;
    u32 key;if(!successor((void *)a,&key)||be_point((void *)course,key)!=z)return 0;
    float delta[3],length2,projection;
    if(!hf_intent(physics,(void *)a,(void *)z,&vectors,0,delta,&length2,&projection))return 0;
    if(projection<0){
        u64 older=q(route,0x68);
        if(!be_span(older,0x374)||older==a||older==z||
           be_point((void *)course,w((void *)older,0x8c))!=older||!be_edge((void *)older,w((void *)a,0x8c))||
           !hf_intent(physics,(void *)older,(void *)a,&vectors,1,delta,&length2,&projection))return 0;
        z=a;a=older;
    }else if(projection>length2)return 0;
    return sgr_future(physics,(void *)course,(void *)base,a,z,
                      speed,speed*ticks+64.f,&vectors)||shg_future(physics,(void *)course,(void *)base,a,z,
                      speed,speed*ticks+64.f,&vectors);
}

#include "snaking_ground_future_generated_v305.h"

static u32 sgr_continue(void *driver,void *physics,void *route,u32 ticks,
                        const SnakeHopFrame *frame,const DfrProbe *probe){
    if(probe->stage!=17&&probe->stage!=21)return 0;
    if(probe->segment)return probe->segment<16&&sgf_route(driver,physics,route,ticks,frame);
    return sgr_route(driver,physics,route,ticks,frame);
}

EXPORT u32 lab_snaking_ground_recovery_heading(void *driver,void *physics,void *producer,
                                             void *route,u32 racer,u32 now){
    if(racer>=12||!ptr((u64)driver)||!ptr((u64)physics)||!ptr((u64)producer)||!ptr((u64)route))return 0;
    const SnakeState *s=&lab_snaking_state[racer];
    const SnakeHopFrame *frame=&lab_snaking_drift_frames[racer];
    if(s->lock||s->driver!=(u64)driver||s->producer!=(u64)producer||
       !ptr(s->clock)||w((void *)s->clock,0x44)!=now||w((void *)s->clock,0x38)!=6||
       !ptr((u64)physics)||!ptr((u64)route)||q(producer,0x88)!=(u64)route)return 0;
    u64 drift=q(physics,0x110);if(!ptr(drift))return 0;
    u32 ds=w((void *)drift,0x10);
    if(!df_scope(frame,s,physics,(void *)drift,now,ds))return 0;
    u64 course=q(driver,0x80),base=be_course((u64)lab_module_start-LAB_MAIN_TO_MODULE,driver,route);
    if(!ptr(course)||!base||course!=frame->course||base!=frame->base_course)return 0;
    /* The same tick's phase1 publication establishes successful continuation.
     * Heading feedback has its own four-update tube and 5% slack-change budget.
     * Do not demand that an UNCORRECTED full-charge ray already be safe before
     * allowing the correction that is meant to bend it back toward the route. */
    HopVectors vectors;return df_vectors(physics,frame,&vectors);
}
