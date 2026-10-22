#ifndef HARDERAI_FLOW_LANE_OBSERVER_V305_H
#define HARDERAI_FLOW_LANE_OBSERVER_V305_H
typedef unsigned char fl_u8;
typedef unsigned int fl_u32;
typedef int fl_i32;
typedef unsigned long long fl_u64;
#define FL_EXPORT __attribute__((visibility("default")))
#define FL_RECORD_SIZE 256u
#define FL_CAPACITY 12u
/* Independent streams: kind0 final native target, kind1 native flow join.
 * generation is observer-local owner/clock-regression generation, not a native
 * race epoch. A host must bind its own current epoch and establish a baseline.
 * flow bits/mode are valid only when admitted==1; refusal never copies stale
 * physics flow fields. Query-attempt count cannot be inferred from this seam. */
typedef struct {
    fl_u32 lock,fault;
    fl_u64 sequence,events,dropped,generation;
    fl_u64 role,physics,driver,context,race,system,clock;
    fl_u64 route,current,following,response,native_sp;
    fl_u32 tick,racer,kind,admitted;
    fl_u32 response_flags,current_key,following_key,role_bytes;
    fl_u32 center_bits[3],side_bits[3],offset_bits,target_bits[3];
    fl_u32 flow_bits[3];fl_i32 mode;
    fl_u32 position_bits[3],speed_bits,drift_state,stage,reserved[2];
} FlowLaneRecord;
typedef struct {fl_u64 magic;fl_u32 abi,size,capacity,streams,target_site,target_return,flow_site,flow_return;} FlowLaneConfig;
_Static_assert(sizeof(FlowLaneRecord)==FL_RECORD_SIZE,"Flow lane record ABI");
_Static_assert(__builtin_offsetof(FlowLaneRecord,role)==40,"Flow owners");
_Static_assert(__builtin_offsetof(FlowLaneRecord,tick)==136,"Flow payload");
_Static_assert(__builtin_offsetof(FlowLaneRecord,flow_bits)==208,"Flow vector");
_Static_assert(sizeof(FlowLaneConfig)==40,"Flow configuration");
FL_EXPORT void lab_flow_lane_target_publish(void *route,fl_u32 x,fl_u32 y,fl_u32 z);
FL_EXPORT void lab_flow_lane_contact_publish(void *physics,const void *response,fl_u32 admitted,const void *native_sp);
#endif
