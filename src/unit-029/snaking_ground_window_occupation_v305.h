/* Native state and geometry remain untouched. Called only by the
 * existing grounded window, with its original eight-update grant bound. */
EXPORT const u32 lab_snaking_ground_window_occupation_config[4]={1,2,4,8};
static u32 gwp_occupation(const void *physics,const void *route,void *course,void *native_base,
    u64 a,u64 z,u64 ba,u64 bz,const float *delta,float length2,float projection,
    float width,float speed,const HopVectors *v,float *prior_remaining){
    if(!finite(projection)||projection>=0||!finite(speed)||speed<=2.f||
       a!=q(route,0x60)||z!=q(route,0x58))return 0;
    u64 older=q(route,0x68);
    if(!be_span(older,0x374)||older==a||older==z||
       be_point(course,w((void *)older,0x8c))!=older||
       !be_edge((void *)older,w((void *)a,0x8c)))return 0;
    u64 bo=be_point(native_base,w((void *)older,0x8c));
    if(!bo||bo==ba||bo==bz||!be_edge((void *)bo,w((void *)ba,0x8c))||
       b((void *)bo,0x370)||b((void *)bo,0x2e4)>1)return 0;
    float sd[3],sn,sp,pd[3],pn,pp;
    if(!hf_intent(physics,(void *)a,(void *)z,v,1,sd,&sn,&sp)||
       !hf_intent(physics,(void *)bo,(void *)ba,v,1,pd,&pn,&pp)||
       !gw_edge(sd,sn,v)||!gw_edge(pd,pn,v)||!gw_edge(delta,length2,v)||
       !hnb_straight(pd,pn,delta,length2))return 0;
    float previous_width=f((void *)bo,0x24),radius=f((void *)ba,0x24);
    if(!finite(previous_width)||!finite(radius)||previous_width<lab_snaking_config.minimum_width||
       radius<lab_snaking_config.minimum_width||previous_width>10000.f||radius>10000.f)return 0;
    if(radius<previous_width)previous_width=radius;
    float margin=lab_snaking_config.margin+2.f*speed;
    float previous_clearance=previous_width-margin,current_clearance=width-margin,travel=4.f*speed;
    if(previous_clearance<=0||current_clearance<=0)return 0;
    float previous_offset[3],current_offset[3],selected_offset[3];
    for(u32 i=0;i<3;i++){
        float position=f(physics,0x2c+4*i);
        previous_offset[i]=position-f((void *)bo,0xc+4*i);
        current_offset[i]=position-f((void *)ba,0xc+4*i);
        selected_offset[i]=position-f((void *)a,0xc+4*i);
        if(!finite(previous_offset[i])||!finite(current_offset[i])||!finite(selected_offset[i]))return 0;
    }
    for(u32 mode=0;mode<2;mode++){
        const float *axis=mode?v->pending:v->motion;
        HnbInterval previous,current,selected;
        if(!hnb_interval(pd,pn,pp,previous_offset,axis,previous_clearance,0.f,1,&previous)||
           !hnb_interval(delta,length2,projection,current_offset,axis,current_clearance,0.f,1,&current)||
           !hnb_interval(sd,sn,sp,selected_offset,axis,0.f,0.f,0,&selected)||
           !hnc_cover(previous,current,travel,current_offset,axis,previous_clearance,current_clearance)||
           !hbs_contains(selected,travel))return 0;
    }
    float length=root(pn),distance=length-pp/length;
    if(!finite(distance)||distance<0||distance>length)return 0;
    *prior_remaining=distance;return 1;
}

static u32 gws_occupation(const void *physics,const void *route,void *course,void *native_base,
    u64 a,u64 z,u64 ba,u64 bz,const float *delta,float length2,float projection,
    float speed,const HopVectors *v,float *debt){
    if(!finite(projection)||projection<length2||!finite(speed)||speed<=2.f)return 0;
    u64 following=z==q(route,0x60)?q(route,0x58):q(route,0x50);
    u32 key;
    if(!ptr(following)||following==a||following==z||
       !successor((void *)z,&key)||be_point(course,key)!=following||
       w((void *)following,0x8c)!=key)return 0;
    u64 bf=be_point(native_base,key);
    if(!bf||bf==ba||bf==bz||!be_edge((void *)bz,key))return 0;
    float sd[3],sd2,sp,nsd[3],nsd2,nsp,nd[3],nd2,np;
    if(!hf_intent(physics,(void *)a,(void *)z,v,1,sd,&sd2,&sp)||
       !hf_intent(physics,(void *)z,(void *)following,v,0,nsd,&nsd2,&nsp)||
       !hf_intent(physics,(void *)bz,(void *)bf,v,1,nd,&nd2,&np)||
       !gw_edge(sd,sd2,v)||!hnb_future_edge(nsd,nsd2,nsp,v)||!gw_edge(nd,nd2,v)||
       !hnb_straight(sd,sd2,nsd,nsd2)||!hnb_straight(delta,length2,nd,nd2))return 0;
    float width=f((void *)bz,0x24),r=f((void *)bf,0x24);
    if(!finite(width)||!finite(r)||width<lab_snaking_config.minimum_width||
       r<lab_snaking_config.minimum_width||width>10000||r>10000)return 0;
    if(r<width)width=r;
    float clearance=width-lab_snaking_config.margin-2.f*speed,travel=4.f*speed;
    if(clearance<=0)return 0;
    float base_offset[3],selected_offset[3],next_selected_offset[3];
    for(u32 i=0;i<3;i++){
        float p=f(physics,0x2c+4*i);
        base_offset[i]=p-f((void *)bz,0xc+4*i);
        selected_offset[i]=p-f((void *)a,0xc+4*i);
        next_selected_offset[i]=p-f((void *)z,0xc+4*i);
        if(!finite(base_offset[i])||!finite(selected_offset[i])||!finite(next_selected_offset[i]))return 0;
    }
    for(u32 mode=0;mode<2;mode++){
        const float *axis=mode?v->pending:v->motion;
        HnbInterval next_base,first_selected,next_selected;
        if(!hnb_interval(nd,nd2,np,base_offset,axis,clearance,0.f,1,&next_base)||
           !hnb_interval(sd,sd2,sp,selected_offset,axis,0.f,0.f,0,&first_selected)||
           !hnb_interval(nsd,nsd2,nsp,next_selected_offset,axis,0.f,0.f,0,&next_selected)||
           !hnb_selected_join(sd,sd2,nsd,nsd2,next_selected_offset,axis,&first_selected,&next_selected)||
           !hbs_contains(next_base,travel)||
           !(hnb_cover(first_selected,next_selected,travel)||hbs_contains(next_selected,travel)))return 0;
    }
    /* The next loop edge adds its complete base length. Debit the already
     * occupied distance measured on that edge, rather than the old tangent. */
    float distance=np/root(nd2);
    if(!finite(distance)||distance<0)return 0;
    *debt=distance;return 1;
}
