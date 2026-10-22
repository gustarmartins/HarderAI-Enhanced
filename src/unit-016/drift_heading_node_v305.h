/* Keep held-drift feedback across one real, shallow cached path node.
 * Both four-update rays must fit wholly inside the shared contracted BASE
 * node ball. That ball is inside both infinite radial tubes. Forward motion,
 * current occupation and the next finite endpoint bound longitudinal travel.
 * This is path-envelope evidence, not a KCL/wall-clearance guarantee. */
EXPORT const u32 lab_drift_heading_node_config[4]={1,2,4,1};
typedef struct {
    u64 selected,cache[4],nodes[6];
    u32 words[6][13];
} HeadingNodeProof;
static const u32 hn_offsets[11]={0xc,0x10,0x14,0x24,0x28,0x2c,0x8c,0x30,0x34,0x38,0x60};

static void hn_snapshot(HeadingNodeProof *p){
    for(u32 j=0;j<6;j++){
        for(u32 k=0;k<11;k++)p->words[j][k]=w((void *)p->nodes[j],hn_offsets[k]);
        p->words[j][11]=b((void *)p->nodes[j],0x2e4);
        p->words[j][12]=b((void *)p->nodes[j],0x370);
    }
}
static u32 hn_same(const HeadingNodeProof *p,const void *driver,const void *route){
    if(q(driver,0x80)!=p->selected)return 0;
    for(u32 k=0;k<4;k++)if(q(route,0x50+8*k)!=p->cache[k])return 0;
    for(u32 j=0;j<6;j++){
        for(u32 k=0;k<11;k++)if(w((void *)p->nodes[j],hn_offsets[k])!=p->words[j][k])return 0;
        if(b((void *)p->nodes[j],0x2e4)!=p->words[j][11]||
           b((void *)p->nodes[j],0x370)!=p->words[j][12])return 0;
    }
    return 1;
}
static u32 hn_adjacent(const float *a,float an,const float *z,float zn){
    float advance=dot(a,z);
    return finite(advance)&&advance>0&&advance*advance>=.95f*an*zn;
}
static u32 hn_ray(const float *axis,const float *up,const float *d,float n,
                  const float *next,float nn,float next_projection,float speed,u32 owned){
    float one=dot(axis,d),two=dot(axis,next),vertical=dot(axis,up);
    if(!finite(one)||!finite(two)||one<=0||two<=0||vertical*vertical>.0025f||
       (!owned&&(one*one<.95f*n||two*two<.95f*nn)))return 0;
    float end=next_projection+speed*4.f*two;
    return finite(end)&&end+lab_bounded_drift_heading_config.margin*root(nn)<nn;
}

static u32 hn_continue(const void *driver,const void *physics,const void *route,
    u64 selected,u64 base,u64 a,u64 z,u64 ba,u64 bz,const float *up,
    const float *motion,const float *pending,float speed,u32 owned,
    float *slack,float *toward,HeadingNodeProof *proof){
    if(lab_bounded_drift_heading_config.horizon!=4||!finite(speed)||speed<=0)return 0;
    proof->selected=selected;
    for(u32 k=0;k<4;k++)proof->cache[k]=q(route,0x50+8*k);
    u64 following=z==proof->cache[3]&&scp_earlier(route,(void *)selected,
        proof->cache[3],proof->cache[2],proof->cache[1])==a?proof->cache[2]:
        z==proof->cache[2]?proof->cache[1]:z==proof->cache[1]?proof->cache[0]:0;
    if(!be_span(following,0x374)||following==a||following==z)return 0;
    u32 key=w((void *)following,0x8c);
    if(be_point((void *)selected,key)!=following||!be_edge((void *)z,key)||
       b((void *)following,0x370)||b((void *)following,0x2e4)>1)return 0;
    u64 bf=be_point((void *)base,key);
    if(!bf||bf==ba||bf==bz||!be_edge((void *)bz,key)||b((void *)bf,0x370)||b((void *)bf,0x2e4)>1)return 0;
    proof->nodes[0]=a;proof->nodes[1]=z;proof->nodes[2]=following;
    proof->nodes[3]=ba;proof->nodes[4]=bz;proof->nodes[5]=bf;hn_snapshot(proof);
    float sd[3],so[3],sn,sp,nd[3],no[3],nn,np;
    float bd[3],bo[3],bn,bp,bnd[3],bno[3],bnn,bnp;
    if(!span(physics,a,z,up,sd,so,&sn,&sp)||sp<0||sp>sn||
       !span(physics,z,following,up,nd,no,&nn,&np)||
       !span(physics,ba,bz,up,bd,bo,&bn,&bp)||bp<0||bp>bn||
       !span(physics,bz,bf,up,bnd,bno,&bnn,&bnp)||
       !hn_adjacent(sd,sn,nd,nn)||!hn_adjacent(bd,bn,bnd,bnn))return 0;
    float radius=10000.f;
    for(u32 j=3;j<6;j++)for(u32 k=0;k<3;k++){
        float value=f((void *)proof->nodes[j],0x24+4*k);
        if(!finite(value)||value<=0||value>10000.f)return 0;
        if(value<radius)radius=value;
    }
    radius-=lab_bounded_drift_heading_config.margin;
    if(radius<=0)return 0;
    float max_distance2=dot(bno,bno),travel=speed*4.f;
    if(!finite(max_distance2))return 0;
    for(u32 mode=0;mode<2;mode++){
        const float *axis=mode?pending:motion;
        if(!hn_ray(axis,up,sd,sn,nd,nn,np,speed,owned)||
           !hn_ray(axis,up,bd,bn,bnd,bnn,bnp,speed,owned))return 0;
        float end[3];for(u32 k=0;k<3;k++)end[k]=bno[k]+travel*axis[k];
        float distance2=dot(end,end);if(!finite(distance2))return 0;
        if(distance2>max_distance2)max_distance2=distance2;
    }
    if(max_distance2>=radius*radius)return 0;
    *slack=radius-root(max_distance2);
    if(!finite(*slack)||*slack<=0)return 0;
    float length=root(sn),next_length=root(nn),remaining=length-sp/length;
    float ahead=lab_bounded_drift_heading_config.lookahead;
    float available=remaining+next_length-lab_bounded_drift_heading_config.margin;
    if(ahead>available)ahead=available;
    if(!finite(ahead)||ahead<=0)return 0;
    for(u32 k=0;k<3;k++)toward[k]=ahead<=remaining?
        sd[k]*(sp/sn+ahead/length)-so[k]:nd[k]*((ahead-remaining)/next_length)-no[k];
    float vertical=dot(toward,up);
    for(u32 k=0;k<3;k++)toward[k]-=vertical*up[k];
    /* Center seeking can ask for a heading far outside the drift planner's
     * motion cone even when the kart still has ample lateral room. Reserve
     * half of the .95 cone around the shared tangent for drift transients.
     * The final four-cone check below qualifies the wider adjacent
     * bend for both selected and base views. It changes the local aim, never native path coordinates. */
    float tangent[3];
    for(u32 k=0;k<3;k++)tangent[k]=sd[k]/length+nd[k]/next_length;
    vertical=dot(tangent,up);
    for(u32 k=0;k<3;k++)tangent[k]-=vertical*up[k];
    float norm=dot(tangent,tangent);
    if(!finite(norm)||norm<=0)return 0;
    norm=root(norm);for(u32 k=0;k<3;k++)tangent[k]/=norm;
    float forward=dot(toward,tangent),lateral[3],side2=0;
    if(!finite(forward)||forward<=0)return 0;
    for(u32 k=0;k<3;k++){lateral[k]=toward[k]-forward*tangent[k];side2+=lateral[k]*lateral[k];}
    float cosine=root(.95f),limit2=forward*forward*(1.f-cosine)/(1.f+cosine);
    if(!finite(side2)||!finite(limit2)||limit2<=0)return 0;
    if(side2>limit2){
        float scale=root(limit2/side2);
        for(u32 k=0;k<3;k++)toward[k]=forward*tangent[k]+scale*lateral[k];
    }
    /* The wider adjacent bend must not make the final local goal point
     * outside either SELECTED or BASE edge cone, including tilted views. */
    float goal2=dot(toward,toward);
    if(!finite(goal2)||goal2<=0)return 0;
    const float *goal_edges[4]={sd,nd,bd,bnd};
    float goal_lengths[4]={sn,nn,bn,bnn};
    for(u32 edge=0;edge<4;edge++){
        float advance=dot(toward,goal_edges[edge]);
        if(!finite(advance)||advance<=0||
           advance*advance<.95f*goal2*goal_lengths[edge])return 0;
    }
    return hn_same(proof,driver,route);
}
