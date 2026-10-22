#include "flow_lane_native_binding_contact.h"
static unsigned w(flm_u64 p,unsigned o){return *(const volatile unsigned *)(p+o);}
static unsigned short h(flm_u64 p,unsigned o){return *(const volatile unsigned short *)(p+o);}
static float f(flm_u64 p,unsigned o){return *(const volatile float *)(p+o);}
static unsigned finite(double x){union{double f;flm_u64 u;}v={x};return (v.u&0x7ff0000000000000ull)!=0x7ff0000000000000ull;}
static double absd(double x){return x<0?-x:x;}
static unsigned clip(double a,double b,double *lo,double *hi){if(!b)return a>=0;double t=-a/b;if(b>0){if(t>*lo)*lo=t;}else if(t<*hi)*hi=t;return *lo<=*hi;}
unsigned lab_flow_lane_native_contact_span(const FlowLaneContactSpanInput *in,FlowLaneContactSpan *out){
    if(!out)return 0;*out=(FlowLaneContactSpan){0};
    if(!in||in->reserved||!finite(in->radius)||in->radius<=0||in->radius>32)return 0;
    /* Decode validates the entire borrowed model layout and every index. Its
     * reconstructed vertices are not used for the contact halfspaces. */
    FlowLaneMeshTriangle checked;
    if(!lab_flow_mesh_decode(&in->model,in->prism,&checked))return 0;
    flm_u64 prism=in->model.prisms+20ull*in->prism;
    float thickness=f(in->model.model,0x168),length=f(prism,0);
    if(!finite(thickness)||thickness<=0||thickness>1000||!finite(length)||length<=0)return 0;
    unsigned material=h(prism,14),category=w(in->model.categories,4*in->prism),bit=1u<<(material&31);
    if(!(bit&FLM_NATIVE_GROUND_MASK)||!(category&0x60019u))return 0;
    flm_u64 origin=in->model.vertices+12ull*h(prism,4);
    double lo=0,hi=1,largest=0;
    for(unsigned plane=0;plane<4;plane++){
        unsigned offset=plane<3?8+2*plane:6;
        flm_u64 normal=in->model.normals+12ull*h(prism,offset);
        double a=0,b=0,scale=1.+in->radius+thickness,n2=0;
        for(unsigned k=0;k<3;k++){
            double x=in->center_a[k],z=in->center_b[k],v=f(origin,4*k),n=f(normal,4*k);
            if(!finite(x)||!finite(z)||!finite(v)||!finite(n)||absd(x)>1.e8||absd(z)>1.e8||absd(v)>1.e8||absd(n)>2)return 0;
            a+=(x-v)*n;b+=(z-x)*n;n2+=n*n;
            double maximum=absd(x)>absd(z)?absd(x):absd(z);scale+=(maximum+absd(v))*absd(n);
        }
        if(n2<.999||n2>1.001)return 0;
        if(plane==2){a-=length;scale+=length;}
        /* 2^-20 exceeds gamma12 for binary32: interpolated float position,
         * subtraction, three products and two additions. Inputs are bounded
         * away from overflow; extra unit scale covers cancellation/subnormal
         * roundoff. This margin can refuse; it cannot create floor contact. */
        double error=scale*0x1p-20;if(error>largest)largest=error;
        if(plane<3){if(!clip(-a-error,-b,&lo,&hi))return 0;}
        else if(!clip(in->radius-error-a,-b,&lo,&hi)||!clip(a-(in->radius-thickness)-error,b,&lo,&hi))return 0;
    }
    if(!finite(lo)||!finite(hi)||lo<0||hi>1)return 0;
    out->status=1;out->response_bit=bit;out->category=category&0x60019u;
    out->begin=lo;out->end=hi;out->rounding_bound=largest;return 1;
}
