/* Generated from exact pinned helper bodies. */
static u32 (*const hvr_original_vectors)(const void *,const SnakeHopFrame *,HopVectors *) __attribute__((unused))=har_vectors;
static u32 hvr_unit(const void *p,u32 offset,float *v,float low,float high,HvrFailure *failure,u32 site){
    float n=0;
    for(u32 i=0;i<3;i++){v[i]=f(p,offset+4*i);if(!finite(v[i]))return hvr_fail(failure,site,1,i,&v[i],1);n+=v[i]*v[i];}
    if(!finite(n)||n<low||n>high)return hvr_fail(failure,site,2,0xffffffffu,(const float[]){n,low,high},3);
    n=root(n);for(u32 i=0;i<3;i++)v[i]/=n;
    return 1;
}

static u32 hvr_same_plane(const float *a,const float *z,HvrFailure *p,u32 site){
    float d=hf_dot(a,z);
    u32 accepted=finite(d)&&d>0&&d*d>=1.f-lab_snaking_hop_frame_config.plane_squared;
    if(!accepted)return hvr_fail(p,site,4,0xffffffffu,&d,1);
    return accepted;
}

static u32 hvr_motion_plane(const float *axis,const float *normal,HvrFailure *p,u32 site){
    float d=hf_dot(axis,normal);
    u32 accepted=finite(d)&&d*d<=lab_snaking_hop_frame_config.plane_squared;
    if(!accepted)return hvr_fail(p,site,3,0xffffffffu,&d,1);
    return accepted;
}

static u32 hvr_environment(const void *physics,HvrFailure *p,u32 site){
    u32 accepted=hf_environment(physics);
    if(!accepted)return hvr_group(p,site);
    return accepted;
}

static u32 hvr_ground_axis(const void *physics,const SnakeHopFrame *frame,
                           const float *actual,const float *admission,HvrFailure *failure,u32 site){
    if(nga_axis(physics,frame,actual,admission))return 1;
    return hvr_same_plane(actual,admission,failure,site);
}
static u32 hvr_surface_target(const void *physics,const SnakeHopFrame *frame,
    const float *target,const float *admission,HvrFailure *failure,u32 site){
    if(ngs_target_axis(physics,frame,target,admission))return 1;
    return hvr_same_plane(target,admission,failure,site);
}
static u32 hvr_vectors(const void *physics,const SnakeHopFrame *frame,HopVectors *v,HvrProbe *p){
    float chassis_up[3],chassis_heading[3],admission_up[3];
    if(!hvr_environment(physics,&p->primary,1)||
       !hvr_unit(frame,72,admission_up,.999f,1.001f,&p->primary,2)||
       !hvr_unit(physics,0x14,chassis_up,.9f,1.1f,&p->primary,3)||
       !hvr_unit(physics,0x20,chassis_heading,.9f,1.1f,&p->primary,4)||
       !hvr_motion_plane(chassis_heading,chassis_up,&p->primary,5)||
       !hvr_unit(physics,0x320,v->up,.999f,1.001f,&p->primary,6)||
       !hvr_unit(physics,0x32c,v->target_up,.999f,1.001f,&p->primary,7)||
       !hvr_unit(physics,0x2fc,v->motion,.9f,1.1f,&p->primary,8)||
       !hvr_unit(physics,0x308,v->pending,.9f,1.1f,&p->primary,9)||
       !hvr_ground_axis(physics,frame,v->up,admission_up,&p->primary,10)||
       !hvr_surface_target(physics,frame,v->target_up,admission_up,&p->primary,11))return 0;
    return hvr_motion_plane(v->motion,v->up,&p->primary,12)&&hvr_motion_plane(v->motion,v->target_up,&p->primary,13)&&
           hvr_motion_plane(v->pending,v->up,&p->primary,14)&&hvr_motion_plane(v->pending,v->target_up,&p->primary,15);
}

static u32 hvr_air_vectors(const void *physics,const SnakeHopFrame *frame,HopVectors *v,HvrProbe *p){
    if(hvr_vectors(physics,frame,v,p))return 1;
    if(hrp_landed(physics,frame,v))return 1;
    /* Caller hf_scope also checks continuous ownership/tick, phase, current
     * course/base identity and lifetime. No exception survives contact or ds4/5. */
    if(frame->valid!=1||frame->physics!=(u64)physics||
       q(physics,0x110)!=frame->drift||!ptr(frame->drift)||
       w((void *)frame->drift,0x10)!=2||!nth_air_contact(physics,frame)||
       !hvr_environment(physics,&p->fallback,102))return hvr_group(&p->fallback,101);
    u32 air=w(physics,0x26c),elapsed=frame->last-frame->start;
    if(air<11||air>19||elapsed>30||air>elapsed||b(physics,0x27b)>1||
       f(physics,0x274)!=.05f*(float)air)return hvr_group(&p->fallback,103);
    u64 role=q(physics,0xf8);if(!ptr(role)||q((void *)role,0x28)!=(u64)physics)return hvr_group(&p->fallback,104);
    u32 flags=w((void *)role,0x1cc),mode=w(physics,0x230);
    /* Bit21 selects another gravity branch; bit4 changes the normal path.
     * Ordinary native world-up reset: material2, or no bit12 hold. The outer
     * native-intent gate retains all its other special-state exclusions. */
    if((flags&0x00200010u)||mode>2)return hvr_group(&p->fallback,105);
    if(b(physics,0x27b)==0){
        if((mode!=2&&(flags&0x1000u))||w(physics,0x32c)||
           w(physics,0x330)!=0x3f800000u||w(physics,0x334))return hvr_group(&p->fallback,106);
    }else{
        /* Native selector1 negates gravity27c into target32c after air>=11.
         * Only exact world-down gravity is supported here. Keep signed-zero
         * correspondence, so this cannot admit another gravity direction or
         * an independently mismatching world-up target. Bit12 does not gate
         * this native selector1 branch (retail182a8c..182aac).
         */
        if(!lsg_hop_target(physics,frame))return hvr_group(&p->fallback,107);
    }
    float chassis_up[3],heading[3];
    if(!hvr_unit(frame,72,v->target_up,.999f,1.001f,&p->fallback,108)||
       !hvr_unit(physics,0x14,chassis_up,.9f,1.1f,&p->fallback,109)||
       !hvr_unit(physics,0x20,heading,.9f,1.1f,&p->fallback,110)||!hvr_motion_plane(heading,chassis_up,&p->fallback,111)||
       !hvr_unit(physics,0x320,v->up,.999f,1.001f,&p->fallback,112)||
       !hvr_unit(physics,0x2fc,v->motion,.9f,1.1f,&p->fallback,113)||
       !hvr_unit(physics,0x308,v->pending,.9f,1.1f,&p->fallback,114)||
       !hvr_ground_axis(physics,frame,v->up,v->target_up,&p->fallback,115))return hvr_group(&p->fallback,120);
    if(!hvr_motion_plane(v->motion,v->up,&p->fallback,116)||!hvr_motion_plane(v->motion,v->target_up,&p->fallback,117)||
       !hvr_motion_plane(v->pending,v->up,&p->fallback,118)||!hvr_motion_plane(v->pending,v->target_up,&p->fallback,119)){
        if(!hrp_vectors(physics,frame,v))return hvr_group(&p->fallback,121);
    }
    /* Native actual320 rotates toward the airborne target. It remains a body
     * coherence/tangency check above; the future road envelope uses the
     * retained ground-admission plane. Actual/pending travel vectors still
     * drive both alignment and four-update 3D width forecasts unchanged.
     * This is limited to the branch-qualified airborne exception. */
    for(u32 i=0;i<3;i++)v->up[i]=v->target_up[i];
    return 1;
}

static u32 hvr_observe(const void *physics,const void *route,u32 ticks,const SnakeHopFrame *frame,HopVectors *v){
    HvrProbe p={0};
    u32 accepted=hvr_air_vectors(physics,frame,v,&p);
    if(!accepted)hvr_publish(&p,frame,physics,route,ticks);
    return accepted;
}
