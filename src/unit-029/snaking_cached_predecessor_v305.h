/* The retail route cache has another historical point at +70. Accept that
 * point only as an unambiguous reciprocal predecessor in the current view.
 * This resolves a segment; it grants no geometry, motion or owner permission. */
static u64 scp_earlier(const void *route,void *course,u64 older,u64 a,u64 z){
    if(!be_span((u64)route,0x78)||older!=q(route,0x68)||
       a!=q(route,0x60)||z!=q(route,0x58)||
       !be_span(older,0x374)||be_point(course,w((void *)older,0x8c))!=older)return 0;
    u64 prior=q(route,0x70);
    if(!be_span(prior,0x374)||prior==older||prior==a||prior==z||
       be_point(course,w((void *)prior,0x8c))!=prior)return 0;
    u8 reverse=b((void *)older,0x2e4);
    if(reverse>1||b((void *)older,0x370)||b((void *)prior,0x2e4)!=reverse||
       w((void *)older,reverse?0x30:0x34)!=1||
       w((void *)older,reverse?0x38:0x60)!=w((void *)prior,0x8c)||
       !be_edge((void *)prior,w((void *)older,0x8c)))return 0;
    return prior;
}

static inline u64 scp_following(const void *route,void *course,u64 a,u64 z){
    if(z==q(route,0x68)){
        u64 current=q(route,0x60),next=q(route,0x58);
        return scp_earlier(route,course,z,current,next)==a?current:0;
    }
    return z==q(route,0x60)?q(route,0x58):q(route,0x50);
}
