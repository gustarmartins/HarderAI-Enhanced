/* Private, uninstalled native racer-follow line guard prototype.
 * Selected/base path envelopes are NOT KCL or road-edge clearance. A proposed
 * straight chord must be covered continuously by up to eight owned route-edge
 * cylinders. Endpoint-only containment is deliberately insufficient.
 * Native status, probability, age, exit, cooldown and RNG remain native.
 */
typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long long u64;
#define EXPORT __attribute__((visibility("default")))
extern char lab_module_start[];
static u64 q(const void *p,u32 o){return *(const u64 *)((const u8 *)p+o);}
static u32 w(const void *p,u32 o){return *(const u32 *)((const u8 *)p+o);}
static u8 b(const void *p,u32 o){return *((const u8 *)p+o);}
static float f(const void *p,u32 o){return *(const float *)((const u8 *)p+o);}
static u32 ptr(u64 p){return p>=0x10000&&!(p&7)&&p<(1ull<<39)-0x1000;}
static u32 finite(float v){union{float f;u32 u;}x={.f=v};return (x.u&0x7fffffffu)<0x7f800000u;}
static float root(float v){float r;__asm__("fsqrt %s0,%s1":"=w"(r):"w"(v));return r;}
#include "native_base_envelope_v305.inc"

/* Stack record belongs to exactly one wrapped 4CE2C -> 4894C update. The two
 * assembly seams validate its exact stack address before passing it here. */
typedef struct {u64 magic,other,packet,target;u32 denied,reason,primary_accepted,secondary_denied;} FollowJob;
_Static_assert(sizeof(FollowJob)==48,"FollowJob overlaps saved X28/LR");
EXPORT const struct {u32 abi,edges;float margin,plane2;} lab_racer_follow_config={2,8,24.f,.01f};

static u32 job_matches(FollowJob *job,void *other,void *target){
    return job->magic==0x32474a46u&&job->other==(u64)other&&job->target==(u64)target&&
           be_span((u64)other,0x1c8)&&q(other,0xa0)==job->packet&&q(other,0xa8)==job->target;
}

static u32 reject(FollowJob *job,u32 reason){job->reason=reason;return 1;}

/* Clip a chord parameter interval against 0 <= a + b*t <= upper. */
static u32 slab(float a,float slope,float upper,float *lo,float *hi){
    if(!finite(a)||!finite(slope)||!finite(upper)||upper<=0)return 0;
    if(slope==0)return a>=0&&a<=upper;
    float x=-a/slope,y=(upper-a)/slope;
    if(x>y){float swap=x;x=y;y=swap;}
    if(x>*lo)*lo=x;if(y<*hi)*hi=y;
    return finite(*lo)&&finite(*hi)&&*lo<=*hi;
}

static u32 edge_axis(const void *a,const void *z,const float *position,const float *motion,
                     const float *up,float up2,float *delta,float *length2,
                     float *projection,float *advance){
    float n=0,p=0,v=0,h=0;
    for(u32 i=0;i<3;i++){
        float start=f(a,0xc+4*i),end=f(z,0xc+4*i);
        if(!finite(start)||!finite(end))return 0;
        delta[i]=end-start;n+=delta[i]*delta[i];
        p+=(position[i]-start)*delta[i];v+=motion[i]*delta[i];h+=delta[i]*up[i];
    }
    if(!finite(n)||n<64||!finite(p)||!finite(v)||!finite(h)||
       h*h>lab_racer_follow_config.plane2*n*up2)return 0;
    *length2=n;*projection=p;*advance=v;return 1;
}

/* Returns the exact interval of the straight candidate chord covered by the
 * finite base cylinder AND the selected edge's forward longitudinal slab. */
static u32 edge_interval(const void *a,const void *z,const void *ba,const void *bz,
                         const float *position,const float *motion,const float *up,float up2,
                         float *lo,float *hi){
    float d[3],n,p,v;
    if(!edge_axis(a,z,position,motion,up,up2,d,&n,&p,&v)||v<=0||!slab(p,v,n,lo,hi))return 0;
    if(!edge_axis(ba,bz,position,motion,up,up2,d,&n,&p,&v)||v<=0||!slab(p,v,n,lo,hi))return 0;
    float radius=10000.f;
    for(u32 i=0;i<3;i++)for(u32 j=0;j<2;j++){
        float r=f(j?bz:ba,0x24+4*i);
        if(!finite(r)||r<=0||r>10000)return 0;
        if(r<radius)radius=r;
    }
    radius-=lab_racer_follow_config.margin;if(radius<=0)return 0;
    float aa=0,bb=0,cc=-radius*radius;
    for(u32 i=0;i<3;i++){
        float e=position[i]-f(ba,0xc+4*i)-p/n*d[i];
        float t=motion[i]-v/n*d[i];
        aa+=t*t;bb+=2*e*t;cc+=e*e;
    }
    if(!finite(aa)||!finite(bb)||!finite(cc))return 0;
    if(aa==0){if(cc>0)return 0;}
    else{
        float discriminant=bb*bb-4*aa*cc;
        if(!finite(discriminant)||discriminant<0)return 0;
        float rr=root(discriminant),x=(-bb-rr)/(2*aa),y=(-bb+rr)/(2*aa);
        if(x>*lo)*lo=x;if(y<*hi)*hi=y;
    }
    return finite(*lo)&&finite(*hi)&&*lo<=*hi;
}

static u64 successor(void *course,const void *point){
    u8 selected=b(point,0x2e4);u32 n=w(point,0x30),p=w(point,0x34);
    if(selected>1||n>10||p>10||(selected?p:n)!=1||b(point,0x370))return 0;
    return be_point(course,w(point,selected?0x60:0x38));
}

static u32 corridor(void *driver,void *route,void *physics,const float *position,const float *candidate,u64 base,u32 primary){
    u64 selected=q(driver,0x80),previous=q(route,0x60),next=q(route,0x58),older=q(route,0x68);
    if(!be_span(selected,0x28)||!be_span(previous,0x374)||!be_span(next,0x374)||previous==next)return 0;
    u64 native_base=be_course(base,driver,route);if(!native_base)return 0;
    float up[3],up2=0,motion[3],motion2=0;
    for(u32 i=0;i<3;i++){
        if(!finite(position[i])||!finite(candidate[i]))return 0;
        up[i]=f(physics,0x14+4*i);motion[i]=candidate[i]-position[i];
        if(!finite(up[i])||!finite(motion[i]))return 0;
        up2+=up[i]*up[i];motion2+=motion[i]*motion[i];
    }
    if(up2<.9f||up2>1.1f||!finite(motion2)||motion2<=.0001f||(primary&&motion2>=90000))return 0;
    void *a=(void *)previous,*z=(void *)next;
    if(be_point((void *)selected,w(a,0x8c))!=previous||be_point((void *)selected,w(z,0x8c))!=next||
       !be_edge(a,w(z,0x8c)))return 0;
    float d[3],n,p,v;
    if(!edge_axis(a,z,position,motion,up,up2,d,&n,&p,&v))return 0;
    if(p<0){
        if(!be_span(older,0x374)||older==previous||older==next)return 0;
        void *old=(void *)older;
        if(be_point((void *)selected,w(old,0x8c))!=older||!be_edge(old,w(a,0x8c))||
           !edge_axis(old,a,position,motion,up,up2,d,&n,&p,&v)||p<0||p>n)return 0;
        z=a;a=old;
    }else if(p>n)return 0;
    float covered=0;u64 visited[9];visited[0]=(u64)a;
    for(u32 edge=0;edge<lab_racer_follow_config.edges;edge++){
        for(u32 i=0;i<=edge;i++)if(visited[i]==(u64)z)return 0;
        visited[edge+1]=(u64)z;
        u64 ba=be_point((void *)native_base,w(a,0x8c)),bz=be_point((void *)native_base,w(z,0x8c));
        if(!ba||!bz||!be_edge((void *)ba,w((void *)bz,0x8c)))return 0;
        float lo=0,hi=1;
        if(!edge_interval(a,z,(void *)ba,(void *)bz,position,motion,up,up2,&lo,&hi)||
           lo>covered||hi<covered)return 0;
        if(hi>covered)covered=hi;
        if(covered>=1){
            /* Same-job ownership readback; no concurrent lifetime lease. */
            return q(driver,0x80)==selected&&q(route,0x60)==previous&&q(route,0x58)==next&&
                   q(route,0x68)==older&&q(route,0x40)==(u64)driver&&q(route,0x48)==(u64)driver+0x80;
        }
        a=z;u64 following=successor((void *)selected,a);if(!following)return 0;z=(void *)following;
    }
    return 0;
}

/* 0: unsupported scope, 1: qualified refusal, 2: qualified chord accepted.
 * State4 admission belongs to the primary seam. A recorded primary acceptance
 * lets the later seam check its own candidate after native downstream exits. */
static u32 follow_policy(FollowJob *job,void *other,void *target,const float *candidate,const float *position,u32 primary){
    u64 base=(u64)lab_module_start-LAB_MAIN_TO_MODULE;
    if(!job_matches(job,other,target)||q(other,0)!=base+0x11acd40||
       !be_span((u64)target,44)||b(target,0x28))return 0;
    u64 driver=q(other,0x70),follow=q(other,0x78),navigation=q(other,0x28),route=q(other,0x90);
    if(!be_span(driver,0xd8))return 0;
    u64 state=q((void *)driver,0x50),adapter=q((void *)driver,0x88);
    if(!be_span(state,0xb8)||!be_span(adapter,0x58)||q((void *)state,0x88)!=driver||
       q((void *)state,0xb0)!=navigation)return 0;
    u64 role=q((void *)adapter,0x48),physics=q((void *)adapter,0x50);
    /* Only an owned, local, autonomous CPU qualifies the policy. Human,
     * manual, network and invalid role graphs are native pass-through. D0 is
     * a reference/list identity and intentionally does not exclude a CPU. */
    if(!be_span(role,0x1d0)||!be_span(physics,0x100)||q((void *)role,0x28)!=physics||
       q((void *)physics,0xf8)!=role||b((void *)role,0xd2)!=1||b((void *)role,0xd3)!=1||
       b((void *)role,0xe1)!=1||b((void *)role,0xe7)||(w((void *)role,0x1cc)&0x01204018u))return 0;
    if(!be_span(follow,0xd8)||driver==follow||!be_span(navigation,0x90)||q((void *)navigation,0x68)!=(u64)other||
       q((void *)navigation,0x58)!=driver||q((void *)navigation,0x78)!=job->packet||
       q((void *)navigation,0x80)!=(u64)target||!be_span(route,0x70))return reject(job,1);
    u64 group=q(other,0x68),table=q((void *)driver,0),follow_table=q((void *)follow,0);
    if(!be_span(group,0x68)||q((void *)driver,0x60)!=group||q((void *)follow,0x60)!=group||
       !be_span(table,0x190)||!be_span(follow_table,0x190)||
       q((void *)table,0xc0)!=base+0x70fb4||q((void *)table,0x160)!=base+0x71a20||
       q((void *)follow_table,0xc0)!=base+0x70fb4||q((void *)follow_table,0x160)!=base+0x71a20)return reject(job,2);
    u32 count=w((void *)group,0x58);u64 roster=q((void *)group,0x60);
    if(!count||count>12||!be_span(roster,8ull*count))return reject(job,3);
    u32 own_count=0,follow_count=0;
    for(u32 i=0;i<count;i++){u64 item=q((void *)roster,8*i);own_count+=item==driver;follow_count+=item==follow;}
    if(own_count!=1||follow_count!=1)return reject(job,3);
    if(!corridor((void *)driver,(void *)route,(void *)physics,position,candidate,base,primary))return reject(job,5);
    if(q(other,0x70)!=driver||q(other,0x78)!=follow||q(other,0x90)!=route||
       q(other,0x28)!=navigation||q(other,0x68)!=group||q((void *)group,0x60)!=roster||
       w((void *)group,0x58)!=count)return reject(job,6);
    job->reason=0;return 2;
}

/* Only the primary seam takes the native4C0A8 abort and skips pursuit pace. */
EXPORT u32 lab_racer_follow_deny(FollowJob *job,void *other,void *target,const float *candidate,const float *position){
    if(!job_matches(job,other,target)||b(other,0x41)!=4||w(other,0x44)<2)return 0;
    u32 result=follow_policy(job,other,target,candidate,position,1);
    if(result==1)job->denied=1;
    if(result==2)job->primary_accepted=1;
    return result==1;
}

EXPORT u32 lab_racer_follow_secondary_deny(FollowJob *job,void *other,void *target,const float *candidate,const float *position){
    if(!job_matches(job,other,target))return 0;
    if(job->denied==1){job->secondary_denied=1;return 1;}
    if(job->primary_accepted!=1)return 0;
    /* The native primary update can end state4 before this writer. Permit
     * only that same-update4->0 transition; never broaden to another state. */
    u8 state=b(other,0x41);
    if(state!=4&&!(state==0&&b(other,0x42)==4&&b(other,0x43)==1))return 0;
    u32 result=follow_policy(job,other,target,candidate,position,0);
    if(result==1)job->secondary_denied=1;
    return result==1;
}
