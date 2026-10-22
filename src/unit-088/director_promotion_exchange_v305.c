#include "director_promotion_exchange_v305.h"
typedef unsigned char dx_u8;
static dx_u32 ptr(dx_u64 p){return p>=0x10000&&!(p&7)&&p<(1ull<<39)-0x4000;}
static dx_u64 q(dx_u64 p,dx_u32 off){return *(const volatile dx_u64 *)(p+off);}
static dx_u32 w(dx_u64 p,dx_u32 off){return *(const volatile dx_u32 *)(p+off);}
static dx_u8 b(dx_u64 p,dx_u32 off){return *(const volatile dx_u8 *)(p+off);}
dx_u32 lab_director_promotion_exchange_v305(dx_u64 base,void *group,dx_u32 source_racer,
    dx_u32 target_front_slot,PromotionExchangeResult *out){
    PromotionExchangeResult result={0};if(!out)return DX_REFUSED;
    dx_u64 g=(dx_u64)group;
    if(!base||(base&4095)||base>=(1ull<<39)-0x1400000||!ptr(g)||source_racer>=12||target_front_slot>=6||
       q(g,0)!=base+0x11ad808||w(g,0x58)!=12||w(g,0x5c)!=12){*out=result;return DX_REFUSED;}
    dx_u64 manager=q(g,0x68),drivers=q(g,0x60),race=q(g,0x28);
    if(!ptr(manager)||!ptr(drivers)||!ptr(race)||q(race,0)!=base+0x11b4308||q(race,0xb0)!=g||
       q(manager,0)!=base+0x11ae550||q(manager,0x28)!=g||w(manager,0x98)!=12||w(manager,0x9c)!=12||
       w(manager,0x228)!=11||w(manager,0x22c)!=1){*out=result;return DX_REFUSED;}
    dx_u64 wrappers=q(manager,0xa0),parameters=q(manager,0x78);
    if(!ptr(wrappers)||!ptr(parameters)){*out=result;return DX_REFUSED;}
    dx_u64 roster[12],subgroup[3],entries[12]={0};dx_u32 slots[12],types[12]={0};
    dx_u32 cpu_mask=0,order_mask=0,reference=12;
    for(dx_u32 k=0;k<12;k++){
        dx_u64 driver=q(drivers,8*k),wr=q(wrappers,8*k);
        if(!ptr(driver)||!ptr(wr)||q(driver,0)!=base+0x11aea40||
           q(driver,0x28)!=g||q(driver,0x60)!=g||q(wr,8)!=driver){*out=result;return DX_REFUSED;}
        for(dx_u32 j=0;j<k;j++)if(roster[j]==wr||q(drivers,8*j)==driver){*out=result;return DX_REFUSED;}
        roster[k]=wr;slots[k]=w(wr,0x24);
        dx_u32 found=0;
        for(dx_u32 j=0;j<11;j++)found+=q(manager,0xa8+16*j)==wr;
        if(!found){
            if(reference!=12||q(manager,0x168)!=wr||q(manager,0x170)!=wr||q(wr,0x10)){
                *out=result;return DX_REFUSED;
            }
            reference=k;
        }else{
            dx_u32 order=w(wr,0x20);
            if(found!=1||order>=11||slots[k]>=12||order_mask&(1u<<order)||q(manager,0xb0+16*order)!=wr){
                *out=result;return DX_REFUSED;
            }
            order_mask|=1u<<order;cpu_mask|=1u<<k;
            dx_u64 proxy=q(driver,0x88);
            if(!ptr(proxy)||q(proxy,0)!=base+0x11b44f8){*out=result;return DX_REFUSED;}
            dx_u64 role=q(proxy,0x48);
            if(!ptr(role)||w(role,0xa8)!=k||!b(role,0xd2)||!b(role,0xd3)||!b(role,0xe1)||b(role,0xe7)){
                *out=result;return DX_REFUSED;
            }
        }
    }
    if(reference==12||source_racer==reference||order_mask!=0x7ff||cpu_mask!=(0xfffu^(1u<<reference))){
        *out=result;return DX_REFUSED;
    }
    dx_u32 seen=0,start=0,destination=12;
    for(dx_u32 gi=0;gi<3;gi++){
        dx_u64 sub=q(manager,0x80+8*gi);subgroup[gi]=sub;
        if(!ptr(sub)||q(sub,0)!=base+0x11ae478+0x40*gi||q(sub,0x50)!=manager){*out=result;return DX_REFUSED;}
        for(dx_u32 j=0;j<gi;j++)if(subgroup[j]==sub){*out=result;return DX_REFUSED;}
        dx_u32 n=w(sub,8);dx_u64 array=q(sub,0x10);
        if(!n||n>6||w(sub,12)!=6||array!=sub+0x18||w(sub,0x60)!=start||start+n>11){*out=result;return DX_REFUSED;}
        dx_u32 front_labels=0;
        for(dx_u32 j=0;j<n;j++){
            dx_u64 entry=array+8*j,wr=q(entry,0);dx_u32 id=12;
            for(dx_u32 k=0;k<12;k++)if(roster[k]==wr)id=k;
            if(id==12||!(cpu_mask&(1u<<id))||seen&(1u<<id)||q(wr,0x10)!=sub){*out=result;return DX_REFUSED;}
            seen|=1u<<id;entries[id]=entry;types[id]=gi;
            if(!gi){
                if(slots[id]>=n||front_labels&(1u<<slots[id])){*out=result;return DX_REFUSED;}
                front_labels|=1u<<slots[id];
                if(slots[id]==target_front_slot)destination=id;
            }
        }
        if(!gi&&front_labels!=((1u<<n)-1)){*out=result;return DX_REFUSED;}
        start+=n;
    }
    if(start!=11||seen!=cpu_mask||destination==12){*out=result;return DX_REFUSED;}
    result=(PromotionExchangeResult){DX_ALREADY_FRONT,source_racer,destination,slots[source_racer],
        target_front_slot,types[source_racer],g,manager,roster[source_racer],roster[destination]};
    if(!types[source_racer]){*out=result;return result.status;}
    if(slots[source_racer]<=target_front_slot){*out=(PromotionExchangeResult){0};return DX_REFUSED;}
    /* The caller's current native scheduler lease excludes simultaneous
     * subgroup/roster writes. Do not call this helper from host-polling code. */
    *(volatile dx_u64 *)entries[source_racer]=roster[destination];
    *(volatile dx_u64 *)entries[destination]=roster[source_racer];
    *(volatile dx_u64 *)(roster[source_racer]+0x10)=subgroup[0];
    *(volatile dx_u64 *)(roster[destination]+0x10)=subgroup[types[source_racer]];
    *(volatile dx_u32 *)(roster[source_racer]+0x24)=target_front_slot;
    *(volatile dx_u32 *)(roster[destination]+0x24)=slots[source_racer];
    result.status=DX_EXCHANGED;*out=result;return result.status;
}
