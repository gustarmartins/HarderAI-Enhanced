/* Private active-Star route trial. Select among existing priority9 successors
 * whose route to an ordinary exit fits the remaining Star estimate. Native
 * item-ready decisions and every nonselected case retain the retail chooser.
 * No native role, route point, inventory, timer, charge or speed is written. */
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed int i32;
typedef unsigned long long u64;
#define EXPORT __attribute__((visibility("default")))

EXPORT const struct {u32 abi,reserve_ticks,max_nodes,max_depth;float speed_fraction,max_speed;}
lab_star_shortcut_config={1,45,128,48,.75f,40.f};

static u64 q(const void *p,u32 o){return *(const u64 *)((const u8 *)p+o);}
static u32 w(const void *p,u32 o){return *(const u32 *)((const u8 *)p+o);}
static u16 h(const void *p,u32 o){return *(const u16 *)((const u8 *)p+o);}
static u8 b(const void *p,u32 o){return *((const u8 *)p+o);}
static float f(const void *p,u32 o){return *(const float *)((const u8 *)p+o);}
static u32 ptr(u64 p){return p>=0x10000 && !(p&7) && p<(1ull<<39)-0x2000;}
static u32 finite(float v){union{float f;u32 u;}x={.f=v};return (x.u&0x7fffffffu)<0x7f800000u;}
static float root(float v){float out;__asm__("fsqrt %s0,%s1":"=w"(out):"w"(v));return out;}

static void *point(void *course,u32 key){
    u32 group=key&0xffff,index=key>>16,n=w(course,0x18);
    u64 groups=q(course,0x20);
    if(!n||n>1024||group>=n||index==0xffff||!ptr(groups))return 0;
    void *row=(void *)(groups+48ull*group);u32 count=w(row,0x18);u64 array=q(row,0x20);
    if(!count||count>65535||index>=count||!ptr(array))return 0;
    u64 p=q((void *)array,8*index);
    return ptr(p)&&w((void *)p,0x8c)==key?(void *)p:0;
}

/* Mirrors7BE8B4: the selected end of the group supplies its route tag. */
static i32 tag(void *course,void *p){
    u32 group=w(p,0x8c)&0xffff;
    void *first=point(course,group);if(!first||b(first,0x2e4)>1)return -1;
    void *row=(void *)(q(course,0x20)+48ull*group);
    u32 count=w(row,0x10),capacity=w(row,0x18);
    if(!count||count>capacity||count>65535||w(row,0x14))return -1;
    void *end=b(first,0x2e4)?point(course,group|((count-1)<<16)):first;
    if(!end||h(end,0x362)==1)return -1;
    return h(end,0x2e2);
}

static u32 links(void *p,u32 *out){
    u32 a=w(p,0x30),z=w(p,0x34),reverse=b(p,0x2e4),both=b(p,0x370);
    if(a>10||z>10||reverse>1||both>1)return 0;
    u32 n=both?a+z:reverse?z:a;if(!n||n>10)return 0;
    for(u32 i=0;i<n;i++)out[i]=w(p,both?(i<a?0x38+4*i:0x60+4*(i-a)):(reverse?0x60:0x38)+4*i);
    return n;
}

static float distance(const void *a,u32 ao,const void *z,u32 zo){
    float total=0;
    for(u32 i=0;i<3;i++){
        float x=f(a,ao+4*i),y=f(z,zo+4*i);if(!finite(x)||!finite(y))return -1;
        float delta=y-x;total+=delta*delta;
    }
    return finite(total)?root(total):-1;
}

typedef struct {void *course;u32 used;u32 path[48];} Walk;

static float exit_distance(Walk *walk,u32 key,u32 depth){
    if(depth>=lab_star_shortcut_config.max_depth||++walk->used>lab_star_shortcut_config.max_nodes)return -1;
    for(u32 i=0;i<depth;i++)if(walk->path[i]==key)return -1;
    walk->path[depth]=key;
    void *p=point(walk->course,key);if(!p)return -1;
    u32 next[10],count=links(p,next);if(!count)return -1;
    float ordinary=-1;
    for(u32 i=0;i<count;i++){
        void *z=point(walk->course,next[i]);if(!z)return -1;
        i32 t=tag(walk->course,z);if(t<1||t>10)return -1;
        if(t<9){float d=distance(p,0xc,z,0xc);if(d<0)return -1;if(d>ordinary)ordinary=d;}
    }
    /* An ordinary exit ends this reservation. A later optional shortcut must
     * independently qualify at its own fork; do not reserve every chain. */
    if(ordinary>=0)return ordinary;
    float longest=0;
    for(u32 i=0;i<count;i++){
        void *z=point(walk->course,next[i]);
        float segment=distance(p,0xc,z,0xc),rest=exit_distance(walk,next[i],depth+1);
        if(segment<0||rest<0||!finite(segment+rest))return -1;
        if(segment+rest>longest)longest=segment+rest;
    }
    return longest;
}

EXPORT float lab_star_route_distance(void *course,u32 start,u32 choice){
    if(!ptr((u64)course))return -1;
    void *a=point(course,start),*z=point(course,choice);if(!a||!z||tag(course,z)!=9)return -1;
    u32 next[10],count=links(a,next),found=0;
    for(u32 i=0;i<count;i++)found|=next[i]==choice;
    if(!found)return -1;
    Walk walk={.course=course,.used=0};
    float first=distance(a,0xc,z,0xc),rest=exit_distance(&walk,choice,0);
    if(first<0||rest<0||!finite(first+rest))return -1;
    return first+rest;
}

EXPORT i32 lab_star_shortcut_select(void *route,void *info){
    if(!ptr((u64)route)||!ptr((u64)info)||b(info,0x14))return -1;
    u64 driver=q(route,0x40),holder=q(route,0x48);
    if(!ptr(driver)||holder!=driver+0x80)return -1;
    u64 course=q((void *)holder,0),adapter=q((void *)driver,0x88);
    if(!ptr(course)||!ptr(adapter))return -1;
    u64 role=q((void *)adapter,0x48),physics=q((void *)adapter,0x50);
    if(!ptr(role)||!ptr(physics)||q((void *)role,0x28)!=physics||q((void *)physics,0xf8)!=role||
       b((void *)role,0xd2)!=1||b((void *)role,0xd3)!=1||!b((void *)role,0xe1)||b((void *)role,0xe7)||
       (w((void *)role,0x1cc)&0x01204018u))return -1;
    i32 ticks=(i32)w((void *)role,0x1e4);
    if(ticks<=(i32)lab_star_shortcut_config.reserve_ticks)return -1;
    float speed=f((void *)physics,0x37c),base=f((void *)physics,0x420);
    if(!finite(speed)||!finite(base)||speed<=2||base<=2||speed>lab_star_shortcut_config.max_speed)return -1;
    if(base<speed)speed=base;
    float budget=(ticks-lab_star_shortcut_config.reserve_ticks)*speed*lab_star_shortcut_config.speed_fraction;
    u32 key=(u32)h(info,2)|((u32)h(info,4)<<16),choices[10],accepted[10],used=0;
    void *start=point((void *)course,key);if(!start)return -1;
    u32 count=links(start,choices);
    if(count<2||count!=w(info,0xc))return -1;
    float approach=distance((void *)physics,0x2c,start,0xc);if(approach<0)return -1;
    for(u32 i=0;i<count;i++){
        float length=lab_star_route_distance((void *)course,key,choices[i]);
        if(length>=0&&length+approach<=budget)accepted[used++]=i;
    }
    if(!used)return -1;
    u64 rng=q(route,0x38);if(!ptr(rng))return -1;
    u64 vt=q((void *)rng,0);if(!ptr(vt))return -1;
    u64 call=q((void *)vt,0xc8);if(call<0x10000||(call&3)||call>=(1ull<<39))return -1;
    u32 selected=((u32 (*)(void *,u32))call)((void *)rng,used);
    return selected<used?(i32)accepted[selected]:-1;
}
