#ifndef HARDERAI_DIRECTOR_PROMOTION_EXCHANGE_V305_H
#define HARDERAI_DIRECTOR_PROMOTION_EXCHANGE_V305_H
typedef unsigned int dx_u32;
typedef unsigned long long dx_u64;
/* A caller must own the qualified native MainThread group window. This helper
 * never acquires a scheduler lease, invents CPU ORDER or retains a pointer.
 * Every member/slot/backlink is checked before its six infallible stores. */
enum { DX_REFUSED=0,DX_EXCHANGED=1,DX_ALREADY_FRONT=2 };
typedef struct {
    dx_u32 status,source_racer,displaced_racer,source_slot,target_slot,source_group;
    dx_u64 group,manager,source_wrapper,displaced_wrapper;
} PromotionExchangeResult;
_Static_assert(sizeof(PromotionExchangeResult)==56,"Promotion exchange result ABI1");
dx_u32 lab_director_promotion_exchange_v305(dx_u64 base,void *group,dx_u32 source_racer,
    dx_u32 target_front_slot,PromotionExchangeResult *out);
#endif
