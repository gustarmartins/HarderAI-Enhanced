#include "flow_lane_native_geometry_identity_v305.h"
typedef unsigned u32;
typedef unsigned long long u64;
typedef unsigned char u8;
typedef struct {u32 state[8];u64 bytes;u32 used;u8 block[64];} SHA;
static u32 rotate(u32 x,unsigned n){return (x>>n)|(x<<(32-n));}
static void block(SHA *s,const u8 *p){
    static const u32 k[64]={
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
    u32 w[64];for(unsigned i=0;i<16;i++)w[i]=((u32)p[4*i]<<24)|((u32)p[4*i+1]<<16)|((u32)p[4*i+2]<<8)|p[4*i+3];
    for(unsigned i=16;i<64;i++){u32 a=w[i-15],b=w[i-2];w[i]=w[i-16]+(rotate(a,7)^rotate(a,18)^(a>>3))+w[i-7]+(rotate(b,17)^rotate(b,19)^(b>>10));}
    u32 a=s->state[0],b=s->state[1],c=s->state[2],d=s->state[3],e=s->state[4],f=s->state[5],g=s->state[6],h=s->state[7];
    for(unsigned i=0;i<64;i++){
        u32 t=h+(rotate(e,6)^rotate(e,11)^rotate(e,25))+((e&f)^(~e&g))+k[i]+w[i];
        u32 u=(rotate(a,2)^rotate(a,13)^rotate(a,22))+((a&b)^(a&c)^(b&c));
        h=g;g=f;f=e;e=d+t;d=c;c=b;b=a;a=t+u;
    }
    s->state[0]+=a;s->state[1]+=b;s->state[2]+=c;s->state[3]+=d;s->state[4]+=e;s->state[5]+=f;s->state[6]+=g;s->state[7]+=h;
}
static SHA initial(void){return (SHA){.state={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19}};}
static void update(SHA *s,const u8 *p,unsigned count){
    s->bytes+=count;
    while(count){
        if(!s->used&&count>=64){block(s,p);p+=64;count-=64;continue;}
        unsigned take=64-s->used;if(take>count)take=count;
        for(unsigned i=0;i<take;i++)s->block[s->used+i]=p[i];s->used+=take;p+=take;count-=take;
        if(s->used==64){block(s,s->block);s->used=0;}
    }
}
static void finish(SHA *s,u8 *out){
    u64 bits=s->bytes*8;u8 end[128]={0x80};unsigned length=s->used<56?56-s->used:120-s->used;
    for(unsigned i=0;i<8;i++)end[length+i]=(u8)(bits>>(56-8*i));update(s,end,length+8);
    for(unsigned i=0;i<8;i++)for(unsigned j=0;j<4;j++)out[4*i+j]=(u8)(s->state[i]>>(24-8*j));
}
void lab_flow_lane_sha256(const u8 *p,unsigned count,u8 out[32]){SHA s=initial();update(&s,p,count);finish(&s,out);}
static u64 q(u64 p,unsigned offset){return *(const volatile u64 *)(p+offset);}
static u32 w(u64 p,unsigned offset){return *(const volatile u32 *)(p+offset);}
static unsigned same(const FlowLaneMesh *a,const FlowLaneMesh *b){
    const u8 *x=(const u8 *)a,*y=(const u8 *)b;for(unsigned k=0;k<sizeof(*a);k++)if(x[k]!=y[k])return 0;return 1;
}
static void word(SHA *s,u32 value){u8 bytes[4];for(unsigned k=0;k<4;k++)bytes[k]=(u8)(value>>(8*k));update(s,bytes,4);}
unsigned lab_flow_lane_geometry_identity(const FlowLaneMesh *mesh,u8 out[32]){
    if(!out)return 0;for(unsigned k=0;k<32;k++)out[k]=0;
    FlowLaneMesh now;
    if(!mesh||!lab_flow_mesh_open(mesh->course_data,&now)||!same(mesh,&now))return 0;
    SHA hash=initial();static const u8 prefix[]="HAI-FLOW-MESH-v1";update(&hash,prefix,sizeof(prefix));word(&hash,mesh->model_count);
    for(unsigned i=0;i<mesh->model_count;i++){
        const FlowLaneMeshModel *m=&mesh->models[i];u32 thickness=w(m->model,0x20),radius=w(m->model,0x48);
        if(q(mesh->table,8*i)!=m->model||thickness!=w(m->model,0x168))return 0;
        word(&hash,i);word(&hash,m->vertex_count);word(&hash,m->normal_count);word(&hash,m->prism_count);word(&hash,thickness);word(&hash,radius);
        update(&hash,(const u8 *)m->vertices,12*m->vertex_count);
        update(&hash,(const u8 *)m->normals,12*m->normal_count);
        update(&hash,(const u8 *)m->prisms,20*m->prism_count);
        if(thickness!=w(m->model,0x20)||thickness!=w(m->model,0x168)||radius!=w(m->model,0x48))return 0;
    }
    if(!lab_flow_mesh_open(mesh->course_data,&now)||!same(mesh,&now))return 0;
    finish(&hash,out);return 1;
}
unsigned lab_flow_lane_geometry_is_stock_bigblue(const FlowLaneMesh *mesh,u8 out[32]){
    static const u8 expected[32]={0x6e,0xe8,0xca,0x08,0x93,0x13,0x56,0xf9,0x8d,0xe9,0x25,0x5f,0x3c,0x3f,0x27,0x6d,0x65,0x9e,0x23,0x03,0xa9,0xab,0xc4,0x1b,0xa1,0x62,0xff,0x66,0x73,0xc0,0x01,0x0d};
    if(!lab_flow_lane_geometry_identity(mesh,out))return 0;
    unsigned diff=0;for(unsigned k=0;k<32;k++)diff|=out[k]^expected[k];return diff==0;
}
