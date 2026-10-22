/* Included after the existing decode-cache implementation and the generated
 * exact point-contact arithmetic. All calculation data belongs to one callback. */
static FlowLaneDecodedEntry *flg_entry(const FlowLaneMeshModel *m,unsigned index,
                                      FlowLaneDecodeCache *c){
    unsigned at;FlowLaneDecodedEntry *e;
    if(!c||c->geometry_ready!=1||!slot(c,m,index,&at,&e)||!e||
       !equal(m,&e->binding,sizeof(*m)/4))return 0;
    return e;
}
const FlowLanePrismInputs *lab_flow_geometry_prism(const FlowLaneMeshModel *m,
    unsigned index,FlowLaneDecodeCache *c){
    FlowLaneDecodedEntry *e=flg_entry(m,index,c);return e?&e->input:0;
}
static unsigned flg_verify(FlowLaneDecodeCache *c){
    if(!c||!c->count||c->count>FLDC_ENTRIES||c->flow_count>FLDC_ENTRIES)return 0;
    for(unsigned i=0;i<c->count;i++)if(!matches(&c->entry[i].binding,&c->entry[i])){
        c->changed++;return 0;
    }
    return 1;
}
unsigned lab_flow_geometry_open(const FlowLaneMesh *mesh,const unsigned *pairs,
                                unsigned count,FlowLaneDecodeCache *c){
    if(!c)return 0;lab_flow_decode_cache_reset(c);
    if(!mesh||!pairs||!count||count>FLDC_ENTRIES||
       !mesh->model_count||mesh->model_count>FLM_MODELS)return 0;
    for(unsigned i=0;i<count;i++){
        unsigned model=pairs[2*i],index=pairs[2*i+1],at;FlowLaneDecodedEntry *e;
        if(model>=mesh->model_count)return 0;
        const FlowLaneMeshModel *m=&mesh->models[model];FlowLaneMeshTriangle triangle;
        if(!lab_flow_mesh_cached_decode(m,index,&triangle,c)||!slot(c,m,index,&at,&e)||!e)return 0;
        union{unsigned u;float f;} thickness={.u=e->input.thickness};
        double normal2=0;for(unsigned k=0;k<3;k++)normal2+=(double)triangle.normal[k]*triangle.normal[k];
        if((thickness.u&0x7f800000u)==0x7f800000u||thickness.f<=0||thickness.f>32||
           normal2<.999||normal2>1.001)return 0;
        if(lab_flow_lane_cached_envelope(m,index,&triangle,thickness.f,c)<0)return 0;
        if(((e->input.prism[3]>>16)&31)==13){
            if(c->flow_count==FLDC_ENTRIES)return 0;
            c->flow_index[c->flow_count++]=(unsigned)(e-c->entry);
        }
    }
    if(!flg_verify(c))return 0;c->geometry_ready=1;return 1;
}
unsigned lab_flow_geometry_close(FlowLaneDecodeCache *c){
    if(!c)return 0;unsigned ready=c->geometry_ready;c->geometry_ready=0;
    return ready==1&&flg_verify(c);
}
unsigned lab_flow_geometry_point(const FlowLaneMeshModel *m,unsigned index,
    const float center[3],float radius,FlowLanePointContact *out,FlowLaneDecodeCache *c){
    FlowLaneDecodedEntry *e=flg_entry(m,index,c);
    return flg_point_values(e?&e->input:0,center,radius,out);
}
unsigned lab_flow_geometry_flow_point(FlowLaneDecodeCache *c,unsigned ordinal,
    const float center[3],float radius,FlowLanePointContact *out){
    const FlowLanePrismInputs *input=0;
    if(c&&c->geometry_ready==1&&c->count<=FLDC_ENTRIES&&
       c->flow_count<=FLDC_ENTRIES&&ordinal<c->flow_count){
        unsigned index=c->flow_index[ordinal];
        if(index<c->count){input=&c->entry[index].input;
            if(((input->prism[3]>>16)&31)!=13)input=0;}
    }
    return flg_point_values(input,center,radius,out);
}

unsigned lab_flow_geometry_resume(FlowLaneDecodeCache *c){
    if(!c||c->geometry_ready||!flg_verify(c))return 0;
    c->hits=c->misses=c->changed=c->envelope_hits=c->envelope_misses=0;
    c->geometry_ready=1;return 1;
}
