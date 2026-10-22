/* Private continuation trial. Follow a native-qualified landing surface when
 * the pending direction is already on that surface and body-up is settling.
 * Only local forecast vectors change. All callers retain route/width/intent
 * checks; native position, axes, charge and input fields are never written. */
EXPORT const u32 lab_snaking_current_surface_config[4]={2,4,5,1};

static u32 hcs_landed(const void *physics,const SnakeHopFrame *frame,HopVectors *v){
    GroundNativeAxis *history=nga_record(frame);
    if(!history||history->landing!=1||frame->valid!=1||frame->reserved!=1||
       frame->physics!=(u64)physics||!ptr(frame->drift)||
       q(physics,0x110)!=frame->drift||
       b(physics,0x200)!=1||b(physics,0x201)!=1||
       w(physics,0x26c)||f(physics,0x274)!=0.f||!hf_environment(physics)||
       !ngs_owned_history(physics,frame))return 0;
    u32 ds=w((void *)frame->drift,0x10);
    if((ds!=4&&ds!=5)||frame->pad!=ds)return 0;

    NativeAcceptedGroundTarget accepted;
    if(!ngs_native_ground_target(physics)&&
       !ngs_retained_ground_target(physics,frame)&&
       !ngs_accepted_ground_target(physics,frame,&accepted))return 0;

    /* Native contact projection can move both travel vectors to the target
     * before the interpolated body-up reaches it. Their forecast plane is
     * this verified surface; body heading still belongs to chassis-up. */
    float admission[3],chassis[3],heading[3];
    if(!hf_unit(frame,72,admission,.999f,1.001f)||
       !hf_unit(physics,0x14,chassis,.9f,1.1f)||
       !hf_unit(physics,0x20,heading,.9f,1.1f)||!hf_motion_plane(heading,chassis)||
       !hf_unit(physics,0x320,v->up,.999f,1.001f)||
       !hf_unit(physics,0x32c,v->target_up,.999f,1.001f)||
       !hf_unit(physics,0x2fc,v->motion,.9f,1.1f)||
       !hf_unit(physics,0x308,v->pending,.9f,1.1f)||
       !hf_motion_plane(v->motion,v->target_up)||
       !hf_motion_plane(v->pending,v->target_up))return 0;

    /* Reuse the established directional budget for the whole surface change
     * and the remaining orientation lag. A reversed or abrupt normal cannot
     * become a new forecast plane merely because the collision target exists. */
    float change=hf_dot(admission,v->target_up),lag=hf_dot(v->up,v->target_up);
    if(!finite(change)||!finite(lag)||change<=0.f||lag<=0.f||
       change*change<lab_snaking_config.alignment_squared||
       lag*lag<lab_snaking_config.alignment_squared)return 0;
    float motion[3],pending[3];
    if(!hrp_project(v->motion,v->target_up,motion)||
       !hrp_project(v->pending,v->target_up,pending))return 0;
    for(u32 i=0;i<3;i++){
        v->up[i]=v->target_up[i];
        v->motion[i]=motion[i];v->pending[i]=pending[i];
    }
    return 1;
}

static u32 hrp_landed(const void *physics,const SnakeHopFrame *frame,HopVectors *v){
    if(hrp_landed_prior(physics,frame,v))return 1;
    return hcs_landed(physics,frame,v);
}
