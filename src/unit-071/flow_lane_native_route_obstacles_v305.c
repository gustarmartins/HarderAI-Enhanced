#include "flow_lane_native_route_obstacles_v305.h"
static unsigned finite(double x){union{double f;unsigned long long u;}v={x};return (v.u&0x7ff0000000000000ull)!=0x7ff0000000000000ull;}
static unsigned same(FlowLanePrismRef a,FlowLanePrismRef b){return a.model==b.model&&a.prism==b.prism;}
static double dot(const double *a,const double *b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
static void cross(const double *a,const double *b,double *v){v[0]=a[1]*b[2]-a[2]*b[1];v[1]=a[2]*b[0]-a[0]*b[2];v[2]=a[0]*b[1]-a[1]*b[0];}
static double root(double x){double z;__asm__("fsqrt %d0,%d1":"=w"(z):"w"(x));return z;}
static double absolute(double x){return x<0?-x:x;}
static unsigned short h(flm_u64 p,unsigned offset){return *(const volatile unsigned short *)(p+offset);}
static float f(flm_u64 p,unsigned offset){return *(const volatile float *)(p+offset);}
double lab_flow_lane_original_envelope(const FlowLaneMeshModel *model,unsigned index,const FlowLaneMeshTriangle *t,float thickness){
    flm_u64 p=model->prisms+20ull*index;double normal[3],edges[3][3],reach=0,error=0,scale=1;
    for(unsigned k=0;k<3;k++)normal[k]=t->normal[k];
    for(unsigned j=0;j<3;j++)for(unsigned k=0;k<3;k++)edges[j][k]=f(model->normals+12ull*h(p,8+2*j),4*k);
    /* Exact back-plane extrusion direction at each prism corner is parallel
     * to the intersection of its two stored edge planes. Their tiny native
     * non-orthogonality to the face normal is included, never assumed zero. */
    for(unsigned i=0;i<3;i++)for(unsigned j=i+1;j<3;j++){
        double v[3];cross(edges[i],edges[j],v);double den=absolute(dot(normal,v));
        if(den<1.e-6)return -1;double length=thickness*root(dot(v,v))/den;
        if(!finite(length)||length>1024)return -1;if(length>reach)reach=length;
    }
    for(unsigned vertex=1;vertex<3;vertex++){
        double direction[3];cross(normal,edges[vertex==1?1:0],direction);
        double denominator=dot(direction,edges[2]);if(absolute(denominator)<1.e-6)return -1;
        double factor=f(p,0)/denominator,difference=0;
        for(unsigned k=0;k<3;k++){
            double exact=t->triangle.vertex[0][k]+direction[k]*factor;
            double d=exact-t->triangle.vertex[vertex][k];difference+=d*d;
            double magnitude=absolute(exact)+absolute(direction[k]*factor);if(magnitude>scale)scale=magnitude;
        }
        double distance=root(difference);if(distance>error)error=distance;
    }
    /* Bounds double construction error and separation arithmetic outwards.
     * It only enlarges the forbidden capsule; no support/gap predicate uses it. */
    return reach+error+scale*0x1p-30;
}
unsigned lab_flow_lane_route_obstacles(const FlowLaneMesh *mesh,const FlowLanePrismRef *refs,unsigned count,const FlowLaneFacetWorkspace *path,float radius,FlowLaneRouteObstacles *out,FlowLaneDecodeCache *cache,const FlowPreparedGeometry *prepared){
    if(!out)return 0;*out=(FlowLaneRouteObstacles){0};
    if(!mesh||!refs||!count||count>4096||!mesh->model_count||mesh->model_count>FLM_MODELS||!path||path->status!=FLFR_READY||
       !path->segments||path->segments>FLFR_SEGMENTS||!path->facets||path->facets>FLFR_FACETS||!finite(radius)||radius<=0||radius>32)return 0;
    double low[3],high[3];
    for(unsigned k=0;k<3;k++)low[k]=high[k]=path->segment[0].a[k];
    for(unsigned i=0;i<path->segments;i++)for(unsigned endpoint=0;endpoint<2;endpoint++)for(unsigned k=0;k<3;k++){
        double value=endpoint?path->segment[i].b[k]:path->segment[i].a[k];if(!finite(value))return 0;
        if(value<low[k])low[k]=value;if(value>high[k])high[k]=value;
    }
    unsigned grouped=lab_flow_obstacle_groups_match(prepared,mesh,(const unsigned *)refs,count,cache);
    for(unsigned index=0;index<count;index++){
        if(grouped&&index%FLOG_WIDTH==0){
            unsigned skip=lab_flow_obstacle_group_skip(prepared,index,low,high,radius);
            if(skip){index+=skip-1;continue;}
        }
        FlowLanePrismRef ref=refs[index];if(ref.model>=mesh->model_count)return 0;
        const FlowLaneMeshModel *model=&mesh->models[ref.model];FlowLaneMeshTriangle triangle;
        if(!lab_flow_mesh_cached_decode(model,ref.prism,&triangle,cache))return 0;out->decoded++;
        const FlowLanePrismInputs *v=lab_flow_geometry_prism(model,ref.prism,cache);if(!v)return 0;
        float thickness=*(const float *)&v->thickness;
        double normal2=0;for(unsigned k=0;k<3;k++)normal2+=(double)triangle.normal[k]*triangle.normal[k];
        if(!finite(thickness)||thickness<=0||thickness>32||normal2<.999||normal2>1.001)return 0;
        double extra=lab_flow_lane_cached_envelope(model,ref.prism,&triangle,thickness,cache);if(extra<0)return 0;
        double reach=radius+extra;
        double minimum[3],maximum[3];unsigned outside=0;
        for(unsigned k=0;k<3;k++){
            minimum[k]=maximum[k]=triangle.triangle.vertex[0][k];
            for(unsigned v=1;v<3;v++){double x=triangle.triangle.vertex[v][k];if(x<minimum[k])minimum[k]=x;if(x>maximum[k])maximum[k]=x;}
            if(high[k]<minimum[k]-reach||low[k]>maximum[k]+reach)outside=1;
        }
        if(outside)continue;
        for(unsigned si=0;si<path->segments;si++){
            const FlowLaneFacetSegment *s=&path->segment[si];unsigned far=0;
            for(unsigned k=0;k<3;k++){
                double mn=minimum[k],mx=maximum[k];
                double a=s->a[k],b=s->b[k];if(!finite(a)||!finite(b))return 0;
                if(a>b){double tmp=a;a=b;b=tmp;}
                if(b<mn-reach||a>mx+reach){far=1;break;}
            }
            if(far)continue;out->nearby++;
            const float *up=0;
            for(unsigned f=0;f<path->facets;f++)if(same(path->facet[f].ref,s->ref)){up=path->facet[f].normal;break;}
            if(!up)return 0;
            unsigned intended=(triangle.response_bit&0x2001u)!=0;double align=0;
            for(unsigned k=0;k<3;k++)align+=(double)triangle.normal[k]*up[k];
            if(align<.98)intended=0;
            if(intended)for(unsigned endpoint=0;endpoint<2;endpoint++){
                const float *p=endpoint?s->b:s->a;double height=0;
                for(unsigned k=0;k<3;k++)height+=((double)p[k]-triangle.triangle.vertex[0][k])*triangle.normal[k];
                if(height<0)intended=0;
            }
            if(intended)continue;
            if(out->distance_queries>=4096){out->status=3;return 3;}
            double distance;out->distance_queries++;
            if(!lab_flow_lane_segment_triangle(s->a,s->b,&triangle.triangle,&distance))return 0;
            if(distance<=reach*reach){out->status=2;out->obstacle_model=ref.model;out->obstacle_prism=ref.prism;out->segment=si;out->distance_squared=distance;return 2;}
        }
    }
    out->status=1;return 1;
}

#ifdef LAB_FLOW_PREPARED_PROBE_ONLY
unsigned lab_flow_lane_route_obstacles_full_scan(const FlowLaneMesh *mesh,const FlowLanePrismRef *refs,unsigned count,const FlowLaneFacetWorkspace *path,float radius,FlowLaneRouteObstacles *out,FlowLaneDecodeCache *cache){
    if(!out)return 0;*out=(FlowLaneRouteObstacles){0};
    if(!mesh||!refs||!count||count>4096||!mesh->model_count||mesh->model_count>FLM_MODELS||!path||path->status!=FLFR_READY||
       !path->segments||path->segments>FLFR_SEGMENTS||!path->facets||path->facets>FLFR_FACETS||!finite(radius)||radius<=0||radius>32)return 0;
    double low[3],high[3];
    for(unsigned k=0;k<3;k++)low[k]=high[k]=path->segment[0].a[k];
    for(unsigned i=0;i<path->segments;i++)for(unsigned endpoint=0;endpoint<2;endpoint++)for(unsigned k=0;k<3;k++){
        double value=endpoint?path->segment[i].b[k]:path->segment[i].a[k];if(!finite(value))return 0;
        if(value<low[k])low[k]=value;if(value>high[k])high[k]=value;
    }
    for(unsigned index=0;index<count;index++){
        FlowLanePrismRef ref=refs[index];if(ref.model>=mesh->model_count)return 0;
        const FlowLaneMeshModel *model=&mesh->models[ref.model];FlowLaneMeshTriangle triangle;
        if(!lab_flow_mesh_cached_decode(model,ref.prism,&triangle,cache))return 0;out->decoded++;
        const FlowLanePrismInputs *v=lab_flow_geometry_prism(model,ref.prism,cache);if(!v)return 0;
        float thickness=*(const float *)&v->thickness;
        double normal2=0;for(unsigned k=0;k<3;k++)normal2+=(double)triangle.normal[k]*triangle.normal[k];
        if(!finite(thickness)||thickness<=0||thickness>32||normal2<.999||normal2>1.001)return 0;
        double extra=lab_flow_lane_cached_envelope(model,ref.prism,&triangle,thickness,cache);if(extra<0)return 0;
        double reach=radius+extra;
        double minimum[3],maximum[3];unsigned outside=0;
        for(unsigned k=0;k<3;k++){
            minimum[k]=maximum[k]=triangle.triangle.vertex[0][k];
            for(unsigned v=1;v<3;v++){double x=triangle.triangle.vertex[v][k];if(x<minimum[k])minimum[k]=x;if(x>maximum[k])maximum[k]=x;}
            if(high[k]<minimum[k]-reach||low[k]>maximum[k]+reach)outside=1;
        }
        if(outside)continue;
        for(unsigned si=0;si<path->segments;si++){
            const FlowLaneFacetSegment *s=&path->segment[si];unsigned far=0;
            for(unsigned k=0;k<3;k++){
                double mn=minimum[k],mx=maximum[k];
                double a=s->a[k],b=s->b[k];if(!finite(a)||!finite(b))return 0;
                if(a>b){double tmp=a;a=b;b=tmp;}
                if(b<mn-reach||a>mx+reach){far=1;break;}
            }
            if(far)continue;out->nearby++;
            const float *up=0;
            for(unsigned f=0;f<path->facets;f++)if(same(path->facet[f].ref,s->ref)){up=path->facet[f].normal;break;}
            if(!up)return 0;
            unsigned intended=(triangle.response_bit&0x2001u)!=0;double align=0;
            for(unsigned k=0;k<3;k++)align+=(double)triangle.normal[k]*up[k];
            if(align<.98)intended=0;
            if(intended)for(unsigned endpoint=0;endpoint<2;endpoint++){
                const float *p=endpoint?s->b:s->a;double height=0;
                for(unsigned k=0;k<3;k++)height+=((double)p[k]-triangle.triangle.vertex[0][k])*triangle.normal[k];
                if(height<0)intended=0;
            }
            if(intended)continue;
            if(out->distance_queries>=4096){out->status=3;return 3;}
            double distance;out->distance_queries++;
            if(!lab_flow_lane_segment_triangle(s->a,s->b,&triangle.triangle,&distance))return 0;
            if(distance<=reach*reach){out->status=2;out->obstacle_model=ref.model;out->obstacle_prism=ref.prism;out->segment=si;out->distance_squared=distance;return 2;}
        }
    }
    out->status=1;return 1;
}

#endif
