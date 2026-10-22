#ifndef LAB_FLOW_LANE_NATIVE_BINDING_GEOMETRY_H
#define LAB_FLOW_LANE_NATIVE_BINDING_GEOMETRY_H
/* Continuous geometry on a borrowed native prism reconstructed as a triangle.
 * No game access, material reinterpretation or sampled-sphere clearance. */
typedef struct { float vertex[3][3]; } FlowLaneTriangle;
/* Zero on invalid/degenerate/nonfinite input. Otherwise writes exact geometric
 * squared minimum (up to floating-point arithmetic) over the whole segment. */
unsigned lab_flow_lane_segment_triangle(const float a[3],const float b[3],
                                        const FlowLaneTriangle *,double *distance2);
/* Clip the line's projection along up into one triangle and +/-height from its
 * plane. Returned interval is geometric support, not native contact admission.
 * 0=no interval,1=interval,2=invalid. Caller must cover the entire path and bind
 * eligible floor materials/continuous topology before treating it as support. */
unsigned lab_flow_lane_triangle_support(const float a[3],const float b[3],
    const FlowLaneTriangle *,const float up[3],float height,double interval[2]);
#endif
