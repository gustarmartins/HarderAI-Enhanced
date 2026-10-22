/* Private experimental post-selector filter for native 3.0.5 course views.
 * Returning a paired native course is an experimental line-choice change.
 * No game writes, allocation, selector calls, F.SLOT or ratio manipulation.
 * Native construction/collision processing remains responsible for the views.
 */
typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long long u64;
#define EXPORT __attribute__((visibility("default")))
static u64 q(const void*p,u32 o){return *(const u64*)((const u8*)p+o);}
static u32 w(const void*p,u32 o){return *(const u32*)((const u8*)p+o);}
static u8 b(const void*p,u32 o){return *((const u8*)p+o);}
static u32 ptr(u64 p){return p>=0x10000 && !(p&7) && p<(1ull<<39)-0x1000;}

EXPORT void *lab_cpu_paired_route_view(void *driver,void *original,void *manager,u64 base){
    if(!ptr((u64)driver)||!ptr((u64)original)||!ptr((u64)manager)||
       b(driver,0xc4)||b(driver,0xad)||b(driver,0xae)||b(driver,0xaf))return original;
    u64 group=q(driver,0x60),adapter=q(driver,0x88),vt=q(driver,0);
    if(!ptr(group)||!ptr(adapter)||!ptr(vt)||q((void*)group,0x50)!=(u64)manager||
       q(manager,0x38)!=group||q((void*)vt,0x188)!=base+0x71ea4)return original;
    u64 race=q((void*)group,0x28),role=q((void*)adapter,0x48),physics=q((void*)adapter,0x50);
    if(!ptr(race)||q((void*)race,0xb0)!=group||!ptr(role)||!ptr(physics)||
       q((void*)role,0x28)!=physics||q((void*)physics,0xf8)!=role||
       b((void*)role,0xd0)||b((void*)role,0xd2)!=1||b((void*)role,0xd3)!=1||
       b((void*)role,0xe7)||w((void*)role,0xa8)>=12)return original;
    u64 ai=q((void*)role,0x18);
    if(!ptr(ai)||q(driver,0x90)!=ai||q((void*)ai,0x10)!=role||
       q((void*)physics,0x100)!=ai)return original;
    /* Reuse verified native188 semantics. The selector has already executed;
     * this chooses its same array rather than repeating formation/count logic. */
    u32 alt=b(driver,0xd0)!=0 && q(manager,0x59a8)!=0;
    const void *head=(const u8*)manager+(alt?0xb0:0x40);
    if(w(head,0)!=12||w(head,4)!=12)return original;
    u64 array=q(head,8);if(!ptr(array))return original;
    u64 views[12];u32 match=12;
    for(u32 i=0;i<12;i++){
        views[i]=q((void*)array,8*i);if(!ptr(views[i]))return original;
        if(views[i]==(u64)original){if(match!=12)return original;match=i;}
    }
    /* Base0/11 intentionally alias and are retained above. All middle views
     * must be distinct; no ambiguous pointer identity is changed. */
    if(views[0]!=views[11]||match<6||match>10)return original;
    for(u32 i=1;i<=10;i++){
        if(views[i]==views[0])return original;
        for(u32 j=1;j<i;j++)if(views[i]==views[j])return original;
    }
    const void *paired=(void*)views[match-5];
    /* Known cloned-course shape/variant only; these checks are not clearance. */
    u32 groups=w(original,0x10);
    if(!groups||groups>100||w(paired,0x10)!=groups||
       b(original,0x4c)!=alt||b(paired,0x4c)!=alt||
       !ptr(q(original,0x20))||!ptr(q(paired,0x20)))return original;
    return (void*)paired;
}
