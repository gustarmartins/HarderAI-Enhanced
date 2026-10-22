/* Canonical shared plane for an exactly collinear BASE pair.
 * Independent float projections can leave a few ULPs between their caps.
 * Repair only closed longitudinal endpoints, and prove strict radial room at
 * the common cut. Real bends, radial endpoints and disconnected geometry keep
 * their original intervals. No native stores, tolerance or widened tube. */
EXPORT const u32 lab_snaking_collinear_node_config[4]={1,0,1,0};
static u32 scn_join(const float *first,const float *second,const float *node_offset,
                    const float *axis,float clearance,float next_clearance,
                    HnbInterval *a,HnbInterval *z){
    if(a->hi_open||z->lo_open||!finite(a->hi)||!finite(z->lo)||
       a->hi>=z->lo||!finite(clearance)||!finite(next_clearance)||
       clearance<=0||next_clearance<=0)return 0;
    double length2=0,advance=0,projection=0,forward=0;
    for(u32 i=0;i<3;i++){
        if(!finite(first[i])||!finite(second[i])||!finite(node_offset[i])||!finite(axis[i]))return 0;
        length2+=(double)first[i]*first[i];
        advance+=(double)first[i]*axis[i];
        projection+=(double)first[i]*node_offset[i];
        forward+=(double)first[i]*second[i];
    }
    /* Products of finite binary32 operands are exact in binary64. Equality
     * here means collinear represented deltas, with no angular epsilon. */
    if(length2<=0||advance<=0||forward<=0||
       (double)first[0]*second[1]!=(double)first[1]*second[0]||
       (double)first[0]*second[2]!=(double)first[2]*second[0]||
       (double)first[1]*second[2]!=(double)first[2]*second[1])return 0;
    float cut=(float)(-projection/advance);
    if(!finite(cut)||cut<a->hi||cut>z->lo||cut<=a->lo||cut>=z->hi)return 0;
    double point[3],along=0,radial2=0;
    for(u32 i=0;i<3;i++){
        point[i]=(double)node_offset[i]+(double)axis[i]*cut;
        along+=point[i]*first[i];
    }
    along/=length2;
    for(u32 i=0;i<3;i++){
        double residual=point[i]-along*first[i];radial2+=residual*residual;
    }
    double radius=clearance<next_clearance?clearance:next_clearance;
    if(!(radial2<radius*radius))return 0;
    a->hi=cut;z->lo=cut;return 1;
}
