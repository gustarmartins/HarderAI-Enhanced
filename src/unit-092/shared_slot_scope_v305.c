#include "shared_slot_scope_v305.h"
static dx_u64 q(dx_u64 p,dx_u32 off){return *(const volatile dx_u64 *)(p+off);}
static dx_u32 w(dx_u64 p,dx_u32 off){return *(const volatile dx_u32 *)(p+off);}
static dx_u32 pointer(dx_u64 p){return p>=0x10000&&!(p&7)&&p<(1ull<<39)-0x4000;}
static dx_u32 zero(const SharedSlotLease *l){
    for(dx_u32 i=0;i<sizeof(*l)/8;i++)if(((const dx_u64 *)l)[i])return 0;
    return 1;
}
dx_u32 lab_shared_slot_begin(dx_u64 base,void *group,dx_u32 front_witness,dx_u32 mask,
    dx_u64 epoch,dx_u64 generation,SharedSlotLease *out){
    if(!out||!zero(out)||!epoch||!generation||!mask||mask>>12)return 0;
    dx_u32 n=0;for(dx_u32 i=0;i<12;i++)n+=(mask>>i)&1;
    if(n>2)return 0;
    /* An already-front witness executes the complete existing graph preflight
     * and returns without any native store. Never call with a non-front
     * witness: check that fact before invoking the six-store exchange helper. */
    dx_u64 g=(dx_u64)group;
    if(!pointer(g)||front_witness>=12)return 0;
    dx_u64 manager=q(g,0x68);if(!pointer(manager))return 0;
    dx_u64 table=q(manager,0xa0),front=q(manager,0x80);
    if(!pointer(table)||!pointer(front))return 0;
    dx_u64 witness=q(table,front_witness*8);
    if(!pointer(witness)||q(witness,0x10)!=front)return 0;
    PromotionExchangeResult checked;
    if(lab_director_promotion_exchange_v305(base,group,front_witness,0,&checked)!=DX_ALREADY_FRONT)return 0;
    SharedSlotLease lease={.count=n,.epoch=epoch,.generation=generation,.group=g,.manager=manager};
    dx_u32 used=0;
    for(dx_u32 i=0;i<12;i++)if(mask&(1u<<i)){
        dx_u64 wr=q(table,8*i),driver=q(wr,8),sub=q(wr,0x10);dx_u32 slot=w(wr,0x24);
        if(!sub||sub==front||slot<3||slot>=12||wr==q(manager,0x168)||wr==q(manager,0x170))return 0;
        lease.cells[used++]=(SharedSlotCell){wr,driver,i,slot,0,0};
    }
    for(dx_u32 i=0;i<used;i++)*(volatile dx_u32 *)(lease.cells[i].wrapper+0x24)=0;
    __atomic_thread_fence(__ATOMIC_RELEASE);
    lease.active=1;*out=lease;return 1;
}
dx_u32 lab_shared_slot_end(void *group,dx_u64 epoch,dx_u64 generation,SharedSlotLease *lease){
    if(!lease||lease->active!=1||!lease->count||lease->count>2||lease->group!=(dx_u64)group||
       lease->epoch!=epoch||lease->generation!=generation||!pointer(lease->group)||!pointer(lease->manager))return 0;
    /* Caller still owns its original allocation pin even if identity admission
     * or an actor's active flag closed while workers ran. Do not require a
     * fresh gameplay admission here or abandon an owned cleanup obligation. */
    dx_u64 manager=lease->manager;
    if(q(lease->group,0x68)!=manager||q(manager,0x28)!=lease->group)return 0;
    dx_u64 table=q(manager,0xa0);if(!pointer(table))return 0;
    for(dx_u32 i=0;i<lease->count;i++){
        const SharedSlotCell *c=&lease->cells[i];
        if(c->racer>=12||c->reserved||c->previous<3||c->previous>=12||c->assigned||
           !pointer(c->wrapper)||!pointer(c->driver))return 0;
    }
    dx_u32 unresolved=0;
    for(dx_u32 i=0;i<lease->count;i++){
        const SharedSlotCell *c=&lease->cells[i];dx_u32 expected=c->assigned;
        if(q(table,8*c->racer)!=c->wrapper||q(c->wrapper,8)!=c->driver||q(c->driver,0x60)!=lease->group){
            /* Different owners do not prove somebody replaced our assigned
             * value. Keep the dirty obligation distinct from a value conflict. */
            lease->conflict_mask|=1u<<c->racer;unresolved=1;
        }else if(lease->restored_mask&(1u<<c->racer)){
            /* An earlier partial cleanup already restored this owned cell. */
        }else if(!__atomic_compare_exchange_n((dx_u32 *)(c->wrapper+0x24),&expected,c->previous,0,
                                              __ATOMIC_RELEASE,__ATOMIC_RELAXED)){
            lease->conflict_mask|=1u<<c->racer;
        }else lease->restored_mask|=1u<<c->racer;
    }
    if(unresolved)return 0;
    lease->active=0;return lease->conflict_mask?2:1;
}
