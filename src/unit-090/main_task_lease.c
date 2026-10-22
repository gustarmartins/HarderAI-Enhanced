/* Offline nonblocking lock experiment; not connected to control dispatch. */
#include "main_task_lease.h"
extern u32 lab_main_task_prerequisite(u64,void *);


__attribute__((visibility("default")))
void lab_main_task_unlock(MainTaskLease *lease) {
    if (!lease || !lease->base) return;
    void (*unlock)(void *)=(void *)(lease->base+LAB_NATIVE_UNLOCK_MUTEX);
    if (lease->tree_mutex) unlock((void *)lease->tree_mutex);
    if (lease->task_mutex) unlock((void *)lease->task_mutex);
    *lease=(MainTaskLease){0};
}

__attribute__((visibility("default")))
u32 lab_main_task_try_lock(u64 base,void *director,MainTaskLease *out) {
    if (!out) return 0;
    *out=(MainTaskLease){0};
    if (!lab_main_task_prerequisite(base,director)) return 0;
    u64 root=q(director,0x28),system=q((void *)root,0x30);
    u64 task=q((void *)system,0xB0),tree=q((void *)system,0x148);
    u32 (*try_lock)(void *)=(void *)(base+LAB_NATIVE_TRY_MUTEX);
    MainTaskLease lease={.base=base};
    /* Both are nonblocking: a teardown thread holding TaskMgr while waiting
     * for the already-running calc callback must not deadlock this hook. */
    if (!try_lock((void *)(task+0x20))) return 0;
    lease.task_mutex=task+0x20;
    if (!try_lock((void *)(tree+0x20))) {lab_main_task_unlock(&lease);return 0;}
    lease.tree_mutex=tree+0x20;
    if (!lab_main_task_prerequisite(base,director) ||
        q(director,0x28)!=root || q((void *)root,0x30)!=system ||
        q((void *)system,0xB0)!=task || q((void *)system,0x148)!=tree) {
        lab_main_task_unlock(&lease);return 0;
    }
    *out=lease;return 1;
}
