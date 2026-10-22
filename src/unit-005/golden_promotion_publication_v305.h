#ifndef LAB_GOLDEN_PROMOTION_PUBLICATION_V305_H
#define LAB_GOLDEN_PROMOTION_PUBLICATION_V305_H
/* Private scalar publication between the main item boundary and AI workers.
 * Tokens are opaque. The caller owns native lifetime; no pointer is followed.
 * Every shared payload field uses atomic access, including failed reads. */
typedef unsigned int gpp_u32;
typedef unsigned long long gpp_u64;
typedef struct {
    gpp_u64 driver,role,inventory;
    gpp_u32 strength_bits,reserved;
} GoldenPromotionCell;
typedef struct {
    gpp_u64 epoch,generation;
    gpp_u32 tick,mask;
    GoldenPromotionCell actors[12];
} GoldenPromotionView;
typedef struct {
    gpp_u64 sequence;
    gpp_u32 writer,fault;
    GoldenPromotionView value;
} GoldenPromotionPublication;
/* One main-thread writer. Input is private immutable caller storage and must
 * not overlap publication storage. CAS detects accidental competing writers. */
gpp_u32 lab_golden_promotion_publish(GoldenPromotionPublication *,const GoldenPromotionView *);
void lab_golden_promotion_unpublish(GoldenPromotionPublication *);
/* Current actor tokens/clock are supplied by the existing director boundary.
 * Success is a coherent <=2-tick plan, including a valid zero-strength plan. */
gpp_u32 lab_golden_promotion_read(const GoldenPromotionPublication *,gpp_u32 racer,
    gpp_u64 epoch,gpp_u64 generation,gpp_u64 driver,gpp_u64 role,gpp_u64 inventory,
    gpp_u32 tick,float *strength);
#endif
