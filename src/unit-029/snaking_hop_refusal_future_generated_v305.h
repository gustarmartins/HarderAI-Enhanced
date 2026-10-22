/* Read-only owned-hop future coverage. Current cones, planes and full charge
 * horizon remain mandatory. No route search, steering, native state or pose
 * writes. Qualified selected joints use the existing exact bisector rule. */
EXPORT const struct {u32 abi,max_edges,reserved0,reserved1;float current_alignment,adjacent_alignment;}
lab_snaking_hop_future_corridor_config={1,16,0,0,.95f,.995f};
typedef struct {float sd[3],bd[3],sn,bn,sp,bp;HnbInterval s[2],b[2];} HfrEdge;

static u32 hfr_frame(const float *d,float n,float projection,u32 future,const HopVectors *v,HfdProbe *probe,u32 view){
    float one=hf_dot(d,v->motion),two=hf_dot(d,v->pending);
    float up=hf_dot(d,v->up),target=hf_dot(d,v->target_up);
    if(!finite(n)||n<64||!finite(one)||!finite(two)||one<=0||two<=0||
       !finite(up)||!finite(target)||up*up>.0025f*n||target*target>.0025f*n)return hfd_future_fail(probe,101,probe->segment,view,0xffffffffu,probe->a,probe->z,probe->ba,probe->bz,(const float[]){n,projection,one,two,up,target,d[0],d[1],d[2]},9);
    /* Only the hypothetical future-angle veto is replaced. Any edge whose
     * longitudinal projection contains the current kart keeps both cones. */
    u32 accepted=future||projection<0||projection>n||
        (one*one>=lab_snaking_config.alignment_squared*n&&two*two>=lab_snaking_config.alignment_squared*n);
    if(!accepted)return hfd_future_fail(probe,102,probe->segment,view,0xffffffffu,probe->a,probe->z,probe->ba,probe->bz,(const float[]){n,projection,one,two,up,target,d[0],d[1],d[2]},9);
    return accepted;
}
static u32 hfr_cover(const HfrEdge *edges,u32 count,u32 mode,u32 base,float required,HfdProbe *probe,const u64 *seen){
    float end=0;u32 end_open=0,started=0;
    for(u32 i=0;i<count;i++){
        HnbInterval x=base?edges[i].b[mode]:edges[i].s[mode];
        if(x.hi<0)continue;
        if(x.lo>end||(x.lo==end&&(x.lo_open||end_open)))return hfd_future_fail(probe,103,i,base,mode,seen[i],seen[i+1],0,0,(const float[]){x.lo,x.hi,end,required,(float)x.lo_open,(float)x.hi_open,(float)end_open},7);
        if(x.hi>end){end=x.hi;end_open=x.hi_open;started=1;}
        if(started&&end>required)return 1;
    }
    return hfd_future_fail(probe,104,0xffffffffu,base,mode,seen[count-1],seen[count],0,0,(const float[]){end,required,(float)started,(float)end_open,(float)count},5);
}
static u32 hfr_node(const void *p){
    return b(p,0x2e4)<=1&&!b(p,0x370)&&w(p,0x30)<=10&&w(p,0x34)<=10;
}
static u32 hfr_future(const void *physics,void *course,void *base,u64 a,u64 z,
                      float speed,float required,const HopVectors *v,HfdProbe *probe){
    HfdProbe coverage={0};
    if(!finite(required)||required<=64.f||!finite(speed)||speed<=2.f||
       speed>lab_snaking_config.maximum_speed)return hfd_future_fail(probe,105,0xffffffffu,0xffffffffu,0xffffffffu,a,z,0,0,(const float[]){speed,required},2);
    HfrEdge edges[16];u64 seen[17];seen[0]=a;u32 used=1;float previous_clearance=0;
    for(u32 segment=0;segment<16;segment++){
        u32 ak=w((void *)a,0x8c),zk=w((void *)z,0x8c),key;
        if(be_point(course,ak)!=a||be_point(course,zk)!=z||
           !hfr_node((void *)a)||!hfr_node((void *)z)||
           !successor((void *)a,&key)||key!=zk)return hfd_future_fail(probe,106,segment,0,0xffffffffu,a,z,0,0,(const float *)0,0);
        for(u32 i=0;i<used;i++)if(seen[i]==z)return hfd_future_fail(probe,107,segment,0,0xffffffffu,a,z,0,0,(const float *)0,0);seen[used++]=z;
        u64 ba=be_point(base,ak),bz=be_point(base,zk);
        if(!ba||!bz||!hfr_node((void *)ba)||!hfr_node((void *)bz)||!be_edge((void *)ba,zk))return hfd_future_fail(probe,108,segment,1,0xffffffffu,a,z,ba,bz,(const float *)0,0);
        HfrEdge *e=&edges[segment];
        probe->segment=segment;probe->a=a;probe->z=z;probe->ba=ba;probe->bz=bz;
        if(!hf_intent(physics,(void *)a,(void *)z,v,0,e->sd,&e->sn,&e->sp)||
           !hf_intent(physics,(void *)ba,(void *)bz,v,0,e->bd,&e->bn,&e->bp)||
           !hfr_frame(e->sd,e->sn,e->sp,segment!=0,v,probe,0)||!hfr_frame(e->bd,e->bn,e->bp,segment!=0,v,probe,1)){if(probe->substage)return 0;return hfd_future_fail(probe,109,segment,0xffffffffu,0xffffffffu,a,z,ba,bz,(const float[]){speed,required},2);}
        if(segment&&(!hnb_straight(edges[segment-1].sd,edges[segment-1].sn,e->sd,e->sn)||
                     !hnb_straight(edges[segment-1].bd,edges[segment-1].bn,e->bd,e->bn)))return hfd_future_fail(probe,110,segment,0xffffffffu,0xffffffffu,a,z,ba,bz,(const float[]){edges[segment-1].sd[0],edges[segment-1].sd[1],edges[segment-1].sd[2],edges[segment-1].sn,e->sd[0],e->sd[1],e->sd[2],e->sn,edges[segment-1].bd[0],edges[segment-1].bd[1],edges[segment-1].bd[2],edges[segment-1].bn,e->bd[0],e->bd[1],e->bd[2],e->bn},16);
        float width=f((void *)ba,0x24),other=f((void *)bz,0x24);
        if(!finite(width)||!finite(other)||width<lab_snaking_config.minimum_width||
           other<lab_snaking_config.minimum_width||width>10000||other>10000)return hfd_future_fail(probe,111,segment,1,0xffffffffu,a,z,ba,bz,(const float[]){width,other,speed,required},4);
        if(other<width)width=other;
        float clearance=width-lab_snaking_config.margin-2.f*speed;if(clearance<=0)return hfd_future_fail(probe,112,segment,1,0xffffffffu,a,z,ba,bz,(const float[]){width,clearance,speed,required},4);
        float so[3],bo[3];
        for(u32 i=0;i<3;i++){
            float p=f(physics,0x2c+4*i);so[i]=p-f((void *)a,0xc+4*i);bo[i]=p-f((void *)ba,0xc+4*i);
            if(!finite(so[i])||!finite(bo[i]))return hfd_future_fail(probe,113,segment,0xffffffffu,i,a,z,ba,bz,(const float[]){so[i],bo[i]},2);
        }
        for(u32 mode=0;mode<2;mode++){
            const float *axis=mode?v->pending:v->motion;
            if(!hnb_interval(e->sd,e->sn,e->sp,so,axis,0,0,0,&e->s[mode])||
               !hnb_interval(e->bd,e->bn,e->bp,bo,axis,clearance,0,1,&e->b[mode]))return hfd_future_fail(probe,114,segment,0xffffffffu,mode,a,z,ba,bz,(const float[]){e->sn,e->sp,e->bn,e->bp,clearance,required,axis[0],axis[1],axis[2],so[0],so[1],so[2],bo[0],bo[1],bo[2]},15);
            if(segment&&!hnb_selected_join(edges[segment-1].sd,edges[segment-1].sn,e->sd,e->sn,
                so,axis,&edges[segment-1].s[mode],&e->s[mode]))return hfd_future_fail(probe,115,segment,0,mode,a,z,ba,bz,(const float[]){edges[segment-1].sn,e->sn,required,edges[segment-1].sd[0],edges[segment-1].sd[1],edges[segment-1].sd[2],e->sd[0],e->sd[1],e->sd[2],so[0],so[1],so[2],axis[0],axis[1],axis[2]},15);
            if(segment)scn_join(edges[segment-1].bd,e->bd,bo,axis,
                previous_clearance,clearance,&edges[segment-1].b[mode],&e->b[mode]);
        }
        previous_clearance=clearance;
        if(hfr_cover(edges,segment+1,0,0,required,&coverage,seen)&&hfr_cover(edges,segment+1,1,0,required,&coverage,seen)&&
           hfr_cover(edges,segment+1,0,1,required,&coverage,seen)&&hfr_cover(edges,segment+1,1,1,required,&coverage,seen))return 1;
        if(!successor((void *)z,&key))return hfd_coverage_end(probe,&coverage,116);
        a=z;z=be_point(course,key);if(!z)return hfd_coverage_end(probe,&coverage,117);
    }
    return hfd_coverage_end(probe,&coverage,118);
}
static u32 hfr_allow(u32 segment,u32 *cached,const void *physics,void *course,void *base,
                     u64 a,u64 z,float speed,float required,const HopVectors *v,HfdProbe *probe){
    if(!segment)return 0;
    if(!*cached)*cached=hfr_future(physics,course,base,a,z,speed,required,v,probe)?2:1;
    return *cached==2;
}
