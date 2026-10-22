/* Included after the exact native query and sealed-query implementations. */
#include "flow_lane_owned_query_v305.h"
static u32 flo_take(FlowLaneOwnedFlow *o,u32 *cursor,u64 native,u32 size,
                    u64 *original,u64 *owned){
    *original=*owned=0;if(!size)return 1;
    FlowLaneFlowSeal *s=&o->seal;
    if(*cursor>=s->span_count||*cursor>=FLOW_SEAL_SPANS)return 0;
    const FlowLaneSealedSpan *r=&s->spans[(*cursor)++];
    if(r->address!=native||r->size!=size||r->offset>s->bytes||
       size>s->bytes-r->offset||s->bytes>FLOW_SEAL_BYTES)return 0;
    *original=(u64)s->data+r->offset;*owned=(u64)o->data+r->offset;return 1;
}
static void flo_put(u64 p,u32 off,u64 value){*(u64 *)(p+off)=value;}
static u32 flo_clone(FlowLaneOwnedFlow *o){
    FlowLaneFlowSeal *s=&o->seal;const Course *c=(const Course *)s->course_words;
    if(s->ready!=1||s->bytes>FLOW_SEAL_BYTES||c->area_count>128||c->path_count>64)return 0;
    u32 cursor=0,n=0;const u8 *source=(const u8 *)s->data;u8 *target=(u8 *)o->data;
    for(;n+8<=s->bytes;n+=8)*(u64 *)(target+n)=*(const u64 *)(source+n);
    for(;n<s->bytes;n++)target[n]=source[n];
    u64 areas_original,areas,paths_original,paths,table_original,table,rows_original,rows;
    if(!flo_take(o,&cursor,c->areas,0x40,&areas_original,&areas)||
       !flo_take(o,&cursor,c->paths,0x28,&paths_original,&paths)||
       !flo_take(o,&cursor,c->area_table,8*c->area_count,&table_original,&table)||
       !flo_take(o,&cursor,c->path_rows,32*c->path_count,&rows_original,&rows))return 0;
    flo_put(areas,0x38,table);flo_put(paths,0x20,rows);
    for(u32 j=0;j<c->path_count;j++){
        u64 row=rows_original+32*j,native=q(row,0x18),points_original,points;
        u32 count=w(row,0x10);if(count<2||count>256)return 0;
        if(!flo_take(o,&cursor,native,0x68*count,&points_original,&points))return 0;
        flo_put(rows+32*j,0x18,points);
        for(u32 k=0;k<count;k++){
            u64 p=points_original+0x68*k;u32 nc=w(p,0x40);
            if(nc!=(k+1<count?1u:0u))return 0;
            if(nc){
                u64 link_original,link;
                if(!flo_take(o,&cursor,q(p,0x48),8,&link_original,&link)||
                   q(link_original,0)!=native+0x68*(k+1))return 0;
                flo_put(points+0x68*k,0x48,link);flo_put(link,0,points+0x68*(k+1));
            }else flo_put(points+0x68*k,0x48,0);
        }
    }
    for(u32 j=0;j<c->area_count;j++){
        u64 area_original,area,shape_original,shape,vt_original,vt;
        if(!flo_take(o,&cursor,q(table_original,8*j),0x60,&area_original,&area))return 0;
        u32 kind=w(area_original,0xc);if(kind>1)return 0;
        if(!flo_take(o,&cursor,q(area_original,0x58),kind?0x60:0x58,&shape_original,&shape)||
           !flo_take(o,&cursor,q(shape_original,0),16,&vt_original,&vt))return 0;
        flo_put(table,8*j,area);flo_put(area,0x58,shape);
        flo_put(shape,0,vt);flo_put(shape,0x38,shape+8);flo_put(shape,0x40,shape+0x2c);
    }
    u64 slot_original,slot,zero_original,zero;
    if(!flo_take(o,&cursor,s->frame.main_base+0x12f5148,8,&slot_original,&slot)||
       !flo_take(o,&cursor,q(slot_original,0),12,&zero_original,&zero)||cursor!=s->span_count)return 0;
    flo_put(slot,0,zero);
    o->area_table=table;o->path_rows=rows;o->zero=zero;
    o->area_count=c->area_count;o->path_count=c->path_count;return 1;
}
u32 lab_flow_owned_open(const FlowLaneNativeFrame *frame,FlowLaneOwnedFlow *o){
    if(!o)return FLF_INVALID;o->ready=0;o->reserved=0;
    u32 status=lab_flow_lane_seal_open(frame,&o->seal);
    if(status!=FLF_EVALUATED)return status;
    if(!flo_clone(o)){o->seal.ready=0;return FLF_COURSE;}
    if(!fls_matches(&o->seal)){o->seal.ready=0;return FLF_CHANGED;}
    o->ready=1;return FLF_EVALUATED;
}
u32 lab_flow_owned_query(const FlowLaneOwnedFlow *o,const float position[3],FlowLaneFlowResult *out){
    if(!out)return FLF_INVALID;clear(out,FLF_INVALID);
    if(!o||!position)return FLF_INVALID;
    if(o->ready!=1||o->reserved||o->seal.ready!=1){clear(out,FLF_CHANGED);return FLF_CHANGED;}
    float input[3],vector[3]={0,0,0};int mode=-1;u32 accepted=0;
    for(u32 k=0;k<3;k++){input[k]=position[k];if(!number(input[k]))return FLF_INVALID;}
    /* Faithful outer808924 iteration; the actual native containment and
     *8181F0 interpolation run on the owned graph. The first containing Area
     * wins even if its path has no match. Never advance to a later Area after
     * an interpolation miss. No current-course getter or native graph write. */
    for(u32 j=0;j<o->area_count;j++){
        u64 area=q(o->area_table,8*j),shape=q(area,0x58),vt=q(shape,0);
        typedef u32 (*Contains)(u64,const float *);
        if(((Contains)q(vt,0))(shape,input)&1){
            typedef int (*Interpolate)(u64,float *,const float *,int *);
            int result=((Interpolate)(o->seal.frame.main_base+0x8181f0))(
                o->path_rows+32ull*h(area,0x38),vector,input,&mode);
            accepted=result>=0;break;
        }
    }
    if(!accepted){
        for(u32 k=0;k<3;k++)((u32 *)vector)[k]=w(o->zero,4*k);
        mode=-1;
    }
    if((accepted&&(mode<0||mode>1))||(!accepted&&mode!=-1)){
        clear(out,FLF_OUTPUT);return FLF_OUTPUT;
    }
    for(u32 k=0;k<3;k++)if(!number(vector[k])||(!accepted&&vector[k]!=0)){
        clear(out,FLF_OUTPUT);return FLF_OUTPUT;
    }
    out->accepted=accepted;out->mode=mode;
    for(u32 k=0;k<3;k++)out->vector[k]=vector[k];
    out->status=FLF_EVALUATED;return FLF_EVALUATED;
}
u32 lab_flow_owned_close(FlowLaneOwnedFlow *o){
    if(!o)return FLF_INVALID;
    u32 ready=o->ready;o->ready=0;
    if(ready!=1){o->seal.ready=0;return FLF_CHANGED;}
    return lab_flow_lane_seal_close(&o->seal);
}
