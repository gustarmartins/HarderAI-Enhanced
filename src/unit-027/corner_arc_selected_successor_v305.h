
EXPORT const u32 lab_corner_arc_selected_successor_config[8]={1,12,1,15,0,0,0,0};
/* The cached edge can trail actual progress by one successor. Require its
 * complete canonical selected identity before deriving that one next edge.
 * No search, projection clamp, native pointer write or branch selection. */
static u32 css_next(const ArcOwner *c,u64 *a,u64 *z){
    u64 old_a=*a,old_z=*z;
    if(!be_span(old_a,0x374)||!be_span(old_z,0x374)||old_a==old_z||
       !simple_point((void *)old_a)||!simple_point((void *)old_z))return 0;
    u32 ka=w((void *)old_a,0x8c),kz=w((void *)old_z,0x8c);
    if(be_point((void *)c->course,ka)!=old_a||be_point((void *)c->course,kz)!=old_z||
       !be_edge((void *)old_a,kz))return 0;
    u32 next_key=w((void *)old_z,b((void *)old_z,0x2e4)?0x60:0x38);
    u64 next=be_point((void *)c->course,next_key);
    if(!next||next==old_a||next==old_z||!simple_point((void *)next)||
       !be_edge((void *)old_z,next_key))return 0;
    float delta[3],length2,projection;
    if(!co_intent(c,(void *)old_z,(void *)next,1.f,.0025f,1,
                  delta,&length2,&projection,15,0))return 0;
    *a=old_z;*z=next;return 1;
}
