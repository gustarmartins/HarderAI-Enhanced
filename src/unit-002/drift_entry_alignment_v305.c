/* Private proposal: avoid committing a new CPU corner drift against the
 * current hop's accumulated steering sign. Called within native 5C360's actor
 * lifetime, after its opportunity gates, before its first commitment write.
 * No native state/input/charge/ratio writes. No ongoing drift is cancelled. */
typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long long u64;
#define EXPORT __attribute__((visibility("default")))
static u32 ptr(u64 p){return p>=0x10000ull&&p<(1ull<<39)-0x2000&&(p&7)==0;}
static u64 q(u64 p,u32 o){return *(volatile const u64 *)(p+o);}
static u32 w(u64 p,u32 o){return *(volatile const u32 *)(p+o);}
static u8 b(u64 p,u32 o){return *(volatile const u8 *)(p+o);}
static float f(u64 p,u32 o){return *(volatile const float *)(p+o);}
static u32 finite(float x){union{float f;u32 u;}v={.f=x};return (v.u&0x7f800000u)!=0x7f800000u;}
EXPORT const u32 lab_drift_entry_alignment_config[4]={2,0x5ca70,2,0x6c};

EXPORT u32 lab_drift_entry_alignment_defer(u64 producer){
    /* Outside an independently verified local CPU input chain leave native
     * admission untouched; this helper never grants a failed native request. */
    if(!ptr(producer)||b(producer,0x41))return 0;
    u64 driver=q(producer,0x70),packet=q(producer,0x98),source=q(producer,0x80);
    if(!ptr(driver)||!ptr(packet)||!ptr(source)||b(source,0x80)||q(source,0x58)!=driver)return 0;
    u32 flags=w(packet,4);
    /* This intervention owns only the R that the upcoming commitment would
     * add. A jump/drift already requested by another producer stays native. */
    if((flags&0x1bu)!=1u)return 0;
    u64 state=q(driver,0x50),adapter=q(driver,0x88);
    if(!ptr(state)||!ptr(adapter)||q(state,0x88)!=driver||q(state,0xc0)!=packet)return 0;
    u64 nav=q(state,0xb0);
    if(!ptr(nav)||q(nav,0x58)!=driver||q(nav,0x70)!=producer||
       q(nav,0x78)!=packet||q(nav,0x60)!=source)return 0;
    u64 role=q(adapter,0x48),physics=q(adapter,0x50);
    if(!ptr(role)||!ptr(physics)||q(role,0x28)!=physics||q(physics,0xf8)!=role||
       b(role,0xd2)!=1||b(role,0xd3)!=1||b(role,0xe1)!=1||b(role,0xe7)||
       (w(role,0x1cc)&0x01204018u))return 0;
    u64 drift=q(physics,0x110);
    if(!ptr(drift)||q(drift,8)!=role||w(drift,0x10)!=2)return 0;
    float accumulated=f(drift,0x6c),chosen=f(producer,0xa8),existing=f(packet,8);
    float requested=chosen+existing;
    if(!finite(accumulated)||!finite(chosen)||!finite(existing)||!finite(requested)||
       chosen==0.f||requested==0.f)return 0;
    /* Exact sign comparison avoids multiplication overflow/underflow. The
     * native hop itself continues and reevaluates its accumulator each tick. */
    u32 opposite=(accumulated<0.f&&requested>0.f)||(accumulated>0.f&&requested<0.f);
    if(!opposite)return 0;
    return q(producer,0x70)==driver&&q(producer,0x98)==packet&&q(producer,0x80)==source&&
           q(driver,0x50)==state&&q(driver,0x88)==adapter&&q(physics,0x110)==drift&&
           q(drift,8)==role&&w(drift,0x10)==2&&f(drift,0x6c)==accumulated&&
           f(producer,0xa8)==chosen&&f(packet,8)==existing&&w(packet,4)==flags&&
           b(role,0xd2)==1&&b(role,0xd3)==1&&b(role,0xe1)==1&&!b(role,0xe7)&&
           !b(source,0x80)&&!b(producer,0x41);
}
