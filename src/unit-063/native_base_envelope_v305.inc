/* Native-base route provenance for the bounded private clearance helpers.
 * Centers/extents are not KCL boundaries. Caller-owned selected route intent
 * remains separate; no pointer here carries a native lifetime lease. */
static u32 be_span(u64 p,u64 n){
    return ptr(p)&&n&&n<=(1ull<<39)-p;
}

static u64 be_point(void *course,u32 key){
    if(!be_span((u64)course,0x28)||key==0xffffffffu)return 0;
    u32 group=key&0xffffu,index=key>>16,count=w(course,0x10),capacity=w(course,0x18);
    u64 groups=q(course,0x20);
    if(!count||count>capacity||capacity>65536||group>=count||
       !be_span(groups,48ull*capacity))return 0;
    void *row=(void *)(groups+48ull*group);
    u32 n=w(row,0x10),cap=w(row,0x18);u64 items=q(row,0x20);
    if(!n||n>cap||cap>65536||index>=n||!be_span(items,8ull*cap))return 0;
    u64 result=q((void *)items,8*index);
    if(!be_span(result,0x374)||w((void *)result,0x8c)!=key)return 0;
    /* Processed negative extents remain valid data. Clearance callers decide
     * which paired view supplies their positive radius. */
    for(u32 i=0;i<3;i++)if(!finite(f((void *)result,0xc+4*i))||
                            !finite(f((void *)result,0x24+4*i)))return 0;
    return result;
}

static u32 be_edge(void *from,u32 destkey){
    if(!be_span((u64)from,0x374)||destkey==0xffffffffu)return 0;
    u32 next=w(from,0x30),previous=w(from,0x34);u8 selected=b(from,0x2e4);
    if(next>10||previous>10||selected>1||b(from,0x370)||
       (selected?previous:next)!=1)return 0;
    return w(from,selected?0x60:0x38)==destkey;
}

static u64 be_course(u64 mainbase,void *driver,void *route){
    if((mainbase&4095)||mainbase>(1ull<<39)-0x1300000||
       !be_span((u64)driver,0xd1)||!be_span((u64)route,0x70))return 0;
    u64 table=q(driver,0),selected=q(driver,0x80);
    if(!be_span(table,0x190)||!be_span(selected,0x28)||
       q((void *)table,0x148)!=mainbase+0x72364||
       q((void *)table,0x178)!=mainbase+0x72384||
       q((void *)table,0x188)!=mainbase+0x71ea4||
       q(route,0x40)!=(u64)driver||q(route,0x48)!=(u64)driver+0x80)return 0;
    u64 root=q((void *)mainbase,0x12f6388);if(!be_span(root,8))return 0;
    u64 single=q((void *)root,0);if(!be_span(single,0x28))return 0;
    u64 link=q((void *)single,0x20);if(!be_span(link,0x10))return 0;
    u64 scene=q((void *)link,8);if(!be_span(scene,0x1b8)||!(b((void *)scene,0x180)&4))return 0;
    u64 system=q((void *)scene,0x1b0);if(!be_span(system,0x240))return 0;
    u64 race=q((void *)system,0x238);if(!be_span(race,0xb8))return 0;
    u64 group=q((void *)race,0xb0);if(!be_span(group,0x58)||
       q((void *)group,0x28)!=race||q(driver,0x60)!=group)return 0;
    u64 manager=q((void *)group,0x50);if(!ptr(manager))return 0;
    u8 special=b(driver,0xc4),variant=b(driver,0xd0);
    u64 alternate=0,resource=0,common=0,field=0;
    if(variant){
        if(!be_span(manager,0x59b0))return 0;
        alternate=q((void *)manager,0x59a8);
        if(alternate&&!be_span(alternate,0x28))return 0;
    }
    if(special&1){
        if(!be_span(manager,0x59b0))return 0;
        field=manager+(alternate?0x59a8:0x59a0);
    }else{
        resource=q((void *)system,0x220);if(!be_span(resource,0x198))return 0;
        if(alternate){
            if(!be_span(resource,0x9a8))return 0;
            field=resource+0x9a0;
        }else{
            common=q((void *)resource,0x190);if(!be_span(common,0xf0))return 0;
            field=common+0xe8;
        }
    }
    u64 expected=q((void *)field,0);if(!be_span(expected,0x28))return 0;
    /* The qualified native getter already runs in 5C210's width predicate.
     * Preflight every branch-dependent dereference before invoking it. */
    u64 resolved=((u64 (*)(void *))(mainbase+0x7bf5c4))(driver);
    if(resolved!=expected)return 0;
    /* Detect observed owner/selector changes around the native call. This is
     * structural readback, not an assertion of concurrent lifetime safety. */
    if(q((void *)mainbase,0x12f6388)!=root||q((void *)root,0)!=single||
       q((void *)single,0x20)!=link||q((void *)link,8)!=scene||
       !(b((void *)scene,0x180)&4)||q((void *)scene,0x1b0)!=system||
       q((void *)system,0x238)!=race||q((void *)race,0xb0)!=group||
       q((void *)group,0x28)!=race||q((void *)group,0x50)!=manager||
       q(driver,0x60)!=group||q(driver,0)!=table||q(driver,0x80)!=selected||
       q(route,0x40)!=(u64)driver||q(route,0x48)!=(u64)driver+0x80||
       b(driver,0xc4)!=special||b(driver,0xd0)!=variant||
       q((void *)table,0x148)!=mainbase+0x72364||
       q((void *)table,0x178)!=mainbase+0x72384||
       q((void *)table,0x188)!=mainbase+0x71ea4||q((void *)field,0)!=expected)return 0;
    if(variant&&q((void *)manager,0x59a8)!=alternate)return 0;
    if(!(special&1)&&(q((void *)system,0x220)!=resource||
       (!alternate&&q((void *)resource,0x190)!=common)))return 0;
    return resolved;
}
