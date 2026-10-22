/* Selected lane and base path can describe the same corner with different
 * point spacing. Allow a useful bend paired with a near-straight segment.
 * Two straight views, opposing/sharp turns and all corridor vetoes stay out.
 */
EXPORT const u32 lab_corner_arc_mixed_bend_config[4]={1,1,2,0};
/* Locate the physical base predecessor when selected-node publication has
 * already advanced. This is a real canonical edge, not a projection clamp. */
static u32 cmb_base_lead(const ArcOwner *c,void *a,void *z,ArcEdge *lead,u32 *key){
    u32 previous=w(a,b(a,0x2e4)?0x38:0x60);
    u64 node=be_point((void *)c->base_course,previous);
    if(!be_span(node,0x374)||node==(u64)a||node==(u64)z||
       w((void *)node,0x8c)!=previous||!co_edge(c,(void *)node,a,1,1,lead,16))return 0;
    *key=previous;return 1;
}
