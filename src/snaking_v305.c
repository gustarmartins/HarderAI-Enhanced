/* Experimental autonomous-CPU blue-turbo pairs on connected straight routes.
 * All charge, drift-state transitions and boost requests remain native.
 * Only this callback's owned movement packet is edited. Physical players,
 * ordinary native drift maneuvers, recoveries and unsupported scenes pass on.
 */
typedef unsigned char u8;
typedef unsigned int u32;
typedef signed int i32;
typedef unsigned long long u64;
#define EXPORT __attribute__((visibility("default")))
extern char lab_module_start[];
#ifndef LAB_MAIN_TO_MODULE
#error Main-to-module placement must be explicit
#endif

EXPORT const struct {
    u32 abi,leg_deadline,settle,initial_delay;
    u32 cooldown[12];
    float margin,minimum_width,alignment_squared,maximum_speed;
} lab_snaking_config={1,110,12,240,
    {720,720,720,1080,1080,1080,1440,1440,1440,0,0,0},
    40.f,100.f,.9801f,40.f};

/* Course-progress distances, shared with the native formation wrapper. A
 * larger gap changes how often a CPU may earn a turbo, never its AI ratio. */
EXPORT const struct {
    u32 abi;
    float crowded,neutral_gap,ramp,catch_reduction,leader_delay;
} lab_snaking_gap_config={1,120.f,300.f,1200.f,.35f,.5f};

typedef struct {
    u64 driver,producer,clock,previous_end;
    u32 lock,last,phase,start,next_tick;
    i32 direction,first_side;
    u32 steer_count,steer_period,episodes,release_requests,aborts,reason,pad;
} SnakeState;
_Static_assert(sizeof(SnakeState)==88,"Snaking state ABI");
EXPORT SnakeState lab_snaking_state[12];

static u64 q(const void *p,u32 o){return *(const u64 *)((const u8 *)p+o);}
static u32 w(const void *p,u32 o){return *(const u32 *)((const u8 *)p+o);}
static u8 b(const void *p,u32 o){return *((const u8 *)p+o);}
static float f(const void *p,u32 o){return *(const float *)((const u8 *)p+o);}
static void sw(void *p,u32 o,u32 v){*(u32 *)((u8 *)p+o)=v;}
static void sf(void *p,u32 o,float v){*(float *)((u8 *)p+o)=v;}
static u32 ptr(u64 p){return p>=0x10000 && !(p&7) && p<(1ull<<39)-0x2000;}
static u32 finite(float v){union {float f;u32 u;}x={.f=v};return (x.u&0x7fffffffu)<0x7f800000u;}
static float root(float v){float out;__asm__("fsqrt %s0,%s1":"=w"(out):"w"(v));return out;}

static float gap_weight(float distance){
    float v=(distance-lab_snaking_gap_config.neutral_gap)/lab_snaking_gap_config.ramp;
    return v<0?0:v>1?1:v;
}

EXPORT u32 lab_snaking_gap_cooldown(u32 cooldown,float ahead,float behind){
    if(!cooldown||!finite(ahead)||!finite(behind)||ahead<0||behind<0)return cooldown;
    float factor=ahead<1.e20f?1.f-lab_snaking_gap_config.catch_reduction*gap_weight(ahead):
        behind<1.e20f?1.f+lab_snaking_gap_config.leader_delay*gap_weight(behind):1.f;
    return (u32)((float)cooldown*factor+.5f);
}

static u32 pack_cooldown(u64 wrapper,u64 wrappers,u64 drivers,u64 group,u32 count,
                          u32 racer,u32 cooldown,u32 *crowded){
    *crowded=0;
    float progress=f((void *)wrapper,0x2c),ahead=1.e20f,behind=1.e20f;
    if(count<2||count>12||!finite(progress))return cooldown;
    for(u32 i=0;i<count;i++)if(i!=racer){
        u64 peer=q((void *)wrappers,8*i),driver=q((void *)drivers,8*i);
        if(!ptr(peer)||!ptr(driver)||peer==wrapper||driver==q((void *)wrapper,8)||
           q((void *)peer,8)!=driver||q((void *)driver,0x60)!=group)return cooldown;
        for(u32 j=0;j<i;j++)if(q((void *)wrappers,8*j)==peer||q((void *)drivers,8*j)==driver)return cooldown;
        float delta=f((void *)peer,0x2c)-progress;
        if(!finite(delta))return cooldown;
        if(delta>=0){if(delta<ahead)ahead=delta;}else if(-delta<behind)behind=-delta;
    }
    *crowded=ahead<lab_snaking_gap_config.crowded||behind<lab_snaking_gap_config.crowded;
    return lab_snaking_gap_cooldown(cooldown,ahead,behind);
}

/* Native route keys index group records then point pointers. Check the key
 * back on the resolved point, including the currently selected point. */
static u64 point(void *course,u32 key){
    u32 group=key&0xffff,index=key>>16,count=w(course,0x18);
    u64 groups=q(course,0x20);
    if(!count||count>1024||group>=count||!ptr(groups)||index==0xffff)return 0;
    void *row=(void *)(groups+48ull*group);
    u32 n=w(row,0x18);u64 items=q(row,0x20);
    if(!n||n>65535||index>=n||!ptr(items))return 0;
    u64 result=q((void *)items,8*index);
    return ptr(result)&&w((void *)result,0x8c)==key?result:0;
}

static u32 successor(void *p,u32 *key){
    u32 a=w(p,0x30),z=w(p,0x34);u8 selector=b(p,0x2e4);
    if(a>10||z>10||selector>1||b(p,0x370)||(selector?z:a)!=1)return 0;
    *key=w(p,selector?0x60:0x38);return 1;
}

static u32 corridor(void *physics,void *route,u32 ticks,u64 previous_end,u64 *last,u32 *old_ahead){
    u64 holder=q(route,0x48),a=q(route,0x58),z=q(route,0x50);
    if(!ptr(holder)||!ptr(a)||!ptr(z)||a==z)return 0;
    u64 c=q((void *)holder,0);if(!ptr(c))return 0;
    void *course=(void *)c;
    if(point(course,w((void *)z,0x8c))!=z||point(course,w((void *)a,0x8c))!=a)return 0;
    float speed=f(physics,0x37c);
    if(!finite(speed)||speed<=2.f)return 0;
    for(u32 o=0x424;o<=0x430;o+=4){float v=f(physics,o);if(!finite(v)||v<0)return 0;if(v>speed)speed=v;}
    if(speed>lab_snaking_config.maximum_speed)return 0;
    float required=speed*ticks+64.f,remaining=0,heading[3],norm=0;
    for(u32 i=0;i<3;i++){heading[i]=f(physics,0x20+4*i);if(!finite(heading[i]))return 0;norm+=heading[i]*heading[i];}
    if(norm<.9f||norm>1.1f||heading[1]*heading[1]>.0025f*norm)return 0;
    float up2=0,up_y=f(physics,0x18);
    for(u32 i=0;i<3;i++){float v=f(physics,0x14+4*i);if(!finite(v))return 0;up2+=v*v;}
    if(up2<.9f||up2>1.1f||up_y<=0||up_y*up_y<.9604f*up2)return 0;
    u64 seen[18];seen[0]=a;u32 used=1;*old_ahead=0;
    for(u32 segment=0;segment<16;segment++){
        if(z==previous_end)*old_ahead=1;
        for(u32 j=0;j<used;j++)if(seen[j]==z)return 0;
        seen[used++]=z;
        float delta[3],length2=0,dot=0,width=10000.f;
        for(u32 j=0;j<2;j++){
            void *p=(void *)(j?z:a);float radius=f(p,0x24);
            /* Retail 810E98..810F00 loads Scale and halves all components.
             * X is also the native drift-width gate at 7B6D14 (threshold 70). */
            if(!finite(radius)||radius<lab_snaking_config.minimum_width||radius>10000)return 0;
            if(radius<width)width=radius;
        }
        for(u32 i=0;i<3;i++){
            float start=f((void *)a,0xc+4*i),end=f((void *)z,0xc+4*i);
            if(!finite(start)||!finite(end))return 0;
            delta[i]=end-start;length2+=delta[i]*delta[i];dot+=delta[i]*heading[i];
        }
        if(!finite(length2)||length2<64||dot<=0||dot*dot<lab_snaking_config.alignment_squared*length2*norm||
           delta[1]*delta[1]>.0025f*length2)return 0;
        float length=root(length2);
        if(!segment){
            float offset[3],projection=0,error=0;
            for(u32 i=0;i<3;i++){
                float position=f(physics,0x2c+4*i);if(!finite(position))return 0;
                offset[i]=position-f((void *)a,0xc+4*i);projection+=offset[i]*delta[i];
            }
            if(projection<0||projection>length2)return 0;
            for(u32 i=0;i<3;i++){float e=offset[i]-projection/length2*delta[i];error+=e*e;}
            float margin=lab_snaking_config.margin+2.f*speed;
            if(width<=margin||!finite(error)||error>=(width-margin)*(width-margin))return 0;
            remaining=length-projection/length;
        }else remaining+=length;
        *last=z;
        if(remaining>=required)return 1;
        u32 key;if(!successor((void *)z,&key))return 0;
        a=z;z=point(course,key);if(!z)return 0;
    }
    return 0;
}

static void finish(SnakeState *s,u32 reason){s->reason=reason;__atomic_store_n(&s->lock,0,__ATOMIC_RELEASE);}
static void abort_episode(SnakeState *s,u32 now,u32 cooldown,u32 reason){
    if(s->phase)s->aborts++;
    s->phase=0;s->next_tick=now+cooldown;finish(s,reason);
}

static u32 admissible(u64 base,void *producer){
    u64 eligibility=q(producer,0x80),route_flags=q(producer,0x90);
    return ptr(eligibility)&&ptr(route_flags)&&ptr(q((void *)eligibility,0x68))&&
           ptr(q((void *)eligibility,0x70))&&ptr(q(producer,0x68))&&
           ((u32 (*)(void *))(base+0x5c210))(producer);
}

/* This shadow contains exactly the fields consumed by native 5D468. Native
 * countersteering writes the owned packet and this private phase storage. */
static void steer(u64 base,void *producer,void *packet,SnakeState *s,u32 elapsed){
    u64 shadow[44];
    for(u32 i=0;i<44;i++)((volatile u64 *)shadow)[i]=0;
    shadow[0x70/8]=q(producer,0x70);shadow[0x68/8]=q(producer,0x68);
    shadow[0x98/8]=(u64)packet;
    sw(shadow,0x44,elapsed);sf(shadow,0xa8,(float)s->direction);
    sw(shadow,0x150,s->steer_count);sw(shadow,0x154,s->steer_period);
    ((void (*)(void *))(base+0x5d468))(shadow);
    s->steer_count=w(shadow,0x150);s->steer_period=w(shadow,0x154);
}

EXPORT void lab_snaking_dispatch(void *producer){
    u64 base=(u64)lab_module_start-LAB_MAIN_TO_MODULE;
    if((base&4095)||base>=(1ull<<39)-0x1400000||!ptr((u64)producer))return;
    u64 driver=q(producer,0x70),packet=q(producer,0x98),route=q(producer,0x88);
    if(!ptr(driver)||!ptr(packet)||!ptr(route))return;
    void *d=(void *)driver;u64 adapter=q(d,0x88),state=q(d,0x50);
    if(!ptr(adapter)||!ptr(state)||q((void *)state,0x88)!=driver)return;
    u64 nav=q((void *)state,0xb0),ratio=q((void *)state,0x90);
    if(!ptr(nav)||!ptr(ratio)||q((void *)nav,0x70)!=(u64)producer||q((void *)nav,0x78)!=packet||
       q((void *)nav,0x58)!=driver||q((void *)ratio,0x58)!=driver)return;
    u64 eligibility=q(producer,0x80);
    if(!ptr(eligibility)||q((void *)nav,0x60)!=eligibility||q((void *)eligibility,0x60)!=route)return;
    u64 role=q((void *)adapter,0x48),physics=q((void *)adapter,0x50),wrapper=q((void *)ratio,0x48);
    if(!ptr(role)||!ptr(physics)||!ptr(wrapper)||q((void *)wrapper,8)!=driver||
       q((void *)role,0x28)!=physics||q((void *)physics,0xf8)!=role)return;
    void *r=(void *)role,*p=(void *)physics;
    if(b(r,0xd0)||b(r,0xd2)!=1||b(r,0xd3)!=1||!b(r,0xe1)||b(r,0xe7))return;
    u32 racer=w(r,0xa8),slot=w((void *)wrapper,0x24);
    if(racer>=12||slot>=12)return;
    u64 rootptr=q((void *)base,0x12f6388);
    if(!ptr(rootptr))return;u64 single=q((void *)rootptr,0);
    if(!ptr(single))return;u64 link=q((void *)single,0x20);
    if(!ptr(link))return;u64 scene=q((void *)link,8);
    if(!ptr(scene)||!(b((void *)scene,0x180)&4))return;
    u64 system=q((void *)scene,0x1b0);
    if(!ptr(system))return;u64 race=q((void *)system,0x238),clocks=q((void *)system,0x218);
    if(!ptr(race)||!ptr(clocks))return;u64 clock=q((void *)clocks,0x50);
    if(!ptr(clock)||w((void *)clock,0x38)!=6)return;
    u64 group=q((void *)race,0xb0);
    if(!ptr(group)||q((void *)group,0x28)!=race||q(d,0x60)!=group)return;
    u32 count=w((void *)group,0x58);u64 drivers=q((void *)group,0x60);
    if(count>12||racer>=count||!ptr(drivers)||q((void *)drivers,8*racer)!=driver)return;
    u64 manager=q((void *)group,0x68);
    if(!ptr(manager)||q((void *)ratio,0x40)!=manager)return;
    u64 wrappers=q((void *)manager,0xa0);
    if(!ptr(wrappers)||q((void *)wrappers,8*racer)!=wrapper)return;
    void *config=((void *(*)(void))(base+0x87c244))();
    if(!ptr((u64)config)||w(config,4)==1||w(config,8)>1)return;
    u64 drift=q(p,0x110);
    if(!ptr(drift)||q((void *)drift,8)!=role)return;
    u32 now=w((void *)clock,0x44),cooldown=lab_snaking_config.cooldown[slot],crowded=0;
    if(cooldown)cooldown=pack_cooldown(wrapper,wrappers,drivers,group,count,racer,cooldown,&crowded);
    SnakeState *s=&lab_snaking_state[racer];u32 expected=0;
    if(!__atomic_compare_exchange_n(&s->lock,&expected,1,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED))return;
    if(s->driver!=driver||s->producer!=(u64)producer||s->clock!=clock||now<s->last){
        s->driver=driver;s->producer=(u64)producer;s->clock=clock;s->phase=0;s->previous_end=0;
        s->first_side=(racer&1)?-1:1;s->next_tick=now+lab_snaking_config.initial_delay+17*racer;
    }
    s->last=now;
    u32 flags=w((void *)packet,4),native_state=b(producer,0x41),ds=w((void *)drift,0x10);
    if(!cooldown||(w(r,0x1cc)&0x01204018u)||native_state||(flags&10)||!(flags&1)){
        abort_episode(s,now,cooldown?cooldown:720,1);return;
    }
    if(!s->phase){
        if((i32)(now-s->next_tick)<0){finish(s,2);return;}
        s->next_tick=now+60+2*racer;
        /* Let native driving handle nearby exchanges. Do not cancel an
         * already active drift just because another racer draws alongside. */
        if(crowded){finish(s,14);return;}
        if(ds||!b(p,0x201)||!finite(f((void *)drift,0x80))||f((void *)drift,0x80)<=0||f((void *)drift,0x80)>400){finish(s,3);return;}
        if(!admissible(base,producer)){finish(s,12);return;}
        u64 end=0;u32 old_ahead=0;
        if(!corridor(p,(void *)route,2*(lab_snaking_config.leg_deadline+lab_snaking_config.settle),s->previous_end,&end,&old_ahead)||old_ahead){finish(s,4);return;}
        s->previous_end=end;s->phase=1;s->start=now;s->direction=s->first_side;
        s->first_side=-s->first_side;s->steer_count=0;s->steer_period=0;s->episodes++;
    }
    u32 elapsed=now-s->start;
    if(s->phase==2||s->phase==4){
        if(ds&&ds!=4&&ds!=5){abort_episode(s,now,cooldown,5);return;}
        if(elapsed<lab_snaking_config.settle){finish(s,6);return;}
        if(s->phase==4){s->phase=0;s->next_tick=now+cooldown;finish(s,7);return;}
        if(ds){abort_episode(s,now,cooldown,8);return;}
        if(!admissible(base,producer)){abort_episode(s,now,cooldown,13);return;}
        s->phase=3;s->direction=-s->direction;s->start=now;s->steer_count=0;s->steer_period=0;elapsed=0;
    }
    if(elapsed>=lab_snaking_config.leg_deadline || (ds!=0&&ds!=2&&ds!=(s->direction<0?4u:5u)) ||
       (elapsed>30&&ds!=4&&ds!=5)){abort_episode(s,now,cooldown,9);return;}
    u32 remaining=lab_snaking_config.leg_deadline-elapsed+lab_snaking_config.settle;
    if(s->phase==1)remaining+=lab_snaking_config.leg_deadline+lab_snaking_config.settle;
    u64 end=0;u32 ignored=0;
    if(!corridor(p,(void *)route,remaining,0,&end,&ignored)){abort_episode(s,now,cooldown,10);return;}
    if((ds==4||ds==5)&&w((void *)drift,0x8c)>=1){
        s->phase++;s->start=now;s->release_requests++;finish(s,11);return;
    }
    sw((void *)packet,4,flags|8u);sf((void *)packet,8,(float)s->direction);
    steer(base,producer,(void *)packet,s,elapsed);finish(s,0);
}
