/* Admission-side choice in the current road frame. Native route, state,
 * steering, ratio and charge are read-only; caller owns the new R request. */
EXPORT const struct {u32 abi,edges;float edge_fraction,band_ticks,turn_sine;}
lab_snaking_entry_choice_config={1,16,.5f,1.5f,.035f};

typedef struct {float delta[3],right[3],length2,projection,lateral;} SecEdge;
static u32 sec_edge(void *physics,void *a,void *z,const float *up,u32 occupied,SecEdge *edge){
    if(!be_intent(physics,a,z,up,1.f,.0025f,occupied,edge->delta,&edge->length2,&edge->projection))return 0;
    float right2=0,forward=0,heading2=0;
    for(u32 k=0;k<3;k++){
        u32 i=(k+1)%3,j=(k+2)%3;
        edge->right[k]=up[i]*edge->delta[j]-up[j]*edge->delta[i];right2+=edge->right[k]*edge->right[k];
        float h=f(physics,0x20+4*k);if(!finite(h))return 0;
        forward+=h*edge->delta[k];heading2+=h*h;
    }
    if(!finite(right2)||right2<64||heading2<.9f||heading2>1.1f||forward<=0||
       forward*forward<lab_snaking_config.alignment_squared*edge->length2*heading2)return 0;
    float norm=root(right2);edge->lateral=0;
    for(u32 k=0;k<3;k++){
        edge->right[k]/=norm;
        edge->lateral+=(f(physics,0x2c+4*k)-f(a,0xc+4*k))*edge->right[k];
    }
    return finite(edge->lateral);
}

static i32 sec_bend(void *physics,void *course,void *base,u64 a,u64 z,
                    const float *up,const SecEdge *selected,const SecEdge *native,float required){
    float covered=root(selected->length2)-selected->projection/root(selected->length2);
    float previous_selected[3],previous_base[3],selected2=selected->length2,base2=native->length2;
    for(u32 k=0;k<3;k++){previous_selected[k]=selected->delta[k];previous_base[k]=native->delta[k];}
    u64 seen[18];seen[0]=a;seen[1]=z;u32 used=2;
    for(u32 edge=1;edge<lab_snaking_entry_choice_config.edges&&covered<required;edge++){
        u32 key;if(!successor((void *)z,&key))return 0;
        u64 next=be_point(course,key);if(!next)return 0;
        for(u32 i=0;i<used;i++)if(seen[i]==next)return 0;seen[used++]=next;
        u64 ba=be_point(base,w((void *)z,0x8c)),bz=be_point(base,key);
        if(!ba||!bz||!be_edge((void *)ba,key))return 0;
        SecEdge s,b;
        if(!sec_edge(physics,(void *)z,(void *)next,up,0,&s)||!sec_edge(physics,(void *)ba,(void *)bz,up,0,&b)||
           !hnb_straight(previous_selected,selected2,s.delta,s.length2)||
           !hnb_straight(previous_base,base2,b.delta,b.length2))return 0;
        float st=hf_dot(selected->right,s.delta)/root(s.length2),bt=hf_dot(native->right,b.delta)/root(b.length2);
        float minimum=lab_snaking_entry_choice_config.turn_sine;
        /* Native explicit drift input signs are opposite the released
         * heading: 1850E4 rotates travel by -drift+24. Choose the input whose
         * chassis/release direction follows this shared upcoming bend. */
        if(st>=minimum&&bt>=minimum)return -1;
        if(st<=-minimum&&bt<=-minimum)return 1;
        covered+=root(s.length2);a=z;z=next;
        for(u32 k=0;k<3;k++){previous_selected[k]=s.delta[k];previous_base[k]=b.delta[k];}
        selected2=s.length2;base2=b.length2;
    }
    return 0;
}

EXPORT i32 lab_snaking_entry_choice(void *driver,void *physics,void *producer,void *route,
                                  u32 racer,u32 now,i32 fallback,float required){
    if(racer>=12||(fallback!=-1&&fallback!=1)||!finite(required)||required<=64.f||
       !ptr((u64)driver)||!ptr((u64)physics)||!ptr((u64)producer)||!ptr((u64)route))return 0;
    SnakeState *state=&lab_snaking_state[racer];u64 clock=state->clock;
    if(!ptr(clock)||state->lock!=1||state->phase||state->driver!=(u64)driver||
       state->producer!=(u64)producer||state->last!=now||w((void *)clock,0x38)!=6||w((void *)clock,0x44)!=now)return 0;
    u64 role=q(physics,0xf8),drift=q(physics,0x110),source=q(producer,0x80),packet=q(producer,0x98);
    if(!ptr(role)||!ptr(drift)||!ptr(source)||!ptr(packet)||q((void *)role,0x28)!=(u64)physics||
       q((void *)drift,8)!=role||w((void *)drift,0x10)||w((void *)role,0xa8)!=racer||
       b((void *)role,0xd2)!=1||b((void *)role,0xd3)!=1||!b((void *)role,0xe1)||b((void *)role,0xe7)||
       (w((void *)role,0x1cc)&0x01204018u)||q(producer,0x70)!=(u64)driver||q(producer,0x88)!=(u64)route||
       b(producer,0x41)||q((void *)source,0x58)!=(u64)driver||q((void *)source,0x60)!=(u64)route||
       b((void *)source,0x80)||!b(physics,0x201)||!hf_environment(physics))return 0;
    u32 flags=w((void *)packet,4);if(!(flags&1)||(flags&10))return 0;
    u64 course=q(driver,0x80),base=be_course((u64)lab_module_start-LAB_MAIN_TO_MODULE,driver,route);
    if(!ptr(course)||!base)return 0;
    u64 ca=q(route,0x60),cz=q(route,0x58),older=q(route,0x68),a=ca,z=cz;
    if(!be_span(a,0x374)||!be_span(z,0x374)||a==z||be_point((void *)course,w((void *)a,0x8c))!=a||
       be_point((void *)course,w((void *)z,0x8c))!=z||!be_edge((void *)a,w((void *)z,0x8c)))return 0;
    float up[3];if(!hf_unit(physics,0x14,up,.9f,1.1f))return 0;
    SecEdge selected,native;
    if(!sec_edge(physics,(void *)a,(void *)z,up,0,&selected))return 0;
    if(selected.projection<0){
        if(!be_span(older,0x374)||older==a||older==z||be_point((void *)course,w((void *)older,0x8c))!=older||
           !be_edge((void *)older,w((void *)a,0x8c))||!sec_edge(physics,(void *)older,(void *)a,up,1,&selected))return 0;
        z=a;a=older;
    }else if(selected.projection>selected.length2)return 0;
    u64 ba=be_point((void *)base,w((void *)a,0x8c)),bz=be_point((void *)base,w((void *)z,0x8c));
    if(!ba||!bz||!be_edge((void *)ba,w((void *)bz,0x8c))||!sec_edge(physics,(void *)ba,(void *)bz,up,1,&native))return 0;
    float speed=f(physics,0x37c);if(!finite(speed)||speed<=2.f)return 0;
    for(u32 off=0x424;off<=0x430;off+=4){float value=f(physics,off);if(!finite(value)||value<0)return 0;if(value>speed)speed=value;}
    if(speed>lab_snaking_config.maximum_speed)return 0;
    float width=f((void *)ba,0x24),other=f((void *)bz,0x24);
    if(!finite(width)||!finite(other)||width<lab_snaking_config.minimum_width||other<lab_snaking_config.minimum_width||width>10000||other>10000)return 0;
    if(other<width)width=other;
    float clearance=width-lab_snaking_config.margin-2.f*speed;
    float bl=native.lateral,sl=selected.lateral,band=lab_snaking_entry_choice_config.band_ticks*speed;
    if(clearance<=0||bl<=-clearance||bl>=clearance)return 0;
    if(band<4.f)band=4.f;
    i32 choice;
    if(bl>lab_snaking_entry_choice_config.edge_fraction*clearance)choice=-1;
    else if(bl<-lab_snaking_entry_choice_config.edge_fraction*clearance)choice=1;
    else if(sl>band)choice=-1;
    else if(sl<-band)choice=1;
    else{
        choice=sec_bend(physics,(void *)course,(void *)base,a,z,up,&selected,&native,required);
        if(!choice)choice=bl>band?-1:bl<-band?1:fallback;
    }
    /* Native lookup precedes every final owner/control readback: a change
     * observed during that call must not inherit an earlier valid decision. */
    if(be_course((u64)lab_module_start-LAB_MAIN_TO_MODULE,driver,route)!=base)return 0;
    if(state->clock!=clock||state->lock!=1||state->phase||state->last!=now||
       state->driver!=(u64)driver||state->producer!=(u64)producer||
       q(physics,0xf8)!=role||q(physics,0x110)!=drift||q((void *)role,0x28)!=(u64)physics||q((void *)drift,8)!=role||
       w((void *)drift,0x10)||w((void *)role,0xa8)!=racer||b((void *)role,0xd2)!=1||b((void *)role,0xd3)!=1||
       !b((void *)role,0xe1)||b((void *)role,0xe7)||(w((void *)role,0x1cc)&0x01204018u)||
       q(producer,0x70)!=(u64)driver||q(producer,0x88)!=(u64)route||q(producer,0x80)!=source||q(producer,0x98)!=packet||
       b(producer,0x41)||b((void *)source,0x80)||q((void *)source,0x58)!=(u64)driver||q((void *)source,0x60)!=(u64)route||
       w((void *)packet,4)!=flags||q(driver,0x80)!=course||q(route,0x60)!=ca||q(route,0x58)!=cz||q(route,0x68)!=older||
       !b(physics,0x201)||!hf_environment(physics)||
       w((void *)clock,0x38)!=6||w((void *)clock,0x44)!=now)return 0;
    return choice;
}
