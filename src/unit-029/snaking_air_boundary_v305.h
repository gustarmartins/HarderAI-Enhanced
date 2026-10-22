/* Private next-update prediction: native1848A0 advances the airborne counter
 * before182990 consumes it. This changes only owned prediction; native axes,
 * timers, contact flags, interpolation and exact comparison remain untouched. */
EXPORT const u32 lab_snaking_next_air_config[4]={1,0x201,10,11};
