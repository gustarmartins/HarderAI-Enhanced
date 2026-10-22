/* Private proposal: evaluate a qualified airborne hop in its admitted road
 * plane. The native world-up orientation blend can tilt travel slightly away
 * from that plane. Only local forecast vectors change; physical state does not.
 */
EXPORT const u32 lab_snaking_hop_road_projection_config[4]={2,11,19,1};

static u32 hrp_project(const float *axis,const float *normal,float *out){
    float height=hf_dot(axis,normal),size=0.f;
    if(!finite(height))return 0;
    for(u32 i=0;i<3;i++){
        out[i]=axis[i]-height*normal[i];
        if(!finite(out[i]))return 0;
        size+=out[i]*out[i];
    }
    /* Keep the established directional budget. Normalizing this projection
     * forecasts at least as far along the road as the physical vector would.
     * Native actual/pending axes still must be tangent to the actual normal.
     */
    if(!finite(size)||size<lab_snaking_config.alignment_squared||size>1.001f)return 0;
    size=root(size);
    for(u32 i=0;i<3;i++)out[i]/=size;
    return 1;
}

static u32 hrp_vectors(const void *physics,const SnakeHopFrame *frame,HopVectors *v){
    /* Called only inside har's branch-qualified air11..19 fallback. Retain
     * exact consecutive native-normal history, not just a plausible angle.
     */
    if(!ngs_owned_history(physics,frame)||
       !hf_motion_plane(v->motion,v->up)||!hf_motion_plane(v->pending,v->up))return 0;
    float motion[3],pending[3];
    if(!hrp_project(v->motion,v->target_up,motion)||
       !hrp_project(v->pending,v->target_up,pending))return 0;
    for(u32 i=0;i<3;i++){v->motion[i]=motion[i];v->pending[i]=pending[i];}
    return 1;
}

static u32 hrp_landed_prior(const void *physics,const SnakeHopFrame *frame,HopVectors *v){
    /* Native contact may project pending motion onto the road before actual
     * orientation has settled. Admit only the existing exact, consecutive
     * landing-history chain, on the same originally admitted road plane.
     */
    GroundNativeAxis *history=nga_record(frame);
    if(!history||history->landing!=1||b(physics,0x200)!=1||b(physics,0x201)!=1||
       w(physics,0x26c)||f(physics,0x274)!=0.f||!hf_environment(physics)||
       !ngs_owned_history(physics,frame))return 0;
    float road[3],chassis[3],heading[3];
    if(!hf_unit(frame,72,road,.999f,1.001f)||
       !hf_unit(physics,0x14,chassis,.9f,1.1f)||
       !hf_unit(physics,0x20,heading,.9f,1.1f)||!hf_motion_plane(heading,chassis)||
       !hf_unit(physics,0x320,v->up,.999f,1.001f)||
       !hf_unit(physics,0x32c,v->target_up,.999f,1.001f)||
       !hf_unit(physics,0x2fc,v->motion,.9f,1.1f)||
       !hf_unit(physics,0x308,v->pending,.9f,1.1f)||
       !hf_same_plane(v->target_up,road)||!hf_motion_plane(v->pending,v->target_up))return 0;
    float alignment=hf_dot(v->up,road),motion[3],pending[3];
    if(!finite(alignment)||alignment<=0.f||
       alignment*alignment<lab_snaking_config.alignment_squared||
       !hrp_project(v->motion,road,motion)||!hrp_project(v->pending,road,pending))return 0;
    for(u32 i=0;i<3;i++){
        v->up[i]=v->target_up[i]=road[i];v->motion[i]=motion[i];v->pending[i]=pending[i];
    }
    return 1;
}

#include "snaking_current_surface_v305.h"
