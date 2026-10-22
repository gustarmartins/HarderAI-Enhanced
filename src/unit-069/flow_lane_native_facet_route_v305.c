#include "flow_lane_native_facet_route_v305.h"
#include "flow_lane_native_point_contact_v305.h"
static unsigned finite(double x){union{double f;unsigned long long u;}v={x};return (v.u&0x7ff0000000000000ull)!=0x7ff0000000000000ull;}
static double dot(const double *a,const double *b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
static double root(double x){double z;__asm__("fsqrt %d0,%d1":"=w"(z):"w"(x));return z;}
static unsigned unit(const float *a){double s=0;for(unsigned k=0;k<3;k++){if(!finite(a[k]))return 0;s+=(double)a[k]*a[k];}return s>=.998001&&s<=1.002001;}
static unsigned clip(double value,double slope,double *lo,double *hi){
    if(!finite(value)||!finite(slope))return 0;
    if(!slope)return value<=0;
    double bound=-value/slope;if(slope>0){if(bound<*hi)*hi=bound;}else if(bound>*lo)*lo=bound;
    return *lo<*hi;
}
static unsigned facet(const FlowLaneMesh *mesh,FlowLanePrismRef ref,const FlowLaneFacetInput *in,FlowLaneNativeFacet *out,FlowLaneDecodeCache *cache){
    if(ref.model>=mesh->model_count)return 0;
    const FlowLaneMeshModel *m=&mesh->models[ref.model];FlowLaneMeshTriangle triangle;
    if(!lab_flow_mesh_cached_decode(m,ref.prism,&triangle,cache))return 0;
    if(!(triangle.response_bit&FLM_NATIVE_GROUND_MASK)||!unit(triangle.normal))return 2;
    double n[3],o[3],up[3],a[3],delta[3];
    for(unsigned k=0;k<3;k++){n[k]=triangle.normal[k];o[k]=triangle.triangle.vertex[0][k];up[k]=in->up[k];a[k]=(double)in->a[k]-o[k];delta[k]=(double)in->b[k]-in->a[k];}
    double divisor=dot(n,up);if(divisor<=.8)return 2;
    double height=dot(a,n)/divisor,dh=dot(delta,n)/divisor,lo=0,hi=1;
    if(!clip(height-32,dh,&lo,&hi)||!clip(-2-height,-dh,&lo,&hi))return 2;
    for(unsigned k=0;k<3;k++){out->p0[k]=in->a[k]-height*up[k];out->velocity[k]=delta[k]-dh*up[k];}
    const FlowLanePrismInputs *v=lab_flow_geometry_prism(m,ref.prism,cache);if(!v)return 0;
    const float *normals=(const float *)v->normal;float length=*(const float *)v->prism;
    for(unsigned edge=0;edge<3;edge++){
        double e[3],position[3];
        for(unsigned k=0;k<3;k++){e[k]=normals[3*(edge+1)+k];position[k]=out->p0[k]-o[k];}
        if(!clip(dot(position,e)-(edge==2?length:0),dot(out->velocity,e),&lo,&hi))return 2;
    }
    out->begin=lo;out->end=hi;out->ref=ref;double scale=root(dot(n,n));
    for(unsigned k=0;k<3;k++)out->normal[k]=(float)(n[k]/scale);
    return 1;
}
static unsigned append(FlowLaneFacetWorkspace *out,const float *a,const float *b,FlowLanePrismRef ref,unsigned seam){
    if(out->segments==FLFR_SEGMENTS)return 0;
    FlowLaneFacetSegment *s=&out->segment[out->segments++];
    for(unsigned k=0;k<3;k++){s->a[k]=a[k];s->b[k]=b[k];if(!finite(a[k])||!finite(b[k]))return 0;}
    s->ref=ref;s->seam=seam;s->reserved=0;return 1;
}
static unsigned compatible(const FlowLaneNativeFacet *a,const FlowLaneNativeFacet *b,double t){
    double difference=0;
    for(unsigned k=0;k<3;k++){double d=a->p0[k]+t*a->velocity[k]-b->p0[k]-t*b->velocity[k];difference+=d*d;}
    /* Separated projected sheets are ambiguous. This bound rejects topology;
     * it is never used to close a seam or grant contact. */
    return difference<=1.e-6;
}
static unsigned stable(const FlowLaneMesh *mesh,const FlowLaneFacetWorkspace *out,unsigned index,double t,const FlowLaneFacetInput *in,FlowLaneDecodeCache *cache){
    const FlowLaneNativeFacet *candidate=&out->facet[index];float center[3];
    for(unsigned k=0;k<3;k++)center[k]=(float)(candidate->p0[k]+t*candidate->velocity[k]+candidate->normal[k]*in->local_offset_y);
    FlowLanePointContact own;
    if(!lab_flow_geometry_point(&mesh->models[candidate->ref.model],candidate->ref.prism,center,in->radius,&own,cache)||!own.hit)return 0;
    float depth=own.corrected_depth;unsigned winner=index;
    for(unsigned i=0;i<out->facets;i++){
        const FlowLaneNativeFacet *other=&out->facet[i];if(other->begin>t||other->end<t)continue;
        FlowLanePointContact value;
        if(!lab_flow_geometry_point(&mesh->models[other->ref.model],other->ref.prism,center,in->radius,&value,cache))return 0;
        if(value.hit&&value.corrected_depth>depth){winner=i;depth=value.corrected_depth;}
    }
    return compatible(candidate,&out->facet[winner],t);
}
static unsigned near(const FlowLaneMeshModel *m,unsigned index,const float *a,const float *b,float radius,FlowLaneDecodeCache *cache){
    const FlowLanePrismInputs *v=lab_flow_geometry_prism(m,index,cache);if(!v)return 2;
    const float *normals=(const float *)v->normal,*origin=(const float *)v->origin;
    float length=*(const float *)v->prism;
    for(unsigned j=0;j<4;j++){
        unsigned ni=j<3?j+1:0;double x=0,z=0;
        for(unsigned k=0;k<3;k++){double n=normals[3*ni+k],o=origin[k];x+=((double)a[k]-o)*n;z+=((double)b[k]-o)*n;}
        double constant=j==2?length:0;
        if(x-constant>radius+.01&&z-constant>radius+.01)return 0;
    }
    return 1;
}
static unsigned cover(const FlowLaneMesh *mesh,const FlowLaneFacetInput *in,FlowLaneFacetWorkspace *out,const FlowLaneFacetSegment *s,FlowLaneDecodeCache *cache){
    /* Depth-first dyadic intervals. Every accepted interval has one complete
     * native contact certificate; adjacent intervals share exact endpoints.
     * Stack and query counts are fixed and refusal is the only budget exit. */
    typedef struct {float a[3],b[3];unsigned depth;} Span;
    Span pending[12];unsigned used=1;pending[0].depth=0;
    for(unsigned k=0;k<3;k++){pending[0].a[k]=s->a[k];pending[0].b[k]=s->b[k];}
    while(used){
        Span span=pending[--used];unsigned accepted=0;
        for(unsigned i=0;i<out->facets;i++){
            FlowLanePrismRef ref=out->facet[i].ref;const FlowLaneMeshModel *m=&mesh->models[ref.model];
            unsigned close=near(m,ref.prism,span.a,span.b,in->radius,cache);
            if(close==2){out->status=FLFR_INVALID;return 0;}if(!close)continue;
            if(out->contact_queries>=4096){out->status=FLFR_BUDGET;return 0;}
            out->contact_queries++;FlowLaneContactSpanInput input={.model=*m,.prism=ref.prism,.radius=in->radius};
            for(unsigned k=0;k<3;k++){input.center_a[k]=span.a[k];input.center_b[k]=span.b[k];}
            FlowLaneEdgeCertificate cert;
            if(lab_flow_lane_native_edge_certificate(&input,&cert,cache)){accepted=1;out->branches|=cert.branches;out->contact_intervals++;break;}
        }
        if(accepted)continue;
        if(span.depth==10||used+2>12){out->status=FLFR_CONTACT_GAP;return 0;}
        Span left=span,right=span;left.depth++;right.depth++;
        for(unsigned k=0;k<3;k++){float middle=(float)(((double)span.a[k]+span.b[k])*.5);left.b[k]=middle;right.a[k]=middle;}
        pending[used++]=right;pending[used++]=left;
    }
    return 1;
}
unsigned lab_flow_lane_native_facet_route(const FlowLaneMesh *mesh,const FlowLanePrismRef *refs,unsigned count,const FlowLaneFacetInput *in,FlowLaneFacetWorkspace *out,FlowLaneDecodeCache *cache){
    if(!out)return 0;
    for(unsigned k=0;k<sizeof(*out);k++)((unsigned char *)out)[k]=0;
    if(!mesh||!refs||!count||count>4096||!mesh->model_count||mesh->model_count>FLM_MODELS||!in||in->reserved||!unit(in->up)||
       !finite(in->local_offset_y)||in->local_offset_y<=0||in->local_offset_y>32||!finite(in->radius)||in->radius<=0||in->radius>32)return 0;
    for(unsigned k=0;k<3;k++)if(!finite(in->a[k])||!finite(in->b[k]))return 0;
    unsigned boundaries=2;out->boundary[0]=0;out->boundary[1]=1;
    for(unsigned i=0;i<count;i++){
        FlowLaneNativeFacet temp;unsigned status=facet(mesh,refs[i],in,&temp,cache);
        if(!status)return 0;if(status==2)continue;
        if(out->facets==FLFR_FACETS){out->status=FLFR_BUDGET;return out->status;}
        out->facet[out->facets++]=temp;out->boundary[boundaries++]=temp.begin;out->boundary[boundaries++]=temp.end;
    }
    if(!out->facets){out->status=FLFR_ENDPOINT;return out->status;}
    for(unsigned i=1;i<boundaries;i++){
        double value=out->boundary[i];unsigned j=i;
        while(j&&out->boundary[j-1]>value){out->boundary[j]=out->boundary[j-1];j--;}
        out->boundary[j]=value;
    }
    unsigned spans=0,selected=FLFR_FACETS;double begin=0,end=0;float last[3]={0},last_normal[3];
    for(unsigned k=0;k<3;k++)last_normal[k]=in->up[k];
    /* Stream equal-facet intervals, retaining actual gaps. A later seam is
     * represented by a sphere-center segment and must certify continuously. */
    for(unsigned interval=0;interval<boundaries;interval++){
        double lo=interval+1<boundaries?out->boundary[interval]:1,hi=interval+1<boundaries?out->boundary[interval+1]:1;
        if(interval+1<boundaries&&lo==hi)continue;
        double t=(lo+hi)*.5,best=1.e300;unsigned pick=FLFR_FACETS;
        if(interval+1<boundaries)for(unsigned i=0;i<out->facets;i++){
            FlowLaneNativeFacet *v=&out->facet[i];if(v->begin>t||v->end<t)continue;
            if(!stable(mesh,out,i,t,in,cache))continue;
            if(pick!=FLFR_FACETS&&!compatible(v,&out->facet[pick],t)){out->status=FLFR_AMBIGUOUS;return out->status;}
            double dist=0;for(unsigned k=0;k<3;k++){double reference=in->a[k]+((double)in->b[k]-in->a[k])*t,d=v->p0[k]+v->velocity[k]*t-reference;dist+=d*d;}
            if(dist<best){best=dist;pick=i;}
        }
        if(pick==selected&&pick!=FLFR_FACETS&&lo==end){end=hi;continue;}
        if(selected!=FLFR_FACETS){
            FlowLaneNativeFacet *v=&out->facet[selected];float a[3],b[3];double align=0;
            for(unsigned k=0;k<3;k++){align+=(double)v->normal[k]*last_normal[k];a[k]=(float)(v->p0[k]+v->velocity[k]*begin+v->normal[k]*in->local_offset_y);b[k]=(float)(v->p0[k]+v->velocity[k]*end+v->normal[k]*in->local_offset_y);}
            if(align<.98){out->status=FLFR_BANK;return out->status;}
            if(!spans&&begin!=0){out->status=FLFR_ENDPOINT;return out->status;}
            if(spans){unsigned different=0;for(unsigned k=0;k<3;k++)if(a[k]!=last[k])different=1;
                if(different&&!append(out,last,a,v->ref,1)){out->status=FLFR_BUDGET;return out->status;}}
            if(!append(out,a,b,v->ref,0)){out->status=FLFR_BUDGET;return out->status;}
            for(unsigned k=0;k<3;k++){last[k]=b[k];last_normal[k]=v->normal[k];}spans++;
        }
        selected=pick;begin=lo;
        /* Keep end of last real facet across a gap for endpoint checking. */
        if(pick!=FLFR_FACETS)end=hi;
    }
    if(!spans||end!=1){out->status=FLFR_ENDPOINT;return out->status;}
    for(unsigned i=0;i<out->segments;i++){out->failed_segment=i;if(!cover(mesh,in,out,&out->segment[i],cache))return out->status;}
    out->failed_segment=0;out->status=FLFR_READY;return out->status;
}
