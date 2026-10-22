/* Continue an owned grounded drift across a SELECTED endpoint-plane gap.
 * BASE containment, widths, native intent and the full charging horizon are
 * still checked by the caller. This changes no path, position, axis or charge.
 */
EXPORT const u32 lab_snaking_selected_joint_config[4]={1,2,4,0x534a0001u};

static u32 sjg_occupation(const void *physics,const void *older,
    const void *joint,const void *following,float speed,const HopVectors *v){
    float pd[3],pn,pp,nd[3],nn,np;
    if(!finite(speed)||speed<=2.f||speed>lab_snaking_config.maximum_speed||
       !hf_intent(physics,older,joint,v,0,pd,&pn,&pp)||
       !hf_intent(physics,joint,following,v,0,nd,&nn,&np)||
       !(pp>pn)||!(np<0.f)||
       !hf_edge(pd,pn,v)||!hf_edge(nd,nn,v)||!hnb_straight(pd,pn,nd,nn))return 0;
    float previous_offset[3],next_offset[3];
    for(u32 i=0;i<3;i++){
        float position=f(physics,0x2c+4*i);
        previous_offset[i]=position-f(older,0xc+4*i);
        next_offset[i]=position-f(joint,0xc+4*i);
        if(!finite(previous_offset[i])||!finite(next_offset[i]))return 0;
    }
    float travel=4.f*speed;
    for(u32 mode=0;mode<2;mode++){
        const float *axis=mode?v->pending:v->motion;
        HnbInterval previous,next;
        if(!hnb_interval(pd,pn,pp,previous_offset,axis,0.f,0.f,0,&previous)||
           !hnb_interval(nd,nn,np,next_offset,axis,0.f,0.f,0,&next)||
           !hnb_selected_join(pd,pn,nd,nn,next_offset,axis,&previous,&next)||
           !(hbs_contains(previous,travel)||hbs_contains(next,travel)||
             hnb_cover(previous,next,travel)))return 0;
    }
    return 1;
}

/* Snapshot values after the refusal; never follow these historical pointers
 * from a later host sample. All four objects were qualified by the caller.
 * This uses the existing record's twelve otherwise unused operand words. */
static u32 sjg_refusal(DfrProbe *probe,const void *physics,const void *older,
    const void *joint,const void *following,const HopVectors *v){
    float values[24];
    for(u32 i=0;i<3;i++){
        values[i]=v->motion[i];values[3+i]=v->pending[i];
        values[6+i]=v->up[i];values[9+i]=v->target_up[i];
        values[12+i]=f(physics,0x2c+4*i);
        values[15+i]=f(older,0xc+4*i);
        values[18+i]=f(joint,0xc+4*i);
        values[21+i]=f(following,0xc+4*i);
    }
    return dfr_fail(probe,12,0xffffffffu,0,0x534a0001u,
                    (u64)older,(u64)joint,0,0,values,24);
}
