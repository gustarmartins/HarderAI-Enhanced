/* Inactive laboratory ABI. Both stages run on the same native game thread. */
#ifndef LAB_MANUAL_INTENT_H
#define LAB_MANUAL_INTENT_H
typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef struct { u32 word[9]; } Packet;
typedef struct { u32 racer, flags_mask, words_mask, enabled; } IntentPolicy;
typedef struct {
    u64 context, driver, role;
    u32 phase, outcome;
    Packet native_packet, manual_packet;
    u32 flags_mask, words_mask;
    u64 state, controller;
    int tick;
    u32 reserved;
} IntentTrace;
_Static_assert(sizeof(Packet)==36, "Native input packet ABI");
_Static_assert(sizeof(IntentTrace)==136, "Laboratory intent trace ABI");
typedef void (*HumanProducer)(void *, Packet *);
static inline u64 q(const void *p, u32 off) { return *(const u64 *)((const u8 *)p+off); }
static inline u32 w(const void *p, u32 off) { return *(const u32 *)((const u8 *)p+off); }
static inline u8 b(const void *p, u32 off) { return *((const u8 *)p+off); }
#endif
