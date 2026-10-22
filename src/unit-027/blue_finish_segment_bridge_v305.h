/* An existing unfinished blue may cross one nearly straight route node.
 * The forecast must remain within the union of both contracted BASE tubes
 * and both SELECTED longitudinal ranges. Native charge/steering stay untouched.
 * This is a route-envelope forecast, not KCL or a bound on future steering. */
EXPORT const struct {u32 abi,max_edges,blue_only,reserved;float alignment_squared;}
lab_blue_finish_segment_bridge_config={1,2,1,0,.995f};

typedef struct {float lo,hi;u32 lo_open,hi_open;} BfbInterval;
static float bfb_dot(const float *a,const float *z){return a[0]*z[0]+a[1]*z[1]+a[2]*z[2];}
static u32 bfb_straight(const float *a,float a2,const float *z,float z2){
    float d=bfb_dot(a,z);
    return finite(d)&&d>0&&d*d>=lab_blue_finish_segment_bridge_config.alignment_squared*a2*z2;
}
/* Parameter t is distance traveled along a unit supplied motion axis.
 * Longitudinal bounds are inclusive at the shared internal node. The final
 * endpoint and radial clearance remain strict when coverage is accepted. */
static u32 bfb_interval(const float *delta,float length2,float projection,
    const float *offset,const float *motion,float clearance,float end_reserve,
    u32 radial,BfbInterval *out){
    float advance=bfb_dot(delta,motion),length=root_positive(length2);
    float end=length2-end_reserve*length;
    if(!finite(advance)||advance<=0||!finite(end)||end<=0)return 0;
    float lo=-projection/advance,hi=(end-projection)/advance;
    u32 lo_open=0,hi_open=end_reserve>0;
    if(!finite(lo)||!finite(hi)||hi<=lo)return 0;
    if(radial){
        float a=0,b0=0,c=0;
        for(u32 i=0;i<3;i++){
            float r=offset[i]-projection/length2*delta[i];
            float v=motion[i]-advance/length2*delta[i];
            a+=v*v;b0+=r*v;c+=r*r;
        }
        c-=clearance*clearance;
        if(!finite(a)||!finite(b0)||!finite(c)||a<0||clearance<=0)return 0;
        if(a==0){if(c>=0)return 0;}
        else{
            float discriminant=b0*b0-a*c;
            if(!finite(discriminant)||discriminant<=0)return 0;
            float root=root_positive(discriminant);
            /* Stable quadratic roots for a*t*t+2*b0*t+c. */
            float qv=-b0-(b0<0?-root:root),r0,r1;
            if(qv==0)return 0;
            r0=qv/a;r1=c/qv;
            if(!finite(r0)||!finite(r1))return 0;
            if(r0>r1){float temporary=r0;r0=r1;r1=temporary;}
            if(r0>=lo){lo=r0;lo_open=1;}if(r1<=hi){hi=r1;hi_open=1;}
        }
    }
    if(!finite(lo)||!finite(hi)||hi<=lo)return 0;
    out->lo=lo;out->hi=hi;out->lo_open=lo_open;out->hi_open=hi_open;return 1;
}
static u32 bfb_cover(BfbInterval current,BfbInterval next,float travel){
    /* A positive future interval cannot substitute for current containment.
     * Overlap/meeting of the two convex intervals covers the entire forecast. */
    return (current.lo<0||(current.lo==0&&!current.lo_open))&&current.hi>0&&
        (current.hi>travel||((next.lo<current.hi||
          (next.lo==current.hi&&!next.lo_open&&!current.hi_open))&&next.hi>travel));
}
