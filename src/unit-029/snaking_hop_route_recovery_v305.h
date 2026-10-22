/* Owned hop recovery from an angle proxy. Both actual/pending rays still need
 * the existing finite BASE road width and SELECTED/BASE travel coverage. Native vectors,
 * charge, steering, input and route nodes are never synthesized or written. */
EXPORT const u32 lab_snaking_hop_route_recovery_config[4]={1,16,30,4};
EXPORT u32 lab_snaking_hop_route_recovery(void *driver,void *physics,void *route,u32 ticks,const SnakeHopFrame *frame){
    u64 index=(u64)frame-(u64)lab_snaking_hop_frames;
    if(index>=sizeof(lab_snaking_hop_frames)||index%sizeof(SnakeHopFrame)||
       !ticks||ticks>lab_snaking_config.leg_deadline||!ptr(frame->clock)||!ptr(frame->producer)||
       !ptr((u64)driver)||!ptr((u64)physics)||!ptr((u64)route))return 0;
    SnakeState *state=&lab_snaking_state[index/sizeof(SnakeHopFrame)];
    u32 now=frame->last;u64 drift=q(physics,0x110),role=q(physics,0xf8);
    u64 producer=frame->producer,clock=frame->clock;
    if(!ptr(drift)||!ptr(role)||q((void *)drift,8)!=role||q((void *)role,0x28)!=(u64)physics||
       b((void *)role,0xd2)!=1||b((void *)role,0xd3)!=1||b((void *)role,0xe1)!=1||b((void *)role,0xe7)||
       (w((void *)role,0x1cc)&0x01204018u)||w((void *)role,0xa8)!=index/sizeof(SnakeHopFrame)||state->lock!=1||
       !hf_scope(frame,state,physics,(void *)drift,now,w((void *)drift,0x10))||
       w((void *)clock,0x38)!=6||w((void *)clock,0x44)!=now||
       q((void *)producer,0x70)!=(u64)driver||q((void *)producer,0x88)!=(u64)route||
       b((void *)producer,0x41))return 0;
    u64 source=q((void *)producer,0x80),packet=q((void *)producer,0x98);
    if(!ptr(source)||!ptr(packet)||q((void *)source,0x58)!=(u64)driver||
       q((void *)source,0x60)!=(u64)route||b((void *)source,0x80))return 0;
    u32 flags=w((void *)packet,4);if((flags&10)||!(flags&1))return 0;
    HopVectors vectors;if(!hvr_observe(physics,route,ticks,frame,&vectors))return 0;
    u64 course=q(driver,0x80),base=be_course((u64)lab_module_start-LAB_MAIN_TO_MODULE,driver,route);
    if(!ptr(course)||!base||course!=frame->course||base!=frame->base_course||
       frame->physics!=(u64)physics||frame->driver!=(u64)driver)return 0;
    float speed=f(physics,0x37c);if(!finite(speed)||speed<=2.f)return 0;
    for(u32 o=0x424;o<=0x430;o+=4){float cap=f(physics,o);if(!finite(cap)||cap<0)return 0;if(cap>speed)speed=cap;}
    if(speed>lab_snaking_config.maximum_speed)return 0;
    u64 saved_a=q(route,0x60),saved_z=q(route,0x58),saved_older=q(route,0x68);
    u64 a=saved_a,z=saved_z;
    if(!be_span(a,0x374)||!be_span(z,0x374)||a==z||
       be_point((void *)course,w((void *)a,0x8c))!=a||be_point((void *)course,w((void *)z,0x8c))!=z)return 0;
    u32 key;if(!successor((void *)a,&key)||be_point((void *)course,key)!=z)return 0;
    float delta[3],length2,projection;
    if(!hf_intent(physics,(void *)a,(void *)z,&vectors,0,delta,&length2,&projection))return 0;
    if(projection<0){
        u64 older=saved_older;
        if(!be_span(older,0x374)||older==a||older==z||
           be_point((void *)course,w((void *)older,0x8c))!=older||!be_edge((void *)older,w((void *)a,0x8c))||
           !hf_intent(physics,(void *)older,(void *)a,&vectors,1,delta,&length2,&projection))return 0;
        z=a;a=older;
    }else if(projection>length2)return 0;
    if(!sgr_future(physics,(void *)course,(void *)base,a,z,speed,
                   speed*ticks+64.f,&vectors)&&!shg_future(physics,(void *)course,(void *)base,a,z,speed,
                   speed*ticks+64.f,&vectors))return 0;
    return state->lock==1&&frame->last==now&&state->last==now&&q(driver,0x80)==course&&q(physics,0x110)==drift&&
           frame->producer==producer&&frame->clock==clock&&q(physics,0xf8)==role&&
           q((void *)role,0x28)==(u64)physics&&q((void *)drift,8)==role&&
           q(route,0x60)==saved_a&&q(route,0x58)==saved_z&&q(route,0x68)==saved_older&&
           be_course((u64)lab_module_start-LAB_MAIN_TO_MODULE,driver,route)==base&&
           q((void *)producer,0x70)==(u64)driver&&q((void *)producer,0x88)==(u64)route&&!b((void *)producer,0x41)&&
           q((void *)producer,0x80)==source&&!b((void *)source,0x80)&&
           q((void *)source,0x58)==(u64)driver&&q((void *)source,0x60)==(u64)route&&
           q((void *)producer,0x98)==packet&&w((void *)packet,4)==flags&&
           b((void *)role,0xd2)==1&&b((void *)role,0xd3)==1&&b((void *)role,0xe1)==1&&!b((void *)role,0xe7)&&
           !(w((void *)role,0x1cc)&0x01204018u)&&w((void *)role,0xa8)==index/sizeof(SnakeHopFrame)&&
           w((void *)clock,0x38)==6&&w((void *)clock,0x44)==now&&
           hf_scope(frame,state,physics,(void *)drift,now,w((void *)drift,0x10));
}
