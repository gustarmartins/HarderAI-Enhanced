#ifndef LAB_NATIVE_ADMISSION_H
#define LAB_NATIVE_ADMISSION_H
#include "manual_intent.h"
typedef struct {
    u64 context, driver, role, state, controller, navigation, group, manager, wrapper;
    u32 original_role, original_e7, original_phase, outcome, owns_role, reserved;
} AdmissionReceipt;
_Static_assert(sizeof(AdmissionReceipt)==96,"Native admission receipt ABI");
/* Outcomes: 1 admitted, 2 adopted, 3 entering, 4 entry failed,
 * 5 leaving, 6 released, 7 release failed. Pending/failed states need recovery. */
/* Return: 0 refused, 1 owned, 2 adopted. The opt-in formation bundle also
 * returns 3 for unprepared native formation, without changing the receipt. */
u32 lab_native_admit(void *,u32,u64,AdmissionReceipt *);
u32 lab_native_release(void *,u32,u64,AdmissionReceipt *);
#endif
