/* Private owned-hop trial. Retail182A8C selects the negated local gravity
 * vector when selector27B==1; it is not restricted to world-down gravity.
 * Preserve the native raw floats. No axes, charge, ratio or route writes.
 * A newly supported direction requires the existing consecutive native-axis
 * history. The old world-down path retains its previous admission rules. */
EXPORT const u32 lab_snaking_local_gravity_config[4]={1,11,19,0x4c470001};
static u32 ngs_owned_history(const void *physics,const SnakeHopFrame *frame);

static u32 lsg_target(const void *physics,float *target){
    float gravity[3];
    for(u32 i=0;i<3;i++){
        gravity[i]=f(physics,0x27c+4*i);
        if(!finite(gravity[i]))return 0;
    }
    volatile float xx=gravity[0]*gravity[0],yy=gravity[1]*gravity[1];
    volatile float zz=gravity[2]*gravity[2],xy=xx+yy;
    float length2=xy+zz;
    if(!finite(length2)||length2<.999f||length2>1.001f)return 0;
    for(u32 i=0;i<3;i++)target[i]=-gravity[i];
    return 1;
}

static u32 lsg_hop_target(const void *physics,const SnakeHopFrame *frame){
    float target[3];
    if(!lsg_target(physics,target))return 0;
    for(u32 i=0;i<3;i++)
        if(w(physics,0x32c+4*i)!=nga_bits(target[i]))
            return w(physics,0x26c)>=11&&w(physics,0x26c)<=19&&
                   ngs_owned_history(physics,frame);
    u32 gx=w(physics,0x27c),gy=w(physics,0x280),gz=w(physics,0x284);
    if(!(gx&0x7fffffffu)&&gy==0xbf800000u&&!(gz&0x7fffffffu))return 1;
    return ngs_owned_history(physics,frame);
}
