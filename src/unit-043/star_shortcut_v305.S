#include "register_context.inc"
.text
.balign 4
.global lab_star_shortcut_hook
.type lab_star_shortcut_hook,%function
lab_star_shortcut_hook:
    save_all
    bl lab_star_shortcut_select
    cmp w0,#0
    b.lt .Lstar_native_choice
    str x0,[sp]
    restore_all
    ret
.Lstar_native_choice:
    restore_all
    sub sp,sp,#0xa0
.global lab_star_shortcut_return
lab_star_shortcut_return:
    b .
.size lab_star_shortcut_hook,.-lab_star_shortcut_hook
