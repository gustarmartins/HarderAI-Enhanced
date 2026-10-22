#include "flow_lane_native_cost_v305.h"
static unsigned finite(float v){union{float f;unsigned u;}x={v};return (x.u&0x7fffffffu)<0x7f800000u;}
static float absolute(float v){return v<0?-v:v;}
float lab_flow_lane_native_cost_arrival(float speed,const float *previous,const FlowLaneCostPath *path){
    if(!previous||!path||!finite(speed)||speed<=0||!finite(path->offset)||!path->count||path->count>FLOW_NATIVE_COST_SEGMENTS)return -1;
    float ends[FLOW_NATIVE_COST_SEGMENTS],total=0,flow[3];
    for(unsigned k=0;k<3;k++){if(!finite(previous[k]))return -1;flow[k]=previous[k];}
    for(unsigned i=0;i<path->count;i++){
        const FlowLaneCostSegment *s=&path->segments[i];float norm=0;
        if((s->flags&15)!=15||(s->flags&~31u)||!finite(s->distance)||s->distance<=0)return -1;
        for(unsigned k=0;k<3;k++){
            if(!finite(s->direction[k])||!finite(s->flow[k])||(!(s->flags&16)&&s->flow[k]!=0))return -1;
            norm+=s->direction[k]*s->direction[k];
        }
        if(!finite(norm)||norm<.999f||norm>1.001f||!finite(total+s->distance))return -1;
        total+=s->distance;ends[i]=total;
    }
    float scale=.1f*(speed>=6?.7f:1.f-.3f*speed/6.f),progress=0;
    float exit_blend=speed<=1?.05f:speed>=3?1.f:.05f+.5f*(.95f*(speed-1.f));
    for(unsigned tick=0;tick<240;tick++){
        unsigned index=0;while(index+1<path->count&&progress>=ends[index])index++;
        const FlowLaneCostSegment *s=&path->segments[index];float along=0;unsigned active=0;
        float blend=(s->flags&16)?.05f:exit_blend;
        for(unsigned k=0;k<3;k++){flow[k]+=blend*(s->flow[k]-flow[k]);active|=absolute(flow[k])>.5f;along+=flow[k]*s->direction[k];}
        float step=speed+(active?scale*along:0);
        if(!finite(step)||!finite(progress+step))return -1;
        if(step>0&&progress+step>=total)return tick+(total-progress)/step;
        progress+=step;
    }
    return 241.f;
}
unsigned lab_flow_lane_native_cost_rank(float speed,const float *previous,const FlowLaneCostPath *paths,unsigned count,FlowLaneCostResult *out){
    if(out)*out=(FlowLaneCostResult){0,-1,-1,0};
    if(!out||!paths||!count||count>7)return 0;
    float baseline=lab_flow_lane_native_cost_arrival(speed,previous,&paths[0]);
    out->baseline_ticks=out->selected_ticks=baseline;if(baseline<0)return 0;
    unsigned chosen=0;float best=baseline;
    for(unsigned i=1;i<count;i++){
        float t=lab_flow_lane_native_cost_arrival(speed,previous,&paths[i]);
        if(t>=0&&t<best&&baseline-t>=2.f){chosen=i;best=t;}
    }
    out->choice=chosen;out->selected_ticks=best;out->gain_ticks=baseline-best;return chosen;
}
