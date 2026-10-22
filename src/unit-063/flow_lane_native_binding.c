#include "flow_lane_native_binding.h"
typedef unsigned char u8;
typedef fln_u32 u32;
typedef fln_u64 u64;
static u64 q(const void *p,u32 o){return *(const volatile u64 *)((const u8 *)p+o);}
static u32 w(const void *p,u32 o){return *(const volatile u32 *)((const u8 *)p+o);}
static u8 b(const void *p,u32 o){return *((const volatile u8 *)p+o);}
static float f(const void *p,u32 o){return *(const volatile float *)((const u8 *)p+o);}
static u32 ptr(u64 p){return p>=0x10000&&!(p&7)&&p<(1ull<<39)-0x6000;}
static u32 finite(float v){union{float f;u32 u;}b={v};return (b.u&0x7fffffffu)<0x7f800000u;}
static float root(float v){float z;__asm__("fsqrt %s0,%s1":"=w"(z):"w"(v));return z;}
static void zero(void *p,u32 n){for(u32 k=0;k<n;k++)((u8 *)p)[k]=0;}
static u32 unit(const void *p,u32 o,float *v){float n=0;for(u32 k=0;k<3;k++){v[k]=f(p,o+4*k);if(!finite(v[k]))return 0;n+=v[k]*v[k];}return n>=.999f&&n<=1.001f;}
#include "native_base_envelope_v305.inc"

u32 lab_flow_lane_native_binding_collect(const FlowLaneNativeEntry *entry,FlowLaneNativeFrame *out){
    if(!out)return FLN_INVALID;zero(out,sizeof(*out));
    if(!entry||entry->reserved||!ptr(entry->route)||!ptr(entry->saved_sp)||
       !entry->main_base||(entry->main_base&4095)||entry->main_base>(1ull<<39)-0x1400000)return FLN_INVALID;
    FlowLaneNativeFrame s;zero(&s,sizeof(s));s.main_base=entry->main_base;s.route=entry->route;
    void *route=(void *)s.route,*sp=(void *)entry->saved_sp;
    /* 7BD430 and 7BD8A0 restore their frames before tail calls. Therefore the
     * 7BE068 saved LR and x19/x20 belong to the real ordinary 7B6D70 source. */
    if(q(sp,0x58)!=s.main_base+0x7b6dc8)return FLN_NATIVE_CALLER;
    s.source=q(sp,0x48);s.request=q(sp,0x40);s.driver=q(route,0x40);
    if(!ptr(s.source)||!ptr(s.request)||!ptr(s.driver)||b((void *)s.request,0)||
       q((void *)s.source,0x58)!=s.driver||q((void *)s.source,0x60)!=s.route||
       w((void *)s.source,0x84)!=1||b((void *)s.source,0x80))return FLN_NATIVE_CALLER;
    void *d=(void *)s.driver;s.state=q(d,0x50);u64 proxy=q(d,0x88);s.group=q(d,0x60);
    if(!ptr(s.state)||!ptr(proxy)||!ptr(s.group)||q(d,0)!=s.main_base+0x11aea40||q(d,0x28)!=s.group||
       q((void *)s.state,0x28)!=s.driver||q((void *)s.state,0x88)!=s.driver)return FLN_NATIVE_OWNER;
    s.nav=q((void *)s.state,0xb0);
    if(!ptr(s.nav)||q((void *)s.nav,0x28)!=s.state||q((void *)s.nav,0x58)!=s.driver||q((void *)s.nav,0x60)!=s.source)return FLN_NATIVE_OWNER;
    s.producer=q((void *)s.nav,0x70);
    if(!ptr(s.producer)||q((void *)s.producer,0x70)!=s.driver||q((void *)s.producer,0x80)!=s.source||q((void *)s.producer,0x88)!=s.route||b((void *)s.producer,0x41))return FLN_NATIVE_OWNER;
    s.role=q((void *)proxy,0x48);s.physics=q((void *)proxy,0x50);
    if(!ptr(s.role)||!ptr(s.physics)||q((void *)s.role,0x28)!=s.physics||q((void *)s.physics,0xf8)!=s.role)return FLN_NATIVE_OWNER;
    void *role=(void *)s.role,*p=(void *)s.physics;s.context=q(role,8);s.racer=w(role,0xa8);s.role_bytes=w(role,0xd0);
    if(s.racer>=12||!ptr(s.context)||q((void *)s.context,8)!=s.role||q((void *)s.context,0x18)!=s.driver||w((void *)s.context,0x40)!=s.racer||
       b(role,0xd0)||b(role,0xd2)!=1||b(role,0xd3)!=1||b(role,0xe1)!=1||b(role,0xe7))return FLN_NATIVE_OWNER;
    u64 global=q((void *)s.main_base,0x12f6388);if(!ptr(global))return FLN_NATIVE_OWNER;
    u64 single=q((void *)global,0);if(!ptr(single))return FLN_NATIVE_OWNER;
    u64 link=q((void *)single,0x20);if(!ptr(link))return FLN_NATIVE_OWNER;
    s.scene=q((void *)link,8);if(!ptr(s.scene)||!(b((void *)s.scene,0x180)&4))return FLN_NATIVE_OWNER;
    s.system=q((void *)s.scene,0x1b0);if(!ptr(s.system))return FLN_NATIVE_OWNER;
    s.race=q((void *)s.system,0x238);u64 clocks=q((void *)s.system,0x218);
    if(!ptr(s.race)||!ptr(clocks)||q((void *)s.race,0xb0)!=s.group||q((void *)s.group,0x28)!=s.race||w((void *)s.race,0x790)!=2)return FLN_NATIVE_OWNER;
    s.clock=q((void *)clocks,0x50);if(!ptr(s.clock)||w((void *)s.clock,0x38)!=6)return FLN_NATIVE_OWNER;
    s.tick=w((void *)s.clock,0x44);s.manager=q((void *)s.group,0x68);
    if(!ptr(s.manager)||q((void *)s.manager,0x28)!=s.group)return FLN_NATIVE_OWNER;
    u32 count=w((void *)s.group,0x58),nc=w((void *)s.manager,0x228),nh=w((void *)s.manager,0x22c);
    u64 drivers=q((void *)s.group,0x60),wrappers=q((void *)s.manager,0xa0),contexts=q((void *)s.race,0xc8);
    if(!count||count>12||s.racer>=count||!nc||nc>count||nh>count||nc+nh!=count||w((void *)s.race,0xc0)!=count||w((void *)s.manager,0x98)!=count||
       !ptr(drivers)||!ptr(wrappers)||!ptr(contexts)||q((void *)drivers,8*s.racer)!=s.driver||q((void *)contexts,8*s.racer)!=s.context)return FLN_NATIVE_OWNER;
    u64 wrapper=q((void *)wrappers,8*s.racer);u32 found=0;
    if(!ptr(wrapper)||q((void *)wrapper,8)!=s.driver)return FLN_NATIVE_OWNER;
    for(u32 k=0;k<nc;k++)if(q((void *)s.manager,0xa8+16*k)==wrapper)found++;
    for(u32 k=0;k<nh;k++)if(q((void *)s.manager,0x168+16*k)==wrapper)return FLN_NATIVE_OWNER;
    if(found!=1)return FLN_NATIVE_OWNER;
    u64 drift=q(p,0x110),dash=q(p,0x118);
    if(!ptr(drift)||q((void *)drift,8)!=s.role||w((void *)drift,0x10)||!ptr(dash)||q((void *)dash,8)!=s.role||
       (b((void *)dash,0x10)&&(int)w((void *)dash,0x14)>0)||(int)w(role,0x1e4)>0||
       (w(role,0x1cc)&0x01204018u)||!b(p,0x200)||!b(p,0x201)||b(p,0x588)||w(p,0x5d4)||f(p,0x3c8)!=0)return FLN_NATIVE_PHYSICS;
    float up[3],pending[3];
    if(!unit(p,0x14,up)||!unit(p,0x320,s.up)||!unit(p,0x2fc,s.motion)||!unit(p,0x308,pending))return FLN_NATIVE_PHYSICS;
    float udot=0,mdot=0;for(u32 k=0;k<3;k++){udot+=up[k]*s.up[k];mdot+=pending[k]*s.motion[k];}
    if(udot<.9995f||mdot<.9995f)return FLN_NATIVE_PHYSICS;
    s.body_radius=f(p,0x16c);float offset2=0;
    if(!finite(s.body_radius)||s.body_radius<=0||s.body_radius>32)return FLN_NATIVE_PHYSICS;
    for(u32 k=0;k<3;k++){
        float local=f(p,0x158+4*k);if(!finite(local))return FLN_NATIVE_PHYSICS;offset2+=local*local;
        s.body_center[k]=f(p,0x2c+4*k);
        for(u32 j=0;j<3;j++)s.body_center[k]+=f(p,8+12*j+4*k)*f(p,0x158+4*j);
        if(!finite(s.body_center[k]))return FLN_NATIVE_PHYSICS;
    }
    if(!finite(offset2)||offset2>1024)return FLN_NATIVE_PHYSICS;
    s.rotation_radius=s.body_radius+root(offset2)+2.f;
    s.selected=q(d,0x80);s.base_course=be_course(s.main_base,d,route);s.current=q(route,0x58);s.following=q(route,0x50);
    if(!s.base_course||!be_span(s.current,0x374)||!be_span(s.following,0x374)||!ptr(s.selected))return FLN_NATIVE_ROUTE;
    FlowLaneBaseInput *i=&s.proposal;i->current_key=w((void *)s.current,0x8c);i->following_key=w((void *)s.following,0x8c);
    if(i->current_key!=(39u<<16)||i->following_key!=(40u<<16)||be_point((void *)s.selected,i->current_key)!=s.current||be_point((void *)s.selected,i->following_key)!=s.following||!be_edge((void *)s.current,i->following_key))return FLN_NATIVE_ROUTE;
    u64 bp=be_point((void *)s.base_course,i->current_key),bz=be_point((void *)s.base_course,i->following_key);
    if(!bp||!bz||!be_edge((void *)bp,i->following_key))return FLN_NATIVE_ROUTE;
    for(u32 k=0;k<10;k++)if(w((void *)s.current,0x288+4*k)!=0xffff)return FLN_NATIVE_ROUTE;
    i->speed=f(p,0x37c);i->original_offset=f(route,0x98);s.offset_bits=w(route,0x98);i->base_half_width=f((void *)bp,0x24);
    for(u32 k=0;k<3;k++){
        union{float f;u32 u;}bits={.u=entry->center_bits[k]};
        if(entry->center_bits[k]!=w((void *)s.current,0xc+4*k))return FLN_NATIVE_ROUTE;
        i->native_center[k]=bits.f;i->native_side[k]=f(route,0x8c+4*k);i->base_center[k]=f((void *)bp,0xc+4*k);i->position[k]=f(p,0x2c+4*k);
    }
    u64 collision=q(p,0x140);if(!ptr(collision))return FLN_NATIVE_COLLISION;
    s.dispatcher=q((void *)collision,0x38);if(!ptr(s.dispatcher))return FLN_NATIVE_COLLISION;
    s.static_wrapper=q((void *)s.dispatcher,0xa8);if(!ptr(s.static_wrapper))return FLN_NATIVE_COLLISION;
    u64 vt=q((void *)s.static_wrapper,0);if(!ptr(vt)||q((void *)vt,0x88)!=s.main_base+0x7c3a50||(q((void *)vt,0x48)!=s.main_base+0x7c7654&&
        !(vt==s.main_base+0x12d0060&&q((void *)vt,0x48)==s.main_base+0x8dc58c))||q((void *)vt,0x60)!=s.main_base+0x7c3554||
       (b((void *)s.static_wrapper,0x28)&7)||!(w((void *)s.static_wrapper,0x68)&2))return FLN_NATIVE_COLLISION;
    s.course_data=q((void *)s.static_wrapper,0x370);if(!ptr(s.course_data))return FLN_NATIVE_COLLISION;
    if(w((void *)s.clock,0x44)!=s.tick||q(route,0x40)!=s.driver||q(route,0x58)!=s.current||q(route,0x50)!=s.following||w(route,0x98)!=s.offset_bits||w(role,0xd0)!=s.role_bytes)return FLN_NATIVE_OWNER;
    for(u32 k=0;k<sizeof(s);k++)((u8 *)out)[k]=((const u8 *)&s)[k];return FLN_COLLECTED;
}

u32 lab_flow_lane_native_binding_dynamic(const FlowLaneNativeFrame *s,const float *a,const float *z,float radius){
    if(!s||!a||!z||!ptr(s->dispatcher)||!finite(radius)||radius<=0||radius>128)return 0;
    for(u32 k=0;k<3;k++)if(!finite(a[k])||!finite(z[k]))return 0;
    u32 count=w((void *)s->dispatcher,0x1390);if(count>100)return 0;
    for(u32 j=0;j<count;j++){
        u64 object=q((void *)s->dispatcher,0x708+8*j);if(!object)continue;if(!ptr(object))return 0;
        if(!(w((void *)object,0x68)&2))continue;
        double lo=0,hi=1;
        for(u32 k=0;k<3;k++){
            float mn=f((void *)object,8+4*k),mx=f((void *)object,0x14+4*k);
            if(!finite(mn)||!finite(mx)||mn>mx)return 0;
            double start=a[k],delta=(double)z[k]-a[k],lower=(double)mn-radius,upper=(double)mx+radius;
            if(!delta){if(start<lower||start>upper){lo=1;hi=0;break;}continue;}
            double enter=(lower-start)/delta,leave=(upper-start)/delta;
            if(enter>leave){double swap=enter;enter=leave;leave=swap;}
            if(enter>lo)lo=enter;if(leave<hi)hi=leave;
        }
        if(lo<=hi)return FLN_DYNAMIC_OVERLAP;
    }
    if(w((void *)s->dispatcher,0x1390)!=count||q((void *)s->dispatcher,0xa8)!=s->static_wrapper)return 0;
    return 1;
}
