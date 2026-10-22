/* Private proposal: keep an owned straight-drift aim inside the route cone.
 * The existing BASE envelope, owner checks and packet turn budget still apply.
 * Only a local goal vector changes; no path, charge, ratio or primary writes.
 */
EXPORT const u32 lab_snaking_route_goal_config[4]={1,1,0,0};

extern const u8 lab_snaking_hop_frames[];
/* Borrow only a continuously qualified hop belonging to this exact update.
 * Real launch pads, flight, recovery, stale owners and different road frames
 * do not acquire heading assistance through this private proposal. */
static u32 srg_hop_scope(const void *producer,u64 driver,u64 physics,u64 drift,
                        u64 clock,u32 racer,u64 native_base){
    const u8 *s=lab_snaking_state+88*racer,*h=lab_snaking_hop_frames+88*racer;
    u32 now=w((void *)clock,0x44),start=w(s,44),elapsed=now-start;
    u32 air=w((void *)physics,0x26c),contact=b((void *)physics,0x200)|(b((void *)physics,0x201)<<1);
    i32 direction=(i32)w(s,52);
    if(b(producer,0x41)||q(s,0)!=driver||q(s,8)!=(u64)producer||q(s,16)!=clock||
       w(s,32)||w(s,36)!=now||w(s,40)!=1||!elapsed||elapsed>30||
       (direction!=-1&&direction!=1)||w((void *)drift,0x10)!=2||
       q(h,0)!=driver||q(h,8)!=(u64)producer||q(h,16)!=clock||
       q(h,24)!=q((void *)driver,0x80)||q(h,32)!=native_base||
       q(h,40)!=physics||q(h,48)!=drift||w(h,56)!=start||w(h,60)!=now||w(h,64)!=1||
       air>19||air>elapsed||(contact!=0&&contact!=3)||(contact==3&&air)||
       f((void *)physics,0x274)!=.05f*(float)air||w((void *)physics,0x5d8))return 0;
    float admission[3],actual[3],target[3];
    if(!unit(h,72,admission)||!unit((void *)physics,0x320,actual)||
       !unit((void *)physics,0x32c,target))return 0;
    return dot(admission,actual)>=.9995f&&dot(admission,target)>=.9995f;
}

static u32 srg_aim(const float *delta,const float *up,float *toward){
    float tangent[3],vertical=dot(delta,up);
    for(u32 i=0;i<3;i++)tangent[i]=delta[i]-vertical*up[i];
    float norm=dot(tangent,tangent);
    if(!finite(norm)||norm<64.f)return 0;
    norm=root(norm);for(u32 i=0;i<3;i++)tangent[i]/=norm;
    float forward=dot(toward,tangent),lateral[3],side2=0;
    if(!finite(forward)||forward<=0)return 0;
    for(u32 i=0;i<3;i++){lateral[i]=toward[i]-forward*tangent[i];side2+=lateral[i]*lateral[i];}
    /* Half of the existing sqrt(.95) heading cone leaves angular room for
     * native drift transients. Same construction as the adjacent-node aim. */
    float cosine=root(.95f),limit2=forward*forward*(1.f-cosine)/(1.f+cosine);
    if(!finite(side2)||!finite(limit2)||limit2<=0)return 0;
    if(side2>limit2){
        float scale=root(limit2/side2);
        for(u32 i=0;i<3;i++)toward[i]=forward*tangent[i]+scale*lateral[i];
    }
    return 1;
}
