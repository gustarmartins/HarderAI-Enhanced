/* Native1848A0 determines airborne age from201. A retained200 flag does not
 * stop that counter. Admit this extra air case only with exact owned history.
 * No new lifetime, route, width, drift-state, charge or steering exception. */
EXPORT const u32 lab_snaking_takeoff_history_config[4]={1,2,0x201,19};
static u32 nth_air_contact(const void *physics,const SnakeHopFrame *frame){
    u32 current=b(physics,0x200);
    if(current>1||b(physics,0x201))return 0;
    if(!current)return 1; /* Existing contact00 branch stays unchanged. */
    return ngs_owned_history(physics,frame)&&w(physics,0x26c)>0&&
           w(physics,0x26c)<=19&&w((void *)frame->drift,0x10)==2;
}
