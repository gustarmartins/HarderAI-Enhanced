/* Exact native query provenance for a retained orientation target. Keep all
 * original unit/contact/projection checks and the caller's route/history gates.
 * The observer consumer owns its race/actor lease and never writes physics. */
extern u32 lab_native_ground_target_retained(const void *,u64,u32,u32);
EXPORT const u32 lab_snaking_retained_ground_target_config[4]={1,1,0x189020,0};
static u32 ngs_retained_ground_target(const void *physics,const SnakeHopFrame *frame){
    if(!frame||frame->valid!=1||frame->reserved!=1||frame->physics!=(u64)physics||
       b(physics,0x200)!=1||b(physics,0x201)!=1||w(physics,0x26c)||b(physics,0x20a))return 0;
    u64 role=q(physics,0xf8);
    if(!ptr(role)||q((void *)role,0x28)!=(u64)physics)return 0;
    float displacement=f(physics,0x568);
    if(!finite(displacement)||displacement<0.f||(b((void *)role,0xd8)&&displacement<3.f))return 0;
    float normal[3],target[3],expected[3];
    if(!hf_unit(physics,0x17c,normal,.999f,1.001f)||
       !hf_unit(physics,0x32c,target,.999f,1.001f)||!nct_expected(physics,role,expected))return 0;
    return lab_native_ground_target_retained(physics,frame->clock,frame->start,frame->last);
}
