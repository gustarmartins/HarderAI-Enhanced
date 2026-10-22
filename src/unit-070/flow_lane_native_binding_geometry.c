#include "flow_lane_native_binding_geometry.h"
typedef struct {double x[3];} V;
static unsigned finite(double d){union{double d;unsigned long long u;} b={.d=d};return (b.u&0x7ff0000000000000ull)!=0x7ff0000000000000ull;}
static V sub(V a,V b){V r;for(unsigned j=0;j<3;j++)r.x[j]=a.x[j]-b.x[j];return r;}
static V add(V a,V b){V r;for(unsigned j=0;j<3;j++)r.x[j]=a.x[j]+b.x[j];return r;}
static V mul(V a,double s){V r;for(unsigned j=0;j<3;j++)r.x[j]=a.x[j]*s;return r;}
static double dot(V a,V b){return a.x[0]*b.x[0]+a.x[1]*b.x[1]+a.x[2]*b.x[2];}
static V cross(V a,V b){V r={{a.x[1]*b.x[2]-a.x[2]*b.x[1],a.x[2]*b.x[0]-a.x[0]*b.x[2],a.x[0]*b.x[1]-a.x[1]*b.x[0]}};return r;}
static double clamp(double x){return x<0?0:x>1?1:x;}
static unsigned load(const float *p,V *v){if(!p)return 0;for(unsigned j=0;j<3;j++){v->x[j]=p[j];if(!finite(v->x[j])||v->x[j]<-1.e8||v->x[j]>1.e8)return 0;}return 1;}
static double norm(V v){return dot(v,v);}
static double segment_segment(V p,V q,V a,V b){
    V d=sub(q,p),e=sub(b,a),r=sub(p,a);double A=norm(d),E=norm(e),F=dot(e,r),s=0,t=0;
    if(A<=1.e-24&&E<=1.e-24)return norm(r);
    if(A<=1.e-24)t=clamp(F/E);
    else{
        double C=dot(d,r);
        if(E<=1.e-24)s=clamp(-C/A);
        else{
            double B=dot(d,e),den=A*E-B*B;
            if(den>1.e-24)s=clamp((B*F-C*E)/den);
            t=(B*s+F)/E;
            if(t<0){t=0;s=clamp(-C/A);}else if(t>1){t=1;s=clamp((B-C)/A);}
        }
    }
    return norm(sub(add(p,mul(d,s)),add(a,mul(e,t))));
}
static double point_triangle(V p,V a,V b,V c){
    V ab=sub(b,a),ac=sub(c,a),ap=sub(p,a);double d1=dot(ab,ap),d2=dot(ac,ap);
    if(d1<=0&&d2<=0)return norm(ap);
    V bp=sub(p,b);double d3=dot(ab,bp),d4=dot(ac,bp);
    if(d3>=0&&d4<=d3)return norm(bp);
    double vc=d1*d4-d3*d2;
    if(vc<=0&&d1>=0&&d3<=0)return norm(sub(p,add(a,mul(ab,d1/(d1-d3)))));
    V cp=sub(p,c);double d5=dot(ab,cp),d6=dot(ac,cp);
    if(d6>=0&&d5<=d6)return norm(cp);
    double vb=d5*d2-d1*d6;
    if(vb<=0&&d2>=0&&d6<=0)return norm(sub(p,add(a,mul(ac,d2/(d2-d6)))));
    double va=d3*d6-d5*d4;
    if(va<=0&&d4-d3>=0&&d5-d6>=0)return norm(sub(p,add(b,mul(sub(c,b),(d4-d3)/((d4-d3)+(d5-d6))))));
    double den=va+vb+vc;
    return norm(sub(p,add(a,add(mul(ab,vb/den),mul(ac,vc/den)))));
}
static unsigned triangle(const FlowLaneTriangle *t,V *a,V *b,V *c,V *n){
    if(!t||!load(t->vertex[0],a)||!load(t->vertex[1],b)||!load(t->vertex[2],c))return 0;
    *n=cross(sub(*b,*a),sub(*c,*a));return finite(norm(*n))&&norm(*n)>1.e-16;
}
static unsigned inside(V p,V a,V b,V c,V n){
    return dot(cross(sub(b,a),sub(p,a)),n)>=0&&dot(cross(sub(c,b),sub(p,b)),n)>=0&&dot(cross(sub(a,c),sub(p,c)),n)>=0;
}
unsigned lab_flow_lane_segment_triangle(const float *p,const float *q,const FlowLaneTriangle *t,double *out){
    if(!out)return 0;*out=-1;V a,b,c,n,x,z;
    if(!triangle(t,&a,&b,&c,&n)||!load(p,&x)||!load(q,&z))return 0;
    double d0=dot(sub(x,a),n),d1=dot(sub(z,a),n);
    if((d0<=0&&d1>=0)||(d0>=0&&d1<=0)){
        double den=d0-d1;
        if(den!=0&&inside(add(x,mul(sub(z,x),d0/den)),a,b,c,n)){*out=0;return 1;}
    }
    double result=point_triangle(x,a,b,c),v=point_triangle(z,a,b,c);if(v<result)result=v;
    v=segment_segment(x,z,a,b);if(v<result)result=v;
    v=segment_segment(x,z,b,c);if(v<result)result=v;
    v=segment_segment(x,z,c,a);if(v<result)result=v;
    if(!finite(result)||result<0)return 0;*out=result;return 1;
}
/* Intersect t in [lo,hi] with a+t*b >=0. No gap-merging epsilon can manufacture
 * support across a missing triangle. A higher-level proof may over-refuse. */
static unsigned clip(double a,double b,double *lo,double *hi){
    if(b==0)return a>=0;
    double x=-a/b;if(b>0){if(x>*lo)*lo=x;}else if(x<*hi)*hi=x;
    return *lo<=*hi;
}
unsigned lab_flow_lane_triangle_support(const float *p,const float *q,const FlowLaneTriangle *t,
    const float *up,float height,double interval[2]){
    if(!interval)return 2;interval[0]=interval[1]=0;V a,b,c,n,x,z,u;
    if(!triangle(t,&a,&b,&c,&n)||!load(p,&x)||!load(q,&z)||!load(up,&u)||!finite(height)||height<0)return 2;
    double u2=norm(u),N=dot(n,u);if(u2<.999||u2>1.001||N*N<1.e-12*norm(n)*u2)return 2;
    double h0=dot(sub(x,a),n)/N,h1=dot(sub(z,a),n)/N;
    V xa=sub(x,mul(u,h0)),za=sub(z,mul(u,h1)),d=sub(za,xa);double lo=0,hi=1;
    if(!clip(height-h0,h0-h1,&lo,&hi)||!clip(height+h0,h1-h0,&lo,&hi))return 0;
    V starts[3]={a,b,c},ends[3]={b,c,a};
    for(unsigned j=0;j<3;j++){
        V edge=sub(ends[j],starts[j]);
        if(!clip(dot(cross(edge,sub(xa,starts[j])),n),dot(cross(edge,d),n),&lo,&hi))return 0;
    }
    if(!finite(lo)||!finite(hi)||lo<0||hi>1)return 2;
    interval[0]=lo;interval[1]=hi;return 1;
}
