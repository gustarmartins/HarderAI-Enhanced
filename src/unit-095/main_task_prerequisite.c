/* Read-only task graph prerequisite. Does not acquire a lock or lease memory. */
#include "native_admission.h"
static int valid(u64 p) {return p>=0x10000 && !(p&7) && p<(1ull<<39)-0x1000;}
static u64 get(u64 p) {return *(volatile u64 *)p;}
static u32 word(u64 p) {return *(volatile u32 *)p;}

__attribute__((visibility("default")))
u32 lab_main_task_prerequisite(u64 base,void *director) {
    u64 race=(u64)director;
    if (!valid(race)) return 0;
    u64 embedded=get(race+0x28);
    if (!valid(embedded)) return 0;
    u64 system=get(embedded+0x30);
    if (!valid(system) || embedded!=system+0x178 ||
        get(system)!=base+LAB_RACE_SYSTEM_TABLE || get(system+0x238)!=race ||
        word(system+0xB8)!=4) return 0;
    u64 task=get(system+0xB0);
    if (!valid(task)) return 0;
    u64 frame=get(task+0x40);
    if (!valid(frame)) return 0;
    u64 table=get(frame);
    if (table!=base+LAB_FRAME_TABLE_A && table!=base+LAB_FRAME_TABLE_B) return 0;
    if (get(frame+0x38)!=task) return 0;
    u64 manager=get(frame+0x40);
    if (!valid(manager)) return 0;
    u64 mutex=manager+8;
    if (get(system+0x148)!=mutex || get(manager+0xC0)!=mutex) return 0;
    __asm__ volatile("dmb ishld" ::: "memory");
    return get(race+0x28)==embedded && get(embedded+0x30)==system &&
        get(system)==base+LAB_RACE_SYSTEM_TABLE && get(system+0x238)==race &&
        word(system+0xB8)==4 && get(system+0xB0)==task && get(task+0x40)==frame &&
        get(frame)==table && get(frame+0x38)==task && get(frame+0x40)==manager &&
        get(system+0x148)==mutex && get(manager+0xC0)==mutex;
}
