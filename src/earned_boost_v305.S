/* Opt-in CPU earned-boost consumers. These hooks have no persistent state,
 * do not select events, and do not change timers, routes or input ownership.
 * The checked module transform fills each native return branch.
 */
#if LAB_EARNED_BOOST_SCOPE > 0
#define LAB_IDENTITY_REGISTER_MACROS_ONLY
#include "identity_local_trampolines_v305.S"
#endif
.text
.balign 4
.global lab_earned_star_ratio_hook
.type lab_earned_star_ratio_hook,%function
lab_earned_star_ratio_hook:
#if LAB_EARNED_BOOST_SCOPE > 0
    save_all
    mov x0,x19
    mov x1,x20
    bl lab_earned_boost_selected
    cbz w0,.Lstar_unselected
    restore_all
    b .Lstar_selected
.Lstar_unselected:
    restore_all
    fcmp s2,s1
    fcsel s1,s2,s1,gt
    b lab_earned_star_ratio_return
.Lstar_selected:
#endif
    /* Native ordinary-CPU branch only: s0=P, s1=Star floor (or zero),
     * s2=unboosted AI ratio, w23=signed Star timer, x20=role.
     * Replace the native max(r,k) with max(r,1)*k during a local Star.
     * Native special-state branches never reach this hook.
     */
    sub sp, sp, #48
    stp x8, x9, [sp]
    stp q3, q4, [sp, #16]
    cmp w23, #0
    b.le .Lstar_original
    ldrb w8, [x20, #0xe7]
    cbnz w8, .Lstar_original
    fmov s3, #1.0
    fcmp s2, s3
    fcsel s3, s2, s3, gt
    fmov s4, s1
    fmul s1, s3, s1
    /* Preserve the NZCV result of the displaced original comparison. */
    fcmp s2, s4
    b .Lstar_done
.Lstar_original:
    fcmp s2, s1
    fcsel s1, s2, s1, gt
.Lstar_done:
    ldp q3, q4, [sp, #16]
    ldp x8, x9, [sp]
    add sp, sp, #48
.global lab_earned_star_ratio_return
lab_earned_star_ratio_return:
    b .
.size lab_earned_star_ratio_hook, .-lab_earned_star_ratio_hook

.balign 4
.global lab_earned_boost_cap_hook
.type lab_earned_boost_cap_hook,%function
lab_earned_boost_cap_hook:
#if LAB_EARNED_BOOST_SCOPE > 0
    save_all
    mov x0,x19
    mov x1,x21
    bl lab_earned_boost_selected
    cbz w0,.Lboost_unselected
    restore_all
    b .Lboost_selected
.Lboost_unselected:
    restore_all
    fmul s0,s1,s0
    b lab_earned_boost_cap_return
.Lboost_selected:
#endif
    /* Native dispatcher already accepted a boost and a multiplier >1.
     * s0=multiplier, s1=selected native base, x19=physics, x21=role.
     * The independent branch already includes its native 7.6 minimum.
     */
    sub sp, sp, #64
    stp x8, x9, [sp]
    stp q1, q2, [sp, #16]
    str q3, [sp, #48]
    mrs x9, nzcv
#if LAB_EARNED_BOOST_SCOPE == 0
    ldrb w8, [x21, #0xd2]
    cbz w8, .Lboost_original
#endif
    ldrb w8, [x21, #0xe7]
    cbnz w8, .Lboost_original
    ldr w8, [x21, #0x1cc]
    tbnz w8, #21, .Lboost_original
    tst w8, #0x18
    b.ne .Lboost_original
    /* +424 is the native pre-AI cap, including preceding vehicle/surface
     * calculations. +428 also preserves a stronger current AI/pace cap.
     * Compare/select retains the native base if a candidate is unordered.
     */
    ldr s2, [x19, #0x424]
    ldr s3, [x19, #0x428]
    fcmp s2, s1
    fcsel s1, s2, s1, gt
    fcmp s3, s1
    fcsel s1, s3, s1, gt
.Lboost_original:
    fmul s0, s1, s0
    msr nzcv, x9
    ldp q1, q2, [sp, #16]
    ldr q3, [sp, #48]
    ldp x8, x9, [sp]
    add sp, sp, #64
.global lab_earned_boost_cap_return
lab_earned_boost_cap_return:
    b .
.size lab_earned_boost_cap_hook, .-lab_earned_boost_cap_hook
