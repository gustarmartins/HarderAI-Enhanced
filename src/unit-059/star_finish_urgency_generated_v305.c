/* Finish-aware first use for local CPU Stars, v3.0.5.
 * The native item-planner call owns planner/inventory. Borrow no graph across
 * calls. Compare two complete observations before authorizing the wrapper's
 * single countdown reset; native item consumption and admission remain native.
 * Remaining distance uses this racer's native lap/section representation.
 */
typedef unsigned char u8;typedef unsigned short u16;typedef unsigned int u32;
typedef unsigned long long u64;
#define EXPORT __attribute__((visibility("default")))
extern const u8 lab_module_start[];
EXPORT const u32 lab_star_finish_config[4]={1,450,30,0};
typedef union {u32 u;float f;} Bits;
static u64 q(u64 p,u32 o){return *(const volatile u64 *)(p+o);}
static u32 w(u64 p,u32 o){return *(const volatile u32 *)(p+o);}
static u32 h(u64 p,u32 o){return *(const volatile u16 *)(p+o);}
static u32 b(u64 p,u32 o){return *(const volatile u8 *)(p+o);}
static float f(u64 p,u32 o){Bits v={.u=w(p,o)};return v.f;}
static u32 ptr(u64 p){return p>=0x10000 && !(p&7) && p<(1ull<<39)-0x3000;}
static u32 finite(float f){Bits v={.f=f};return (v.u&0x7fffffffu)<0x7f800000u;}
typedef struct {
    u64 planner,driver,adapter,role,physics,state,inventory,held,root,single,scene_link,scene;
    u64 config_owner,config_link,config,system,race,contexts,context,clocks,clock;
    u64 course_owner,table,course,order,rows,row,boundaries,groups;
    u64 nodes[4];u32 group_index[4],node_index[4],node_count[4];
    u32 racer,count,tick,mode,lap_number,units,phase,flags,boundary_count,group_count;
    float raw,scale,endpoints[4],speed,normal_cap,boost_cap,remaining,travel_speed;
} View;
static u32 endpoint(View *v,u32 index,u32 out){
    if(v->lap_number==1 && index==v->units){v->endpoints[out]=1.f;return 1;}
    if(index>=v->boundary_count)return 0;
    u32 pair=w(v->boundaries,4*index),group=pair&65535u,node=pair>>16;
    if(group>=v->group_count)return 0;
    u64 g=v->groups+0x70ull*group;if(!ptr(g))return 0;
    u32 count=w(g,0x50);u64 array=q(g,0x58);
    if(!count || count>65535 || node>=count || !ptr(array))return 0;
    u64 point=array+0x790ull*node;
    if(!ptr(point)||w(point,0x60)!=pair)return 0;
    float value=f(point,0x6c);if(!finite(value))return 0;
    v->nodes[out]=point;v->group_index[out]=group;v->node_index[out]=node;
    v->node_count[out]=count;v->endpoints[out]=value;return 1;
}
static u32 collect(u64 planner,View *v){
    for(u32 i=0;i<sizeof(*v);i++)((u8 *)v)[i]=0;
    if(!ptr(planner)||b(planner,0x41)!=3)return 0;v->planner=planner;
    v->driver=q(planner,0x90);v->inventory=q(planner,0xa8);
    if(!ptr(v->driver)||!ptr(v->inventory)||(int)w(v->inventory,0xc0)>0)return 0;
    v->held=q(v->inventory,0x58);
    if(!ptr(v->held)||(v->held!=q(v->inventory,0x60)&&v->held!=q(v->inventory,0x68))||
       q(v->held,0x70)!=v->inventory||w(v->held,0x9c)!=8)return 0;
    v->adapter=q(v->driver,0x88);v->state=q(v->driver,0x50);
    if(!ptr(v->adapter)||!ptr(v->state)||q(v->state,0x88)!=v->driver||q(v->state,0xa8)!=planner)return 0;
    v->role=q(v->adapter,0x48);v->physics=q(v->adapter,0x50);
    if(!ptr(v->role)||!ptr(v->physics)||q(v->role,0x28)!=v->physics||q(v->physics,0xf8)!=v->role||
       b(v->role,0xd2)!=1||b(v->role,0xd3)!=1||b(v->role,0xe1)!=1||b(v->role,0xe7)||
       (w(v->role,0x1cc)&0x01204018u)||(int)w(v->role,0x1e4)>0)return 0;
    v->racer=w(v->role,0xa8);if(v->racer>=12)return 0;
    u64 main=(u64)lab_module_start-LAB_MAIN_TO_MODULE;
    v->root=q(main,0x12f6388);if(!ptr(v->root))return 0;
    v->single=q(v->root,0);if(!ptr(v->single))return 0;
    /* Exact read-only path used by native87c244 ->7f41cc. */
    v->config_owner=q(v->single,0x28);if(!ptr(v->config_owner))return 0;
    v->config_link=q(v->config_owner,0x240);if(!ptr(v->config_link))return 0;
    v->config=q(v->config_link,0x10);if(!ptr(v->config))return 0;
    v->mode=w(v->config,8);if(w(v->config,0)||w(v->config,4)||v->mode>1)return 0;
    v->scene_link=q(v->single,0x20);if(!ptr(v->scene_link))return 0;
    v->scene=q(v->scene_link,8);
    if(!ptr(v->scene)||q(v->scene,0)!=main+0x125e158||!(b(v->scene,0x180)&4))return 0;
    v->system=q(v->scene,0x1b0);
    if(!ptr(v->system)||q(v->system,0)!=main+0x125a060||w(v->system,0xb8)!=4)return 0;
    v->race=q(v->system,0x238);
    if(!ptr(v->race)||q(v->race,0)!=main+0x11b4308)return 0;
    v->count=w(v->race,0xc0);v->contexts=q(v->race,0xc8);
    if(!v->count||v->count>12||v->racer>=v->count||!ptr(v->contexts))return 0;
    v->context=q(v->contexts,8*v->racer);
    if(!ptr(v->context)||q(v->context,8)!=v->role||q(v->context,0x18)!=v->driver)return 0;
    v->clocks=q(v->system,0x218);if(!ptr(v->clocks))return 0;
    v->clock=q(v->clocks,0x50);
    if(!ptr(v->clock)||w(v->clock,0x38)!=6)return 0;
    v->tick=w(v->clock,0x44);if(!v->tick||v->tick>=0x80000000u)return 0;
    v->order=q(v->clocks,0x58);v->course_owner=q(v->system,0x220);
    if(!ptr(v->order)||!ptr(v->course_owner)||q(v->driver,0x70)!=v->order||q(planner,0xe0)!=v->order)return 0;
    v->table=q(v->course_owner,0x190);if(!ptr(v->table))return 0;
    v->lap_number=w(v->table,0x14);v->course=q(v->table,0x100);
    if(v->lap_number>127||!ptr(v->course)||q(v->order,0x38)!=v->course||
       w(v->order,0x40)!=v->count||w(v->order,0x60)!=v->count||b(v->order,0x65)!=1)return 0;
    v->units=b(v->order,0x64);v->rows=q(v->order,0x48);
    if(!v->units||v->units>127||b(v->course,0x32)!=v->units||!ptr(v->rows))return 0;
    v->row=v->rows+0x78ull*v->racer;if(!ptr(v->row)||h(v->row,0x10)!=v->racer)return 0;
    v->phase=b(v->row,0x28);v->flags=h(v->row,0x12);v->raw=f(v->row,0x20);v->scale=f(v->course,0x2c);
    if(v->phase>=v->units||(v->flags&0x100)||!finite(v->raw)||!finite(v->scale)||v->scale<=0)return 0;
    if(v->lap_number>=2){
        float fraction=v->raw-(float)v->phase;
        if(v->units!=v->lap_number||!finite(fraction)||fraction<0||fraction>1)return 0;
        v->remaining=((float)v->units-v->raw)*v->scale;
    }else{
        v->boundary_count=w(v->course,0x40);v->boundaries=q(v->course,0x48);
        v->group_count=w(v->course,0x10);v->groups=q(v->course,0x20);
        if(v->boundary_count!=v->units+(v->lap_number==0)||!ptr(v->boundaries)||
           !v->group_count||v->group_count>4096||!ptr(v->groups))return 0;
        if(!endpoint(v,0,0)||!endpoint(v,v->units,1)||!endpoint(v,v->phase,2)||!endpoint(v,v->phase+1,3))return 0;
        float first=v->endpoints[0],last=v->endpoints[1],lo=v->endpoints[2],hi=v->endpoints[3];
        if(!(first<last&&lo<hi&&first<=lo&&hi<=last&&v->raw>=lo&&v->raw<=hi))return 0;
        v->remaining=(last-v->raw)*v->scale;
    }
    v->speed=f(v->physics,0x37c);v->normal_cap=f(v->physics,0x428);v->boost_cap=f(v->physics,0x430);
    if(!finite(v->remaining)||v->remaining<=0||!finite(v->speed)||!finite(v->normal_cap)||!finite(v->boost_cap)||
       v->speed<0||v->normal_cap<=0||v->boost_cap<0||v->speed>40||v->normal_cap>40||v->boost_cap>40)return 0;
    v->travel_speed=v->speed;
    if(v->normal_cap>v->travel_speed)v->travel_speed=v->normal_cap;
    if(v->boost_cap>v->travel_speed)v->travel_speed=v->boost_cap;
    return w(v->clock,0x38)==6&&w(v->clock,0x44)==v->tick;
}
EXPORT u32 lab_star_first_use_urgent(const void *planner){
    View a,z;
    if(!collect((u64)planner,&a)||a.remaining>a.travel_speed*(float)(450+30)||!collect((u64)planner,&z))return 0;
    for(u32 i=0;i<sizeof(a);i++)if(((const u8 *)&a)[i]!=((const u8 *)&z)[i])return 0;
    return 1;
}
