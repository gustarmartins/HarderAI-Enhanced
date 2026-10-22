#include "flow_lane_native_flow_query_v305.h"
typedef fln_u64 u64;
typedef fln_u32 u32;
typedef unsigned char u8;
typedef struct {
    u64 root,single,link,resource,common,areas,area_table,paths,path_rows;
    u32 area_count,path_count;
} Course;
static u64 q(u64 p,u32 o){return *(const volatile u64 *)(p+o);}
static u32 w(u64 p,u32 o){return *(const volatile u32 *)(p+o);}
static unsigned short h(u64 p,u32 o){return *(const volatile unsigned short *)(p+o);}
static u8 b(u64 p,u32 o){return *(const volatile u8 *)(p+o);}
static float f(u64 p,u32 o){return *(const volatile float *)(p+o);}
static u32 span(u64 p,u32 n){return p>=0x10000&&!(p&7)&&p<((1ull<<39)-n);}
static u32 finite(float v){union{float f;u32 u;}x={v};return (x.u&0x7fffffffu)<0x7f800000u;}
static u32 number(float v){return finite(v)&&v>=-1.e8f&&v<=1.e8f;}
static void clear(FlowLaneFlowResult *o,u32 status){
    o->status=status;o->accepted=0;o->mode=-1;o->reserved=0;
    for(u32 k=0;k<3;k++)o->vector[k]=0;o->reserved2=0;
}
static u32 frame(const FlowLaneNativeFrame *s,Course *c){
    if(!s||(s->main_base&4095)||s->main_base>(1ull<<39)-0x1400000||s->racer>=12)return 0;
    const u64 objects[]={s->route,s->driver,s->physics,s->role,s->context,
        s->source,s->request,s->state,s->nav,s->producer,s->scene,s->system,
        s->race,s->group,s->manager,s->clock,s->selected,s->base_course,
        s->current,s->following,s->static_wrapper,s->dispatcher,s->course_data};
    for(u32 k=0;k<sizeof(objects)/sizeof(objects[0]);k++)if(!span(objects[k],0x1800))return 0;
    if(q(s->route,0x40)!=s->driver||q(s->route,0x58)!=s->current||q(s->route,0x50)!=s->following||
       w(s->route,0x98)!=s->offset_bits||q(s->driver,0x80)!=s->selected||
       q(s->source,0x58)!=s->driver||q(s->source,0x60)!=s->route||w(s->source,0x84)!=1||
       b(s->source,0x80)||b(s->request,0)||q(s->driver,0x50)!=s->state||
       q(s->state,0xb0)!=s->nav||q(s->nav,0x60)!=s->source||q(s->nav,0x70)!=s->producer||
       q(s->producer,0x70)!=s->driver||q(s->producer,0x80)!=s->source||q(s->producer,0x88)!=s->route||
       b(s->producer,0x41)||q(s->role,0x28)!=s->physics||q(s->physics,0xf8)!=s->role||
       q(s->role,8)!=s->context||q(s->context,8)!=s->role||q(s->context,0x18)!=s->driver||
       w(s->context,0x40)!=s->racer||w(s->role,0xa8)!=s->racer||w(s->role,0xd0)!=s->role_bytes||
       b(s->role,0xd0)||b(s->role,0xd2)!=1||b(s->role,0xd3)!=1||b(s->role,0xe1)!=1||b(s->role,0xe7)||
       !b(s->physics,0x200)||!b(s->physics,0x201)||q(s->driver,0x60)!=s->group||
       q(s->group,0x28)!=s->race||q(s->group,0x68)!=s->manager||q(s->manager,0x28)!=s->group||
       q(s->race,0xb0)!=s->group||w(s->race,0x790)!=2||
       w(s->clock,0x38)!=6||w(s->clock,0x44)!=s->tick||
       w(s->current,0x8c)!=s->proposal.current_key||w(s->following,0x8c)!=s->proposal.following_key||
       q(s->dispatcher,0xa8)!=s->static_wrapper||q(s->static_wrapper,0x370)!=s->course_data)return 0;
    c->root=q(s->main_base,0x12f6388);if(!span(c->root,8))return 0;
    c->single=q(c->root,0);if(!span(c->single,0x28))return 0;
    c->link=q(c->single,0x20);if(!span(c->link,0x10)||q(c->link,8)!=s->scene)return 0;
    if(!(b(s->scene,0x180)&4)||q(s->scene,0x1b0)!=s->system||q(s->system,0x238)!=s->race)return 0;
    u64 clocks=q(s->system,0x218);if(!span(clocks,0x58)||q(clocks,0x50)!=s->clock)return 0;
    c->resource=q(s->system,0x220);if(!span(c->resource,0x198))return 0;
    c->common=q(c->resource,0x190);if(!span(c->common,0x128)||q(c->common,0xe8)!=s->base_course)return 0;
    c->areas=q(c->common,0x88);c->paths=q(c->common,0x120);
    if(!span(c->areas,0x40)||!span(c->paths,0x28))return 0;
    c->area_count=w(c->areas,0x30);c->area_table=q(c->areas,0x38);
    c->path_count=w(c->paths,0x10);c->path_rows=q(c->paths,0x20);
    return 1;
}
static u32 same(const Course *a,const Course *b){
    return a->root==b->root&&a->single==b->single&&a->link==b->link&&
        a->resource==b->resource&&a->common==b->common&&a->areas==b->areas&&
        a->area_table==b->area_table&&a->paths==b->paths&&a->path_rows==b->path_rows&&
        a->area_count==b->area_count&&a->path_count==b->path_count;
}
static u32 course(const FlowLaneNativeFrame *s,const Course *c){
    if(c->area_count>128||c->path_count>64||
       (c->area_count&&!span(c->area_table,8*c->area_count))||
       (c->path_count&&!span(c->path_rows,32*c->path_count)))return 0;
    u32 total=0;
    for(u32 j=0;j<c->path_count;j++){
        u64 row=c->path_rows+32*j;u32 count=w(row,0x10);u64 pts=q(row,0x18);
        if(count<2||count>256||total+count>2048||!span(pts,0x68*count))return 0;total+=count;
        for(u32 k=0;k<count;k++){
            u64 p=pts+0x68*k;u32 nc=w(p,0x40);u64 next=q(p,0x48);
            if(nc!=(k+1<count?1u:0u))return 0;
            if(nc&&(!span(next,8)||q(next,0)!=p+0x68))return 0;
            for(u32 o=8;o<=0x3c;o+=4)if(!number(f(p,o)))return 0;
            if(f(p,0x38)<0||f(p,0x3c)<0||!number(f(p,0x60))||w(p,0x64)>1)return 0;
            if(nc){float d2=0;for(u32 a=0;a<3;a++){float d=f(p+0x68,8+4*a)-f(p,8+4*a);d2+=d*d;}
                if(!finite(d2)||d2<=1.e-8f)return 0;}
        }
    }
    for(u32 j=0;j<c->area_count;j++){
        u64 a=q(c->area_table,8*j);if(!span(a,0x60))return 0;
        u32 shape=w(a,0xc);if(shape>1||w(a,0x10)!=3||h(a,0x38)>=c->path_count)return 0;
        u64 p=q(a,0x58);if(!span(p,0x60))return 0;u64 vt=q(p,0);if(!span(vt,16))return 0;
        if(q(vt,0)!=s->main_base+(shape?0x808258:0x808134)||
           q(vt,8)!=s->main_base+(shape?0x8082a0:0x808188)||q(p,0x38)!=p+8||q(p,0x40)!=p+0x2c)return 0;
        for(u32 o=8;o<=0x34;o+=4)if(!number(f(p,o)))return 0;
        for(u32 o=0x48;o<=(shape?0x5c:0x54);o+=4)if(!number(f(p,o))||f(p,o)<0)return 0;
    }
    /* Retail no-match branch copies from this immutable global vector. */
    u64 zero=q(s->main_base,0x12f5148);
    if(zero<0x10000||(zero&3)||zero>=(1ull<<39)-12)return 0;
    for(u32 k=0;k<3;k++)if(f(zero,4*k)!=0)return 0;
    return 1;
}
u32 lab_flow_lane_native_flow_query(const FlowLaneNativeFrame *s,const float position[3],FlowLaneFlowResult *o){
    if(!o)return FLF_INVALID;clear(o,FLF_INVALID);
    if(!s||!position)return FLF_INVALID;
    float input[3];for(u32 k=0;k<3;k++){input[k]=position[k];if(!number(input[k]))return FLF_INVALID;}
    Course before,after;if(!frame(s,&before))return FLF_INVALID;
    if(!course(s,&before)){clear(o,FLF_COURSE);return FLF_COURSE;}
    float vector[3]={0,0,0};int mode=-1;
    typedef u32 (*Native)(float *,const float *,int *);
    u32 accepted=((Native)(s->main_base+0x80c65c))(vector,input,&mode);
    if(!frame(s,&after)||!same(&before,&after)||!course(s,&after)){clear(o,FLF_CHANGED);return FLF_CHANGED;}
    if(accepted>1||(accepted&&(mode<0||mode>1))||(!accepted&&mode!=-1)){
        clear(o,FLF_OUTPUT);return FLF_OUTPUT;
    }
    for(u32 k=0;k<3;k++)if(!number(vector[k])||(!accepted&&vector[k]!=0)){
        clear(o,FLF_OUTPUT);return FLF_OUTPUT;
    }
    o->accepted=accepted;o->mode=mode;for(u32 k=0;k<3;k++)o->vector[k]=vector[k];
    o->status=FLF_EVALUATED;return FLF_EVALUATED;
}

#include "flow_lane_sealed_query_impl_v305.h"

#include "flow_lane_owned_query_impl_v305.h"
