/* A second private geometric forecast on an exact native ground target.
 * The ordinary chassis forecast always runs first. No road, input, pose,
 * charge, speed or contact field is changed. Both complete route views and
 * actual/pending motion must still fit the existing corridor predicates. */
EXPORT const u32 lab_corner_native_road_plane_config[4]={1,3,0x17c,0x32c};
static u32 cnr_up(const ArcOwner *c,float *up){
    const void *p=(void *)c->physics,*role=(void *)c->role;
    if(!ca_driving(c)||q(p,0xf8)!=c->role||q(role,0x28)!=c->physics||
       q(p,0x110)!=c->drift||q((void *)c->drift,8)!=c->role||
       w((void *)c->clock,0x38)!=6||w((void *)c->clock,0x44)!=c->now||
       b(p,0x200)!=1||b(p,0x201)!=1||w(p,0x26c)||b(p,0x20a)||f(p,0x274)!=0.f)return 0;
    float travel=f(p,0x568),norm=0.f,chassis[3];
    if(!finite(travel)||travel<0.f||(b(role,0xd8)&&travel<3.f)||
       !ca_unit((void *)p,0x14,chassis))return 0;
    for(u32 k=0;k<3;k++){
        if(w(p,0x17c+4*k)!=w(p,0x32c+4*k))return 0;
        up[k]=f(p,0x17c+4*k);if(!finite(up[k]))return 0;norm+=up[k]*up[k];
    }
    if(!finite(norm)||norm<.999f||norm>1.001f)return 0;
    norm=root_positive(norm);for(u32 k=0;k<3;k++)up[k]/=norm;
    /* Retain the established .99 directional alignment budget for a local
     * frame change. A distant/reversed surface cannot repair the forecast. */
    float agreement=ca_dot(chassis,up);
    return finite(agreement)&&agreement>0.f&&agreement*agreement>=.9801f;
}
