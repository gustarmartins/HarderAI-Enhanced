#include "flow_lane_native_point_contact_v305.h"
typedef unsigned long long U64;
typedef unsigned short U16;
static U64 q(U64 p,unsigned o){return *(const volatile U64 *)(p+o);}
static unsigned w(U64 p,unsigned o){return *(const volatile unsigned *)(p+o);}
static U16 h(U64 p,unsigned o){return *(const volatile U16 *)(p+o);}
static float f(U64 p,unsigned o){return *(const volatile float *)(p+o);}
static unsigned finite(float v){union{float f;unsigned u;} b={v};return (b.u&0x7f800000)!=0x7f800000;}
static unsigned ptr(U64 p,unsigned align){return p>=0x10000&&!(p&(align-1))&&p<(1ull<<39)-0x1000;}
static unsigned vector(U64 p,float v[3]){for(unsigned k=0;k<3;k++){v[k]=f(p,4*k);if(!finite(v[k]))return 0;}return 1;}
static float dot(const float a[3],const float b[3]){return (a[0]*b[0]+a[1]*b[1])+a[2]*b[2];}
static unsigned pair_mask(unsigned i,unsigned j){
    if(i==0)return j==1?0x87861e1fu:0x8666199fu;
    if(i==1)return j==0?0x9986661fu:0x981e607fu;
    return j==0?0xe067819fu:0xe01f807fu;
}
__attribute__((visibility("default")))
unsigned lab_flow_lane_point_contact(const FlowLaneMeshModel *m,unsigned index,
    const float center[3],float radius,FlowLanePointContact *out){
    if(!out)return 0;
    out->hit=out->category=out->branch=out->response_bit=0;
    out->corrected_depth=out->raw_depth=0;
    if(!m||!center||m->reserved||!ptr(m->model,8)||!ptr(m->vertices,4)||!ptr(m->normals,4)||
       !ptr(m->prisms,4)||!ptr(m->categories,4)||!m->vertex_count||m->vertex_count>65536||
       !m->normal_count||m->normal_count>65536||!m->prism_count||m->prism_count>65535||
       index>=m->prism_count||!finite(radius)||radius<=0||radius>128)return 0;
    if(q(m->model,0)!=m->vertices||q(m->model,8)!=m->normals||q(m->model,0x10)!=m->prisms||
       q(m->model,0x50)!=m->prisms||q(m->model,0x160)!=m->categories||w(m->model,0x58)!=m->prism_count)return 0;
    float thickness=f(m->model,0x168);U64 p=m->prisms+20ull*index;
    unsigned pi=h(p,4),ni=h(p,6),edge_index[3]={h(p,8),h(p,10),h(p,12)};
    float origin[3],normal[3],edge[3][3],length=f(p,0);
    if(!finite(thickness)||thickness<=0||!finite(length)||length<=0||pi>=m->vertex_count||ni>=m->normal_count||
       !vector(m->vertices+12ull*pi,origin)||!vector(m->normals+12ull*ni,normal))return 0;
    for(unsigned i=0;i<3;i++)if(edge_index[i]>=m->normal_count||!vector(m->normals+12ull*edge_index[i],edge[i]))return 0;
    float r[3];
    for(unsigned k=0;k<3;k++){
        if(!finite(center[k])||center[k]<-1.e8f||center[k]>1.e8f||origin[k]<-1.e8f||origin[k]>1.e8f)return 0;
        r[k]=center[k]-origin[k];
    }
    float d[3]={dot(r,edge[0]),dot(r,edge[1]),dot(r,edge[2])-length};
    float height=dot(r,normal),depth=radius-height;
    if(!finite(d[0])||!finite(d[1])||!finite(d[2])||!finite(height)||!finite(depth))return 0;
    out->raw_depth=depth;out->response_bit=1u<<(h(p,14)&31);
    if(d[0]>=radius||d[1]>=radius||d[2]>=radius||depth<=0||depth>=thickness)return 1;
    unsigned i,j;
    if(d[0]>d[1]){
        if(d[0]>d[2]){i=0;j=d[1]>=d[2]?1:2;}
        else{i=2;j=0;}
    }else if(d[1]<=d[2]){i=2;j=1;}
    else{i=1;j=d[2]<=d[0]?0:2;}
    unsigned original=w(m->categories,4*index),category;
    float corrected;
    if(d[i]<=0){
        out->branch=FLPC_INTERIOR;category=original&0x00060019u;corrected=depth;
    }else{
        float c=dot(edge[i],edge[j]),radius2=radius*radius,height2=height*height;
        category=original&pair_mask(i,j);
        if(d[i]*c>d[j]){
            out->branch=FLPC_EDGE;
            float available=radius2-d[i]*d[i];
            if(d[i]>height||available<height2)return 1;
            if(!finite(available)||available<0)return 0;
            category&=0x7e01f801u;corrected=__builtin_sqrtf(available)-height;
        }else{
            out->branch=FLPC_VERTEX;
            float denominator=c*c-1.f;
            if(denominator==0)return 1;
            float alpha=(d[j]*c-d[i])/denominator,beta=d[j]-c*alpha;
            float v[3];for(unsigned k=0;k<3;k++)v[k]=edge[i][k]*alpha+edge[j][k]*beta;
            float lateral2=dot(v,v),available=radius2-lateral2;
            if(available<=0||available<height2||lateral2>height2+height2)return 1;
            if(!finite(available)||!finite(lateral2))return 0;
            category&=0x01f807e1u;corrected=__builtin_sqrtf(available)-height;
        }
    }
    if(!finite(corrected))return 0;
    out->corrected_depth=corrected;
    if(category&&corrected>0){out->hit=1;out->category=category;}
    return 1;
}
