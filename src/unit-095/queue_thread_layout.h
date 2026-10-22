#ifndef LAB_QUEUE_THREAD_LAYOUT_H
#define LAB_QUEUE_THREAD_LAYOUT_H
/* Default keeps existing v3 builds unchanged. Explicit v4 bindings require
 * pinned executable verification by the build adapter. No live qualification. */
#ifndef LAB_THREAD_MANAGER_GOT
#define LAB_THREAD_MANAGER_GOT 0x13072F8
#define LAB_THREAD_MANAGER_GLOBAL 0x134F4A0
#define LAB_THREAD_MANAGER_VTABLE 0x12AAC58
#define LAB_MAIN_THREAD_VTABLE 0x12AAB18
#define LAB_SCHEDULER_GOT 0x12F4E50
#define LAB_SCHEDULER_GLOBAL 0x1354F20
#define LAB_TLS_GETTER_GOT 0x12F3498
#define LAB_TLS_CODE_MIN 0x1A4F000
#define LAB_TLS_CODE_LIMIT 0x3A49000
#endif
#endif
