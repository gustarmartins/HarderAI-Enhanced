/* Included in prepared geometry after its exact key/reference helpers.
 * Group boxes use unexpanded extrema and the largest envelope. Adding the
 * radius at query time preserves the original operation order. IEEE monotonic
 * addition/subtraction makes an outside group an outside test for every member.
 * No contact, intended-floor or distance predicate is approximated. */
static unsigned flog_finite(double x){
    union {double f;flm_u64 u;} v={x};
    return (v.u&0x7ff0000000000000ull)!=0x7ff0000000000000ull;
}
static void flog_prepare(FlowPreparedGeometry *p,FlowLaneDecodeCache *c){
    p->obstacle_groups.ready=p->obstacle_groups.count=0;
    if(!p->ref_count||p->ref_count>FLDC_ENTRIES||c->geometry_ready!=1)return;
    for(unsigned i=0;i<p->ref_count;i++){
        unsigned model=p->pairs[2*i],index=p->pairs[2*i+1];
        if(model>=p->mesh.model_count)return;
        const FlowLanePrismInputs *v=lab_flow_geometry_prism(&p->mesh.models[model],index,c);
        if(!v)return;
        const FlowLaneDecodedEntry *e=(const FlowLaneDecodedEntry *)(
            (const unsigned char *)v-__builtin_offsetof(FlowLaneDecodedEntry,input));
        if(!e->envelope_ready||!flog_finite(e->envelope)||e->envelope<0)return;
        FlowObstacleGroup *g=&p->obstacle_groups.group[i/FLOG_WIDTH];
        if(i%FLOG_WIDTH==0){
            for(unsigned k=0;k<3;k++)g->minimum[k]=g->maximum[k]=e->triangle.triangle.vertex[0][k];
            g->extra=e->envelope;
        }
        for(unsigned j=0;j<3;j++)for(unsigned k=0;k<3;k++){
            double x=e->triangle.triangle.vertex[j][k];if(!flog_finite(x))return;
            if(x<g->minimum[k])g->minimum[k]=x;
            if(x>g->maximum[k])g->maximum[k]=x;
        }
        if(e->envelope>g->extra)g->extra=e->envelope;
    }
    p->obstacle_groups.count=(p->ref_count+FLOG_WIDTH-1)/FLOG_WIDTH;
    p->obstacle_groups.ready=1;
}
unsigned lab_flow_obstacle_groups_match(const FlowPreparedGeometry *p,
    const FlowLaneMesh *mesh,const unsigned *pairs,unsigned count,const FlowLaneDecodeCache *c){
    return p&&mesh&&pairs&&c&&p->state==FLPG_READY&&c->geometry_ready==1&&
        p->obstacle_groups.ready==1&&count&&count<=FLDC_ENTRIES&&
        p->obstacle_groups.count==(count+FLOG_WIDTH-1)/FLOG_WIDTH&&
        same(&p->mesh,mesh,sizeof(*mesh))&&references(p,pairs,count);
}
unsigned lab_flow_obstacle_group_skip(const FlowPreparedGeometry *p,unsigned index,
    const double low[3],const double high[3],float radius){
    if(!p||p->state!=FLPG_READY||p->obstacle_groups.ready!=1||!low||!high||
       !p->ref_count||p->ref_count>FLDC_ENTRIES||index>=p->ref_count||index%FLOG_WIDTH||
       p->obstacle_groups.count!=(p->ref_count+FLOG_WIDTH-1)/FLOG_WIDTH||
       !flog_finite(radius)||radius<=0||radius>32)return 0;
    const FlowObstacleGroup *g=&p->obstacle_groups.group[index/FLOG_WIDTH];
    if(!flog_finite(g->extra)||g->extra<0)return 0;
    double reach=radius+g->extra;unsigned outside=0;
    for(unsigned k=0;k<3;k++){
        if(!flog_finite(low[k])||!flog_finite(high[k])||low[k]>high[k]||
           !flog_finite(g->minimum[k])||!flog_finite(g->maximum[k])||
           g->minimum[k]>g->maximum[k])return 0;
        if(high[k]<g->minimum[k]-reach||low[k]>g->maximum[k]+reach)outside=1;
    }
    unsigned remaining=p->ref_count-index;
    return outside?(remaining<FLOG_WIDTH?remaining:FLOG_WIDTH):0;
}
