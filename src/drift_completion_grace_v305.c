/* Experimental near-blue grace at the native route-history exit only.
 * No native memory writes, charge/timer changes or injected steering. The
 * caller retains every earlier native exit and the later backward-dot exit.
 * Course point envelopes are a conservative heuristic, not collision proof.
 */
typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long long u64;
#define EXPORT __attribute__((visibility("default")))

EXPORT const struct {
    u32 abi;
    u8 slot_updates[12];
    float margin, alignment_squared, max_speed;
    u32 max_counter;
} lab_drift_completion_config = {
    1, {6,6,6,4,4,4,2,2,2,0,0,0}, 24.f, .9025f, 100.f, 60000
};

typedef struct {
    u64 producer,drift;
    u32 first,last,limit,initialized;
    u32 lock,offered,held,reason;
} Budget;
EXPORT Budget lab_drift_completion_budget[12];

static u64 q(const void *p,u32 o){return *(const u64 *)((const u8 *)p+o);}
static u32 w(const void *p,u32 o){return *(const u32 *)((const u8 *)p+o);}
static u8 b(const void *p,u32 o){return *((const u8 *)p+o);}
static float f(const void *p,u32 o){return *(const float *)((const u8 *)p+o);}
static u32 ptr(u64 p){return p>=0x10000 && !(p&7) && p<(1ull<<39)-0x1000;}
static u32 finite(float v){union{float f;u32 u;}x={.f=v};return (x.u&0x7fffffffu)<0x7f800000u;}
static u32 done(Budget *s,u32 result,u32 reason){
    s->reason=reason;s->held+=result;
    __atomic_store_n(&s->lock,0,__ATOMIC_RELEASE);return result;
}

static u32 simple_point(const void *p){
    u32 a=w(p,0x30),c=w(p,0x34);u8 selected=b(p,0x2e4);
    return a<=10 && c<=10 && selected<=1 && !b(p,0x370) && (selected?c:a)==1;
}

static u32 corridor(const void *physics,const void *route,u32 ticks){
    u64 previous=q(route,0x58),next=q(route,0x50);
    if(!ptr(previous) || !ptr(next) || previous==next)return 0;
    const void *a=(const void *)previous,*z=(const void *)next;
    if(!simple_point(a) || !simple_point(z))return 0;
    float radius=10000.f,delta[3],offset[3],forward[3];
    float length2=0,projection=0,forward2=0,heading=0;
    for(u32 i=0;i<3;i++){
        float ra=f(a,0x24+4*i),rz=f(z,0x24+4*i);
        float start=f(a,0xc+4*i),end=f(z,0xc+4*i),position=f(physics,0x2c+4*i);
        forward[i]=f(physics,0x20+4*i);
        if(!finite(ra)||!finite(rz)||ra<=0||rz<=0||ra>10000||rz>10000||
           !finite(start)||!finite(end)||!finite(position)||!finite(forward[i]))return 0;
        if(ra<radius)radius=ra;if(rz<radius)radius=rz;
        delta[i]=end-start;offset[i]=position-start;
        length2+=delta[i]*delta[i];projection+=offset[i]*delta[i];
        forward2+=forward[i]*forward[i];heading+=forward[i]*delta[i];
    }
    if(!finite(length2)||length2<64||!finite(forward2)||forward2<.5f||forward2>1.5f||
       !finite(heading)||heading<=0 || !finite(projection) || projection<0 || projection>length2 ||
       heading*heading<lab_drift_completion_config.alignment_squared*length2*forward2 ||
       delta[1]*delta[1]>.01f*length2 || forward[1]*forward[1]>.01f*forward2)return 0;
    float speed=f(physics,0x37c);
    if(!finite(speed)||speed<=0)return 0;
    for(u32 o=0x424;o<=0x430;o+=4){
        float cap=f(physics,o);
        if(!finite(cap)||cap<0)return 0;
        if(cap>speed)speed=cap;
    }
    if(speed>lab_drift_completion_config.max_speed)return 0;
    float travel=speed*ticks+lab_drift_completion_config.margin;
    float clearance=radius-travel;
    float fraction=projection/length2,residual2=0;
    if(clearance<=0 || (1-fraction)*(1-fraction)*length2<=travel*travel)return 0;
    for(u32 i=0;i<3;i++){
        float residual=offset[i]-fraction*delta[i];residual2+=residual*residual;
    }
    return finite(residual2) && residual2<clearance*clearance;
}

EXPORT u32 lab_drift_completion_hold(void *producer,u32 point_match,float native_dot){
    if(!ptr((u64)producer)||point_match>1||!finite(native_dot)||native_dot<0)return 0;
    u64 driver=q(producer,0x70);
    if(!ptr(driver))return 0;
    u64 adapter=q((void *)driver,0x88),state=q((void *)driver,0x50);
    if(!ptr(adapter)||!ptr(state)||q((void *)state,0x88)!=driver)return 0;
    u64 navigation=q((void *)state,0xb0),ratio=q((void *)state,0x90);
    if(!ptr(navigation)||!ptr(ratio)||q((void *)navigation,0x70)!=(u64)producer||
       q((void *)navigation,0x58)!=driver||q((void *)ratio,0x58)!=driver)return 0;
    u64 role=q((void *)adapter,0x48),physics=q((void *)adapter,0x50),wrapper=q((void *)ratio,0x48);
    if(!ptr(role)||!ptr(physics)||!ptr(wrapper)||q((void *)wrapper,8)!=driver||
       q((void *)role,0x28)!=physics||q((void *)physics,0xf8)!=role||
       b((void *)role,0xd0)||b((void *)role,0xd2)!=1||b((void *)role,0xe1)!=1||b((void *)role,0xe7)||
       (w((void *)role,0x1cc)&0x01204018u))return 0;
    u32 racer=w((void *)role,0xa8),slot=w((void *)wrapper,0x24),counter=w(producer,0x44);
    if(racer>=12||slot>=12||counter>lab_drift_completion_config.max_counter)return 0;
    Budget *s=&lab_drift_completion_budget[racer];u32 expected=0;
    if(!__atomic_compare_exchange_n(&s->lock,&expected,1,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED))return 0;
    s->offered++;
    if(!point_match){s->initialized=0;return done(s,0,1);}
    u64 drift=q((void *)physics,0x110),route=q(producer,0x88);
    if(!ptr(drift)||!ptr(route)||q((void *)drift,8)!=role||b(producer,0x41)!=1)return done(s,0,2);
    u32 drift_state=w((void *)drift,0x10),limit=lab_drift_completion_config.slot_updates[slot];
    float direction=f(producer,0xa8);
    if(!limit||!finite(direction)||
       !((drift_state==4&&direction<0)||(drift_state==5&&direction>0))||w((void *)drift,0x8c))
        return done(s,0,3);
    if(!s->initialized||s->producer!=(u64)producer||s->drift!=drift||counter<s->last){
        s->producer=(u64)producer;s->drift=drift;s->first=counter;s->limit=limit;s->initialized=1;
    }
    s->last=counter;if(s->limit<limit)limit=s->limit;
    u32 elapsed=counter-s->first;
    if(elapsed>=limit)return done(s,0,4);
    float charge=f((void *)drift,0x70),target=f((void *)drift,0x80),difference=target-charge;
    if(!finite(charge)||!finite(target)||charge<0||target<=0||!finite(difference)||difference<0||
       difference>=2.f*(limit-elapsed))return done(s,0,5);
    u32 needed=(u32)(difference*.5f)+1; /* Native strict >, minimum 2 per explicit charging update. */
    if(!corridor((void *)physics,(void *)route,needed))return done(s,0,6);
    return done(s,1,0);
}
