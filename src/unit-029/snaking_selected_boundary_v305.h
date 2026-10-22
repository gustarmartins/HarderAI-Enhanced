/* SELECTED longitudinal coverage after its exact shared bisector has already
 * passed the kart. The caller still proves current native-cache occupation,
 * canonical singleton topology and both physical BASE forecasts separately. */
EXPORT const u32 lab_snaking_selected_boundary_config[4]={1,1,0,0};
EXPORT u32 lab_snaking_selected_boundary_cover(HnbInterval first,HnbInterval second,float travel){
    if(!finite(travel)||travel<=0||!finite(first.lo)||!finite(first.hi)||
       !finite(second.lo)||!finite(second.hi)||first.lo>=first.hi||second.lo>=second.hi||
       first.lo_open>1||first.hi_open>1||second.lo_open>1||second.hi_open>1)return 0;
    if(hnb_cover(first,second,travel))return 1;
    /* Do not skip a coverage gap, borrow a radial/open cut or accept a
     * different boundary. This shared closed cut came from selected_join. */
    return first.hi==second.lo&&first.hi<=0&&!first.hi_open&&!second.lo_open&&
        second.hi>travel;
}
