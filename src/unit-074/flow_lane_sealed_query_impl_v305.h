/* Included after the hash-pinned native-flow query translation unit. Reuses
 * its exact frame/course validators and actual80C65C call/output contract. */
#include "flow_lane_sealed_query_v305.h"
_Static_assert(sizeof(Course)==sizeof(((FlowLaneFlowSeal *)0)->course_words),"Course seal ABI");
static u32 fls_copy(FlowLaneFlowSeal *s,u64 address,u32 size){
    if(!size)return 1;
    if(address<0x10000||address>(1ull<<39)-size||s->span_count>=FLOW_SEAL_SPANS)return 0;
    u32 offset=(s->bytes+7u)&~7u;
    if(offset>FLOW_SEAL_BYTES||size>FLOW_SEAL_BYTES-offset)return 0;
    FlowLaneSealedSpan *r=&s->spans[s->span_count++];
    r->address=address;r->size=size;r->offset=offset;s->bytes=offset+size;
    unsigned char *to=(unsigned char *)s->data+offset;
    const volatile unsigned char *from=(const volatile unsigned char *)address;
    for(u32 k=0;k<size;k++)to[k]=from[k];return 1;
}
static u32 fls_course(FlowLaneFlowSeal *s,const Course *c){
    /* Parent pointer/count regions precede all borrowed child regions. A
     * changed binding is rejected before following its retained child. */
    if(!fls_copy(s,c->areas,0x40)||!fls_copy(s,c->paths,0x28)||
       !fls_copy(s,c->area_table,8*c->area_count)||!fls_copy(s,c->path_rows,32*c->path_count))return 0;
    for(u32 j=0;j<c->path_count;j++){
        u64 row=c->path_rows+32*j,pts=q(row,0x18);u32 count=w(row,0x10);
        if(count<2||count>256||!span(pts,0x68*count)||!fls_copy(s,pts,0x68*count))return 0;
        for(u32 k=0;k<count;k++){
            u64 p=pts+0x68*k;u32 nc=w(p,0x40);
            if(nc!=(k+1<count?1u:0u))return 0;
            if(nc&&(!span(q(p,0x48),8)||!fls_copy(s,q(p,0x48),8)))return 0;
        }
    }
    for(u32 j=0;j<c->area_count;j++){
        u64 a=q(c->area_table,8*j);if(!span(a,0x60)||!fls_copy(s,a,0x60))return 0;
        u32 shape=w(a,0xc);u64 p=q(a,0x58);
        if(shape>1||!span(p,0x60)||!fls_copy(s,p,shape?0x60:0x58))return 0;
        u64 vt=q(p,0);if(!span(vt,16)||!fls_copy(s,vt,16))return 0;
    }
    u64 slot=s->frame.main_base+0x12f5148,zero=q(slot,0);
    return zero>=0x10000&&!(zero&3)&&zero<(1ull<<39)-12&&fls_copy(s,slot,8)&&fls_copy(s,zero,12);
}
static u32 fls_matches(const FlowLaneFlowSeal *s){
    if(s->ready!=1||s->reserved||s->span_count>FLOW_SEAL_SPANS||s->bytes>FLOW_SEAL_BYTES)return 0;
    Course current;if(!frame(&s->frame,&current)||!same((const Course *)s->course_words,&current))return 0;
    for(u32 i=0;i<s->span_count;i++){
        const FlowLaneSealedSpan *r=&s->spans[i];
        if(r->offset>s->bytes||r->size>s->bytes-r->offset||r->address<0x10000||r->address>(1ull<<39)-r->size)return 0;
        const unsigned char *saved=(const unsigned char *)s->data+r->offset;
        const volatile unsigned char *actual=(const volatile unsigned char *)r->address;
        u32 n=0;
        if(!(r->address&7)&&!(r->offset&7)){
            /* Compare four complete words per branch. These are exact
             * in-range loads, including padding already saved by open;
             * no hash, epsilon or read beyond the borrowed span. */
            for(;n+32<=r->size;n+=32){
                const volatile u64 *a=(const volatile u64 *)(actual+n);
                const u64 *b=(const u64 *)(saved+n);
                u64 changed=(a[0]^b[0])|(a[1]^b[1])|(a[2]^b[2])|(a[3]^b[3]);
                if(changed)return 0;
            }
            for(;n+8<=r->size;n+=8)
                if(*(const volatile u64 *)(actual+n)!=*(const u64 *)(saved+n))return 0;
        }
        for(;n<r->size;n++)if(actual[n]!=saved[n])return 0;
    }
    return 1;
}
u32 lab_flow_lane_seal_open(const FlowLaneNativeFrame *input,FlowLaneFlowSeal *s){
    if(!s)return FLF_INVALID;s->ready=0;s->span_count=0;s->bytes=0;s->reserved=0;
    if(!input)return FLF_INVALID;
    Course before,after;if(!frame(input,&before))return FLF_INVALID;
    if(!course(input,&before))return FLF_COURSE;
    s->frame=*input;*(Course *)s->course_words=before;
    if(!fls_course(s,&before))return FLF_COURSE;
    if(!frame(input,&after)||!same(&before,&after)||!course(input,&after))return FLF_CHANGED;
    s->ready=1;
    if(!fls_matches(s)){s->ready=0;return FLF_CHANGED;}
    return FLF_EVALUATED;
}
u32 lab_flow_lane_sealed_query(FlowLaneFlowSeal *s,const float position[3],FlowLaneFlowResult *out){
    if(!out)return FLF_INVALID;clear(out,FLF_INVALID);
    if(!s||!position)return FLF_INVALID;
    float input[3];for(u32 k=0;k<3;k++){input[k]=position[k];if(!number(input[k]))return FLF_INVALID;}
    if(!fls_matches(s)){s->ready=0;clear(out,FLF_CHANGED);return FLF_CHANGED;}
    float vector[3]={0,0,0};int mode=-1;
    typedef u32 (*Native)(float *,const float *,int *);
    u32 accepted=((Native)(s->frame.main_base+0x80c65c))(vector,input,&mode);
    if(!fls_matches(s)){s->ready=0;clear(out,FLF_CHANGED);return FLF_CHANGED;}
    if(accepted>1||(accepted&&(mode<0||mode>1))||(!accepted&&mode!=-1)){
        clear(out,FLF_OUTPUT);return FLF_OUTPUT;
    }
    for(u32 k=0;k<3;k++)if(!number(vector[k])||(!accepted&&vector[k]!=0)){
        clear(out,FLF_OUTPUT);return FLF_OUTPUT;
    }
    out->accepted=accepted;out->mode=mode;for(u32 k=0;k<3;k++)out->vector[k]=vector[k];
    out->status=FLF_EVALUATED;return FLF_EVALUATED;
}
u32 lab_flow_lane_seal_close(FlowLaneFlowSeal *s){
    if(!s)return FLF_INVALID;u32 ok=fls_matches(s);s->ready=0;
    return ok?FLF_EVALUATED:FLF_CHANGED;
}
