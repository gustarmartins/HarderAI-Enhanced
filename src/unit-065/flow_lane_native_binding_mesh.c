#include "flow_lane_native_binding_mesh.h"
typedef unsigned char u8;
typedef unsigned short u16;
static flm_u64 q(flm_u64 p,flm_u32 o){return *(const volatile flm_u64 *)(p+o);}
static flm_u32 w(flm_u64 p,flm_u32 o){return *(const volatile flm_u32 *)(p+o);}
static u16 h(flm_u64 p,flm_u32 o){return *(const volatile u16 *)(p+o);}
static float f(flm_u64 p,flm_u32 o){return *(const volatile float *)(p+o);}
static unsigned ptr(flm_u64 p,flm_u32 align){return p>=0x10000&&!(p&(align-1))&&p<(1ull<<39)-0x1000;}
static unsigned finite(float v){union{float f;flm_u32 w;} u={v};return (u.w&0x7f800000)!=0x7f800000;}
static void zero(void *p,flm_u32 n){for(flm_u32 i=0;i<n;i++)((volatile u8 *)p)[i]=0;}
static unsigned load(flm_u64 p,float out[3]){
    for(flm_u32 i=0;i<3;i++){out[i]=f(p,4*i);if(!finite(out[i])||out[i]<-1.e8f||out[i]>1.e8f)return 0;}
    return 1;
}
static void cross(const float a[3],const float b[3],float out[3]){
    out[0]=a[1]*b[2]-a[2]*b[1];out[1]=a[2]*b[0]-a[0]*b[2];out[2]=a[0]*b[1]-a[1]*b[0];
}
static float dot(const float a[3],const float b[3]){return (a[0]*b[0]+a[1]*b[1])+a[2]*b[2];}
static flm_u32 model_open(flm_u64 p,FlowLaneMeshModel *out){
    if(!ptr(p,8))return 0;
    flm_u64 v=q(p,0),n=q(p,8),t=q(p,0x10),end=q(p,0x18),cat=q(p,0x160);
    flm_u32 count=w(p,0x58);
    if(!ptr(v,4)||!ptr(n,4)||!ptr(t,4)||!ptr(end,4)||!ptr(cat,4)||
       q(p,0x50)!=t||v>=n||n>=t||t>=end||!count||count>FLM_TOTAL_PRISMS||
       (n-v)%12||(t-n)%12||(end-t)!=(flm_u64)count*20||
       (n-v)/12>65536||(t-n)/12>65536)return 0;
    out->model=p;out->vertices=v;out->normals=n;out->prisms=t;out->tree=end;out->categories=cat;
    out->vertex_count=(flm_u32)((n-v)/12);out->normal_count=(flm_u32)((t-n)/12);
    out->prism_count=count;out->reserved=0;return 1;
}
static flm_u32 same(const FlowLaneMeshModel *v){
    FlowLaneMeshModel now;
    if(!v||v->reserved||!model_open(v->model,&now))return 0;
    return v->vertices==now.vertices&&v->normals==now.normals&&v->prisms==now.prisms&&
        v->tree==now.tree&&v->categories==now.categories&&v->vertex_count==now.vertex_count&&
        v->normal_count==now.normal_count&&v->prism_count==now.prism_count;
}
__attribute__((visibility("default")))
flm_u32 lab_flow_mesh_open(flm_u64 data,FlowLaneMesh *out){
    if(!out)return 0;zero(out,sizeof(*out));
    if(!ptr(data,8))return 0;
    flm_u64 header=q(data,0x40),table=q(data,0x80);
    if(!ptr(header,4)||!ptr(table,8)||w(header,0)!=0x202)return 0;
    flm_u32 count=w(header,0xc);
    if(!count||count>FLM_MODELS)return 0;
    out->course_data=data;out->header=header;out->table=table;out->model_count=count;
    for(flm_u32 i=0;i<count;i++){
        if(!model_open(q(table,8*i),&out->models[i]))return 0;
        out->total_prisms+=out->models[i].prism_count;
        if(out->total_prisms>FLM_TOTAL_PRISMS)return 0;
    }
    if(q(data,0x40)!=header||q(data,0x80)!=table||w(header,0xc)!=count)return 0;
    return 1;
}
__attribute__((visibility("default")))
flm_u32 lab_flow_mesh_decode(const FlowLaneMeshModel *m,flm_u32 index,FlowLaneMeshTriangle *out){
    if(!out)return 0;zero(out,sizeof(*out));
    if(!same(m)||index>=m->prism_count)return 0;
    flm_u64 p=m->prisms+20ull*index;
    flm_u32 pi=h(p,4),ni=h(p,6),n1=h(p,8),n2=h(p,10),n3=h(p,12);
    float length=f(p,0),edge1[3],edge2[3],edge3[3];
    if(pi>=m->vertex_count||ni>=m->normal_count||n1>=m->normal_count||
       n2>=m->normal_count||n3>=m->normal_count||!finite(length)||length<=0||
       !load(m->vertices+12ull*pi,out->triangle.vertex[0])||
       !load(m->normals+12ull*ni,out->normal)||!load(m->normals+12ull*n1,edge1)||
       !load(m->normals+12ull*n2,edge2)||!load(m->normals+12ull*n3,edge3))return 0;
    for(flm_u32 k=0;k<2;k++){
        float edge[3];cross(out->normal,k?edge1:edge2,edge);
        float denominator=dot(edge,edge3);
        if(!finite(denominator)||denominator==0)return 0;
        float scale=length/denominator;
        if(!finite(scale))return 0;
        for(flm_u32 j=0;j<3;j++){
            float v=out->triangle.vertex[0][j]+edge[j]*scale;
            if(!finite(v)||v<-1.e8f||v>1.e8f)return 0;
            out->triangle.vertex[k+1][j]=v;
        }
    }
    out->material=h(p,14);out->category=w(m->categories,4*index);
    out->response_bit=1u<<(out->material&31);out->prism_index=index;
    return 1;
}
__attribute__((visibility("default")))
flm_u32 lab_flow_mesh_next(const FlowLaneMesh *mesh,FlowLaneMeshCursor *cursor,FlowLaneMeshTriangle *out){
    if(!mesh||!cursor||!out||cursor->reserved||!mesh->model_count||mesh->model_count>FLM_MODELS||
       !mesh->total_prisms||mesh->total_prisms>FLM_TOTAL_PRISMS||cursor->model>mesh->model_count||
       cursor->emitted>mesh->total_prisms||!ptr(mesh->course_data,8)||
       q(mesh->course_data,0x40)!=mesh->header||q(mesh->course_data,0x80)!=mesh->table||
       w(mesh->header,0xc)!=mesh->model_count)return FLM_INVALID;
    flm_u32 expected=cursor->prism;
    for(flm_u32 i=0;i<cursor->model;i++)expected+=mesh->models[i].prism_count;
    if(expected!=cursor->emitted)return FLM_INVALID;
    if(cursor->model==mesh->model_count)return !cursor->prism&&cursor->emitted==mesh->total_prisms?FLM_END:FLM_INVALID;
    const FlowLaneMeshModel *m=&mesh->models[cursor->model];
    if(q(mesh->table,8*cursor->model)!=m->model||!lab_flow_mesh_decode(m,cursor->prism,out))return FLM_INVALID;
    out->model_index=cursor->model;cursor->emitted++;cursor->prism++;
    if(cursor->prism==m->prism_count){cursor->model++;cursor->prism=0;}
    return FLM_VALUE;
}

unsigned lab_flow_mesh_model_same(const FlowLaneMeshModel *m){return same(m);}
