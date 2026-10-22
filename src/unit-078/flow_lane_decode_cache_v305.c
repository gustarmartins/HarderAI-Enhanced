#include "flow_lane_decode_cache_v305.h"
static unsigned rd(flm_u64 p){return *(const volatile unsigned *)p;}
static flm_u64 rq(flm_u64 p){return *(const volatile flm_u64 *)p;}
static unsigned equal(const void *a,const void *b,unsigned words){
    typedef unsigned Vector __attribute__((vector_size(16),aligned(4)));
    const volatile unsigned *x=a;const unsigned *y=b;unsigned i=0;
    for(;i+4<=words;i+=4){
        Vector d=*(const volatile Vector *)(x+i)^*(const Vector *)(y+i);
        if(d[0]|d[1]|d[2]|d[3])return 0;
    }
    for(;i<words;i++)if(x[i]!=y[i])return 0;return 1;
}
static unsigned matches(const FlowLaneMeshModel *m,const FlowLaneDecodedEntry *e){
    /* The miss validated all layout/count/bounds rules. On a hit their exact
     * operands must still equal that validated binding before child reads.
     * The model's derived counts need no repeated division by12 when those
     * endpoints and its prism count are unchanged. */
    if(!equal(m,&e->binding,sizeof(*m)/4))return 0;
    flm_u64 p=m->model;
    if(rq(p)!=m->vertices||rq(p+8)!=m->normals||rq(p+0x10)!=m->prisms||
       rq(p+0x18)!=m->tree||rq(p+0x50)!=m->prisms||
       rq(p+0x160)!=m->categories||rd(p+0x58)!=m->prism_count)return 0;
    const FlowLanePrismInputs *v=&e->input;
    if(!equal((const void *)(m->prisms+20ull*e->prism),v->prism,5))return 0;
    unsigned vertex=v->prism[1]&65535u;
    unsigned normals[4]={v->prism[1]>>16,v->prism[2]&65535u,
                         v->prism[2]>>16,v->prism[3]&65535u};
    if(!equal((const void *)(m->vertices+12ull*vertex),v->origin,3))return 0;
    for(unsigned i=0;i<4;i++)
        if(!equal((const void *)(m->normals+12ull*normals[i]),v->normal+3*i,3))return 0;
    return rd(m->categories+4ull*e->prism)==v->category&&rd(p+0x168)==v->thickness;
}
static unsigned inputs(const FlowLaneMeshModel *m,unsigned index,FlowLanePrismInputs *out){
    if(!lab_flow_mesh_model_same(m)||index>=m->prism_count)return 0;
    flm_u64 p=m->prisms+20ull*index;
    for(unsigned i=0;i<5;i++)out->prism[i]=rd(p+4*i);
    unsigned vertex=out->prism[1]&65535u;
    unsigned normals[4]={out->prism[1]>>16,out->prism[2]&65535u,
                         out->prism[2]>>16,out->prism[3]&65535u};
    if(vertex>=m->vertex_count)return 0;
    for(unsigned i=0;i<4;i++)if(normals[i]>=m->normal_count)return 0;
    for(unsigned i=0;i<3;i++)out->origin[i]=rd(m->vertices+12ull*vertex+4*i);
    for(unsigned n=0;n<4;n++)for(unsigned i=0;i<3;i++)
        out->normal[3*n+i]=rd(m->normals+12ull*normals[n]+4*i);
    out->category=rd(m->categories+4ull*index);
    out->thickness=rd(m->model+0x168);return 1;
}
static unsigned slot(FlowLaneDecodeCache *c,const FlowLaneMeshModel *m,unsigned index,
                     unsigned *at,FlowLaneDecodedEntry **entry){
    if(!c||!m||c->count>FLDC_ENTRIES)return 0;
    flm_u64 hash=(m->model>>3)^((flm_u64)index*0x9e3779b97f4a7c15ull);
    unsigned b=(unsigned)(hash^(hash>>32))&(FLDC_BUCKETS-1);
    for(unsigned n=0;n<FLDC_BUCKETS;n++,b=(b+1)&(FLDC_BUCKETS-1)){
        unsigned i=c->bucket[b];
        if(!i){*at=b;*entry=0;return 1;}
        if(i>c->count)return 0;
        FlowLaneDecodedEntry *e=&c->entry[i-1];
        if(e->model==m->model&&e->prism==index){*at=b;*entry=e;return 1;}
    }
    return 0;
}
void lab_flow_decode_cache_reset(FlowLaneDecodeCache *c){
    if(!c)return;
    c->geometry_ready=c->flow_count=0;
    c->count=c->hits=c->misses=c->changed=c->envelope_hits=c->envelope_misses=0;
    for(unsigned i=0;i<FLDC_BUCKETS;i++)c->bucket[i]=0;
}
unsigned lab_flow_mesh_cached_decode(const FlowLaneMeshModel *m,unsigned index,
                                    FlowLaneMeshTriangle *out,FlowLaneDecodeCache *c){
    if(!out||!c||c->geometry_ready>1)return 0;
    unsigned b;FlowLaneDecodedEntry *e;FlowLanePrismInputs before,after;
    if(!slot(c,m,index,&b,&e))return 0;
    if(e){
        if(c->geometry_ready?!equal(m,&e->binding,sizeof(*m)/4):!matches(m,e)){c->changed++;return 0;}
        *out=e->triangle;c->hits++;return 1;
    }
    if(c->geometry_ready||c->count==FLDC_ENTRIES||!inputs(m,index,&before))return 0;
    c->misses++;
    if(!lab_flow_mesh_decode(m,index,out)||!inputs(m,index,&after))return 0;
    if(!equal(&before,&after,sizeof(before)/4)){c->changed++;return 0;}
    e=&c->entry[c->count];e->model=m->model;e->prism=index;e->envelope_ready=0;
    e->binding=*m;e->input=before;e->triangle=*out;e->envelope=0;
    c->bucket[b]=++c->count;return 1;
}
double lab_flow_lane_cached_envelope(const FlowLaneMeshModel *m,unsigned index,
    const FlowLaneMeshTriangle *triangle,float thickness,FlowLaneDecodeCache *c){
    unsigned b;FlowLaneDecodedEntry *e;
    union {float f;unsigned u;} t={.f=thickness};
    if(!triangle||!c||c->geometry_ready>1||!slot(c,m,index,&b,&e)||!e)return -1;
    if((c->geometry_ready?!equal(m,&e->binding,sizeof(*m)/4):!matches(m,e))||!equal(triangle,&e->triangle,sizeof(*triangle)/4)||
       t.u!=e->input.thickness){c->changed++;return -1;}
    if(e->envelope_ready){c->envelope_hits++;return e->envelope;}
    c->envelope_misses++;e->envelope=lab_flow_lane_original_envelope(m,index,triangle,thickness);
    e->envelope_ready=1;return e->envelope;
}

#include "flow_lane_owned_point_impl_v305.h"
#include "flow_lane_owned_geometry_impl_v305.h"
