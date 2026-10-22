#ifndef HARDERAI_DIRECTOR_PROMOTION_RUNTIME_V305_H
#define HARDERAI_DIRECTOR_PROMOTION_RUNTIME_V305_H
#include "director_promotion_exchange_v305.h"
typedef struct {
    dx_u32 abi,enabled,first_tick,interval_ticks,retry_ticks,reserved[3];
} PromotionConfig;
typedef struct {
    dx_u32 lock,fault;
    dx_u64 sequence,updates,epoch,generation,group,clock;
    dx_u32 last_tick,next_tick,cycle,visited_mask,status,eligible_mask,exchange_tick,reserved;
    PromotionExchangeResult result;
} PromotionRecord;
_Static_assert(sizeof(PromotionConfig)==32,"Promotion config ABI1");
_Static_assert(sizeof(PromotionRecord)==144,"Promotion record ABI1");
void lab_director_promotion_dispatch(const void *);
#endif
