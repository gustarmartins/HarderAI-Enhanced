#include "item_task_collect.h"
typedef unsigned char byte;
static item_u64 q(item_u64 p,item_u32 off) {return *(const volatile item_u64 *)(p+off);}
static item_u32 w(item_u64 p,item_u32 off) {return *(const volatile item_u32 *)(p+off);}
static item_u32 b(item_u64 p,item_u32 off) {return *(const volatile byte *)(p+off);}
static int ptr(item_u64 p) {return p>=0x10000 && !(p&7) && p<(1ull<<39)-0x1000;}
static int read_roster(item_u64 base,item_u64 manager,item_u64 race,ItemTaskRoster *out) {
    if (!ptr(manager) || !ptr(race) || q(manager,0)!=base+0x11A7188 ||
        !b(manager,0x170) || q(race,0)!=base+0x11B4308) return 0;
    item_u64 group=q(race,0xB0),contexts=q(race,0xC8),items=q(manager,0xE8);
    item_u32 count=w(manager,0xE0),capacity=w(manager,0xE4);
    if (!count || count>12 || capacity<count || capacity>12 || count!=w(race,0xC0) ||
        !ptr(group) || !ptr(contexts) || !ptr(items) || q(group,0x28)!=race ||
        w(group,0x58)!=count) return 0;
    item_u64 drivers=q(group,0x60);if (!ptr(drivers)) return 0;
    item_u64 queue=manager+0x408,jobs=0;item_u32 threaded=b(base,0x11A71F8)!=0;
    if (threaded) {
        jobs=q(queue,0xB8);
        if (!ptr(jobs) || w(queue,0xC0)!=count || w(queue,0xB0)<count) return 0;
    }
    *out=(ItemTaskRoster){.base=base,.manager=manager,.race=race,.group=group,
                         .items=items,.jobs=jobs,.count=count,.threaded=threaded};
    item_u32 seen=0;
    for (item_u32 i=0;i<count;i++) {
        item_u64 item=q(items,8*i);if (!ptr(item) || q(item,0)!=base+0x11ACB70) return 0;
        item_u32 racer=w(item,0x40);
        if (racer>=count || (seen&(1u<<racer))) return 0;
        seen|=1u<<racer;
        item_u64 wrapper=q(item,0x48);if (!ptr(wrapper)) return 0;
        item_u64 role=q(wrapper,0x48);if (!ptr(role)) return 0;
        item_u64 ctx=q(role,8);if (!ptr(ctx)) return 0;
        item_u64 driver=q(ctx,0x18);
        if (!ptr(driver) || q(contexts,8*racer)!=ctx || q(drivers,8*racer)!=driver ||
            q(ctx,8)!=role || w(ctx,0x40)!=racer || w(role,0xA8)!=racer ||
            q(driver,0x60)!=group || q(race,0x50+8*racer)!=wrapper ||
            q(driver,0x98)!=item) return 0;
        /* Driver+88 is its own wrapper, distinct from the director's
         * per-racer item wrapper. The native item object is driver+98. */
        /* Real queue order is item-array order; racer order need not equal it. */
        if (threaded && (q(jobs,8*i)!=item+0x128 ||
            q(item,0x128)!=base+0x11ACBF0 || q(item,0x140)!=base+0x11ACC08 ||
            q(item,0x148)!=item || q(item,0x150)!=base+0x3ECA8 || q(item,0x158))) return 0;
        out->members[racer]=(ItemTaskMember){item,wrapper,ctx};
    }
    return 1;
}
int lab_item_task_collect(item_u64 base,void *manager,void *race,ItemTaskRoster *out) {
    if (!out || !base || (base&4095) || base>=(1ull<<39)-0x1400000) return 0;
    ItemTaskRoster first,again;
    if (!read_roster(base,(item_u64)manager,(item_u64)race,&first)) return 0;
    __asm__ volatile("dmb ishld" ::: "memory");
    if (!read_roster(base,(item_u64)manager,(item_u64)race,&again)) return 0;
    const item_u64 *a=(const item_u64 *)&first,*z=(const item_u64 *)&again;
    for (item_u32 i=0;i<sizeof(first)/sizeof(*a);i++) if (a[i]!=z[i]) return 0;
    volatile item_u64 *dest=(volatile item_u64 *)out;
    for (item_u32 i=0;i<sizeof(first)/sizeof(*a);i++) dest[i]=a[i];
    return 1;
}
