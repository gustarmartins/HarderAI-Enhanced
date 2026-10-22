/* Same-tick owned grounded snaking feedback while SELECTED is ahead of BASE.
 * Both four-update rays stay in the smaller contracted physical node ball,
 * as in the existing heading-node proof. No native path/pose/charge stores. */
EXPORT const u32 lab_snaking_previous_base_heading_config[4]={1,1,4,0};
static u32 pbh_covered(const void *driver,const void *physics,const void *route,
    u64 selected,u64 base,u64 a,u64 z,u64 ba,u64 bz,const float *up,
    const float *motion,const float *pending,float speed,float *slack,
    HeadingNodeProof *proof){
    if(lab_bounded_drift_heading_config.horizon!=4||!finite(speed)||speed<=0)return 0;
    proof->selected=selected;
    for(u32 k=0;k<4;k++)proof->cache[k]=q(route,0x50+8*k);
    if(a!=proof->cache[2]||z!=proof->cache[1])return 0;
    u64 older=proof->cache[3];
    if(!be_span(older,0x374)||older==a||older==z||
       be_point((void *)selected,w((void *)older,0x8c))!=older||
       !be_edge((void *)older,w((void *)a,0x8c)))return 0;
    u64 bo=be_point((void *)base,w((void *)older,0x8c));
    if(!bo||bo==ba||bo==bz||!be_edge((void *)bo,w((void *)ba,0x8c))||
       b((void *)bo,0x370)||b((void *)bo,0x2e4)>1)return 0;
    proof->nodes[0]=older;proof->nodes[1]=a;proof->nodes[2]=z;
    proof->nodes[3]=bo;proof->nodes[4]=ba;proof->nodes[5]=bz;
    hn_snapshot(proof);
    float pd[3],po[3],pn,pp,nd[3],no[3],nn,np;
    if(!span(physics,bo,ba,up,pd,po,&pn,&pp)||pp<0||pp>pn||
       !span(physics,ba,bz,up,nd,no,&nn,&np)||np>=0||
       !hn_adjacent(pd,pn,nd,nn))return 0;
    float radius=10000.f;
    for(u32 j=3;j<6;j++)for(u32 k=0;k<3;k++){
        float value=f((void *)proof->nodes[j],0x24+4*k);
        if(!finite(value)||value<=0||value>10000.f)return 0;
        if(value<radius)radius=value;
    }
    radius-=lab_bounded_drift_heading_config.margin;
    if(radius<=0)return 0;
    float maximum=dot(no,no),travel=4.f*speed;
    if(!finite(maximum))return 0;
    for(u32 mode=0;mode<2;mode++){
        const float *axis=mode?pending:motion;
        if(!hn_ray(axis,up,pd,pn,nd,nn,np,speed,1))return 0;
        float endpoint[3];
        for(u32 k=0;k<3;k++)endpoint[k]=no[k]+travel*axis[k];
        float distance=dot(endpoint,endpoint);
        if(!finite(distance))return 0;
        if(distance>maximum)maximum=distance;
    }
    if(maximum>=radius*radius)return 0;
    *slack=radius-root(maximum);
    return finite(*slack)&&*slack>0&&hn_same(proof,driver,route);
}
