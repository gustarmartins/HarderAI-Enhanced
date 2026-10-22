#include "flow_lane_native_edge_certificate_v305.h"
typedef struct {float lo,hi;unsigned valid;} B;
typedef union {float f;unsigned u;} F;
static unsigned finite(double x){union{double f;unsigned long long u;}v={x};return (v.u&0x7ff0000000000000ull)!=0x7ff0000000000000ull;}
static double absolute(double x){return x<0?-x:x;}
static float next(float x,int direction){F b={.f=x};if(!x){b.u=direction>0?1u:0x80000001u;return b.f;}b.u+=(x>0)==(direction>0)?1u:~0u;return b.f;}
static B bad(void){return (B){0,0,0};}
static B point(float x){return finite(x)?(B){x,x,1}:bad();}
static B bounds(double lo,double hi){
    float a=next((float)lo,-1),b=next((float)hi,1);
    return finite(a)&&finite(b)&&a<=b?(B){a,b,1}:bad();
}
static B plus(B a,B b){return a.valid&&b.valid?bounds((float)(a.lo+b.lo),(float)(a.hi+b.hi)):bad();}
static B minus(B a,B b){return a.valid&&b.valid?bounds((float)(a.lo-b.hi),(float)(a.hi-b.lo)):bad();}
static B times(B a,B b){
    if(!a.valid||!b.valid)return bad();float v[4]={a.lo*b.lo,a.lo*b.hi,a.hi*b.lo,a.hi*b.hi},lo=v[0],hi=v[0];
    for(unsigned k=0;k<4;k++){if(!finite(v[k]))return bad();if(v[k]<lo)lo=v[k];if(v[k]>hi)hi=v[k];}return bounds(lo,hi);
}
static B divide(B a,B b){
    if(!a.valid||!b.valid||(b.lo<=0&&b.hi>=0))return bad();float v[4]={a.lo/b.lo,a.lo/b.hi,a.hi/b.lo,a.hi/b.hi},lo=v[0],hi=v[0];
    for(unsigned k=0;k<4;k++){if(!finite(v[k]))return bad();if(v[k]<lo)lo=v[k];if(v[k]>hi)hi=v[k];}return bounds(lo,hi);
}
static B square(B a){
    if(!a.valid)return bad();float x=a.lo*a.lo,y=a.hi*a.hi,lo=x<y?x:y;
    if(a.lo<=0&&a.hi>=0)lo=0;return bounds(lo,x>y?x:y);
}
static B root(B a){
    if(!a.valid||a.lo<0)return bad();float lo,hi;
    __asm__("fsqrt %s0,%s1":"=w"(lo):"w"(a.lo));__asm__("fsqrt %s0,%s1":"=w"(hi):"w"(a.hi));return bounds(lo,hi);
}
static float dotf(const float *a,const float *b){return (a[0]*b[0]+a[1]*b[1])+a[2]*b[2];}
static unsigned unit(const float *n){double v=0;for(unsigned k=0;k<3;k++){if(!finite(n[k]))return 0;v+=(double)n[k]*n[k];}return v>=.999&&v<=1.001;}
static B plane(const FlowLaneContactSpanInput *in,const float *origin,const float *normal,float constant){
    double a=-constant,b=-constant,scale=1.+absolute(constant);
    for(unsigned k=0;k<3;k++){
        double x=in->center_a[k],z=in->center_b[k],o=origin[k],n=normal[k];
        if(!finite(x)||!finite(z)||!finite(o)||absolute(x)>1.e8||absolute(z)>1.e8||absolute(o)>1.e8)return bad();
        a+=(x-o)*n;b+=(z-o)*n;double maximum=absolute(x)>absolute(z)?absolute(x):absolute(z);
        scale+=(maximum+absolute(o))*absolute(n);
    }
    double error=scale*0x1p-20;return bounds((a<b?a:b)-error,(a>b?a:b)+error);
}
static unsigned possible(const B *d,unsigned i,unsigned j){
#define GT(a,b) (d[a].hi>d[b].lo)
#define LE(a,b) (d[a].lo<=d[b].hi)
#define GE(a,b) (d[a].hi>=d[b].lo)
#define LT(a,b) (d[a].lo<d[b].hi)
    if(i==0&&j==1)return GT(0,1)&&GT(0,2)&&GE(1,2);
    if(i==0&&j==2)return GT(0,1)&&GT(0,2)&&LT(1,2);
    if(i==2&&j==0)return GT(0,1)&&LE(0,2);
    if(i==2&&j==1)return LE(0,1)&&LE(1,2);
    if(i==1&&j==0)return LE(0,1)&&GT(1,2)&&LE(2,0);
    if(i==1&&j==2)return LE(0,1)&&GT(1,2)&&GT(2,0);
    return 0;
#undef GT
#undef LE
#undef GE
#undef LT
}
unsigned lab_flow_lane_native_edge_certificate(const FlowLaneContactSpanInput *in,FlowLaneEdgeCertificate *out,FlowLaneDecodeCache *cache){
    if(!out)return 0;*out=(FlowLaneEdgeCertificate){0};out->reason=FLEC_INPUT;
    if(!in||in->reserved||!finite(in->radius)||in->radius<=0||in->radius>32)return 0;
    FlowLaneMeshTriangle validated;
    if(!lab_flow_mesh_cached_decode(&in->model,in->prism,&validated,cache))return 0;
    const FlowLanePrismInputs *v=lab_flow_geometry_prism(&in->model,in->prism,cache);if(!v)return 0;
    unsigned material=v->prism[3]>>16,category=v->category;
    float thickness=*(const float *)&v->thickness,length=*(const float *)v->prism,origin[3],normal[4][3];
    if(!finite(thickness)||thickness<=0||thickness>1000||!finite(length)||length<=0||!((1u<<(material&31))&FLM_NATIVE_GROUND_MASK))return 0;
    B d[3],height;
    for(unsigned k=0;k<3;k++)origin[k]=((const float *)v->origin)[k];
    for(unsigned j=0;j<4;j++){
        unsigned ni=j<3?j+1:0;
        for(unsigned k=0;k<3;k++)normal[j][k]=((const float *)v->normal)[3*ni+k];
        if(!unit(normal[j]))return 0;
        B value=plane(in,origin,normal[j],j==2?length:0);
        if(!value.valid)return 0;if(j<3)d[j]=value;else height=value;
    }
    B radius=point(in->radius),depth=minus(radius,height);
    out->reason=FLEC_PLANES;
    if(!depth.valid||depth.lo<=0||depth.hi>=thickness)return 0;
    for(unsigned j=0;j<3;j++)if(d[j].hi>=in->radius)return 0;
    const unsigned order[6][3]={{0,1,0x87861e1f},{0,2,0x8666199f},{1,0,0x9986661f},{1,2,0x981e607f},{2,0,0xe067819f},{2,1,0xe01f807f}};
    for(unsigned pair=0;pair<6;pair++){
        unsigned i=order[pair][0],j=order[pair][1],cat=category&order[pair][2];
        if(!possible(d,i,j))continue;
        if(d[i].lo<=0){out->reason=FLEC_CATEGORY;if(!(category&0x60019u))return 0;out->branches|=1;}
        if(d[i].hi<=0)continue;
        B di={d[i].lo>0?d[i].lo:0,d[i].hi,1},dj=d[j],cosine=point(dotf(normal[i],normal[j]));
        B projection=minus(times(di,cosine),dj);
        if(!projection.valid)return 0;
        if(projection.hi>0){
            out->reason=FLEC_EDGE;
            if(!(cat&0x7e01f801u)||di.hi>height.lo)return 0;
            B available=minus(point(in->radius*in->radius),square(di)),height2=square(height);
            if(!available.valid||!height2.valid||available.lo<=0||available.lo<height2.hi)return 0;
            B corrected=minus(root(available),height);if(!corrected.valid||corrected.lo<=0)return 0;
            out->branches|=2;
        }
        if(projection.lo<=0){
            out->reason=FLEC_VERTEX;if(!(cat&0x01f807e1u))return 0;
            B denominator=point(cosine.lo*cosine.lo-1.f);
            if(!denominator.valid||denominator.lo==0)return 0;
            B alpha=divide(minus(times(dj,cosine),di),denominator),beta=minus(dj,times(cosine,alpha)),v[3];
            for(unsigned k=0;k<3;k++)v[k]=plus(times(point(normal[i][k]),alpha),times(point(normal[j][k]),beta));
            B lateral=plus(plus(square(v[0]),square(v[1])),square(v[2]));
            B available=minus(point(in->radius*in->radius),lateral),height2=square(height),angle=plus(height2,height2);
            if(!lateral.valid||!available.valid||!height2.valid||!angle.valid||available.lo<=0||available.lo<height2.hi||lateral.hi>angle.lo)return 0;
            B corrected=minus(root(available),height);if(!corrected.valid||corrected.lo<=0)return 0;
            out->branches|=4;
        }
    }
    if(!out->branches)return 0;out->status=1;out->reason=0;out->response_bit=1u<<(material&31);
    out->depth_minimum=depth.lo;out->depth_maximum=depth.hi;return 1;
}
