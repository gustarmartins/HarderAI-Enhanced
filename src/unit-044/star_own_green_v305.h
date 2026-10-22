/* Opt-in held-Star coverage for an owned returning Green Shell. Included in
 * the qualified Star unit, after its pointer and existing nearby() helpers.
 * Existing threat priority,90-unit3D proximity, admission and item use remain.
 * RetailF58 decides lifecycle/contact/self-protection; no cached item pointer.
 */
#ifndef LAB_STAR_OWN_GREEN_V305_H
#define LAB_STAR_OWN_GREEN_V305_H

EXPORT const struct {u32 abi,max_pool_count,kind,native_filter;}
lab_star_own_green_config={1,64,0,0xf58};

EXPORT u32 lab_star_own_green_decision(const void *table,const void *race,
    const void *physics,u32 racer,u64 base){
    if(!ptr((u64)table)||!ptr((u64)race)||!ptr((u64)physics)||racer>=12||
       !base||(base&4095)||base>=(1ull<<39)-0x1400000)return 0;
    u64 pool=q(table,0);
    if(!ptr(pool)||q((void *)pool,0)!=base+0x11ab6e0||w((void *)pool,0x38)!=0)return 0;
    u32 count=w((void *)pool,0x40);u64 nodes=q((void *)pool,0x48);
    if(count>lab_star_own_green_config.max_pool_count||(count&&!ptr(nodes)))return 0;
    for(u32 i=0;i<count;i++){
        u64 item=q((void *)nodes,0x20*i+0x10);
        if(!ptr(item)||q((void *)item,0)!=base+0x11ab3a8||
           w((void *)item,0x58)!=0||(int)w((void *)item,0x50)<0||
           w((void *)item,0x5c)!=racer)continue;
        /* Preserve the existing shell policy's lifecycle3/4 scope. NativeF58
         * is additionally required; the policy does not expand to held shells. */
        u32 state=b((void *)item,0x71);
        if(state!=3&&state!=4)continue;
        u64 wrapper=q((void *)item,0x40),source=q((void *)item,0x48);
        if(!ptr(wrapper)||q((void *)wrapper,0)!=item||
           !ptr(q((void *)wrapper,8))||!ptr(source)||
           source!=q(race,0x50+8*racer)||q((void *)source,0)!=base+0x11b44f8)continue;
        u64 role=q((void *)source,0x48),body=q((void *)source,0x50);
        if(!ptr(role)||body!=(u64)physics||q((void *)role,0x28)!=body||
           q(physics,0xf8)!=role||w((void *)role,0xa8)!=racer)continue;
        /* The source proxy's pinned native virtual90 selects the actual
         * local/remote age;F58 also retains frame/contact/active-state checks.
         * Query synchronously in the already-qualified native item call. */
        if(!((u32(*)(const void *,const void *,u32,u32))(base+0xf58))(
                (const void *)0,(const void *)item,racer,racer))continue;
        if(nearby((void *)item,physics))return 1;
    }
    return 0;
}
#endif
