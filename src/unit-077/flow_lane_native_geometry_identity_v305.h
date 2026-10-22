#ifndef LAB_FLOW_LANE_NATIVE_GEOMETRY_IDENTITY_V305_H
#define LAB_FLOW_LANE_NATIVE_GEOMETRY_IDENTITY_V305_H
#include "flow_lane_native_binding_mesh.h"
/* Compute the complete address-independent canonical geometry stream. Current
 * native mesh lifetime remains caller-owned. This primitive does not cache:
 * production caching additionally requires a real course-allocation epoch and
 * current immutable storage binding. It grants no route-write permission. */
unsigned lab_flow_lane_geometry_identity(const FlowLaneMesh *,unsigned char digest[32]);
unsigned lab_flow_lane_geometry_is_stock_bigblue(const FlowLaneMesh *,unsigned char digest[32]);
/* Standalone deterministic primitive for published SHA-256 test vectors. */
void lab_flow_lane_sha256(const unsigned char *,unsigned,unsigned char digest[32]);
#endif
