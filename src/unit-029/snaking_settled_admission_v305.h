/* Private snaking admission experiment. Native state and corner planning stay
 * unchanged. Only a stack copy consumed by 5C210 sees the shortened wait. */
EXPORT const u32 lab_snaking_settled_admission_config[6]={1,30,120,0x100,6,0};

EXPORT u64 lab_snaking_settled_other(u64 base,void *producer,void *role,u64 *local){
    u64 address=q(producer,0x90);
    if(!ptr(address)||b(role,0xd9))return address;
    void *other=(void *)address;
    if(q(other,0)!=base+0x11acd40||q(other,0x48)!=address||
       q(other,0x70)!=q(producer,0x70)||q(other,0x88)!=q(producer,0x80)||
       q(other,0x90)!=q(producer,0x88))return address;
    u32 age=w(other,0x44);
    if(b(other,0x40)!=6||b(other,0x41)!=0||b(other,0x42)>=6||b(other,0x43)||
       age<30||age>=120||w(other,0xd0)!=0x100)return address;
    if(q(other,0xd8)||q(other,0xe0)||q(other,0xe8)||w(other,0xf0)||
       b(other,0xf8)||b(other,0x100)||b(other,0x101))return address;
    /* The pinned predicate reads only D0 from this object. Do not pass this
     * partial shadow to any native state update, query or other callback. */
    for(u32 i=0;i<27;i++)local[i]=0;
    return (u64)local;
}
