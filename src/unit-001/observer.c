/* Bootstrap qualification module: writes only its own status block.
 * It installs no hooks and touches no game object, input or policy.
 * CPU-reference code is linked for capacity/relocation testing, never called.
 */
typedef unsigned int u32;
typedef unsigned long long u64;
extern char lab_module_start[];

struct LabStatus {
    char magic[8];
    u32 abi_version;
    u32 struct_size;
    u64 init_count;
    u64 module_base;
    u32 installed_hooks;
    u32 enabled_experiments;
    u64 reserved[3];
};

__attribute__((visibility("default")))
volatile struct LabStatus lab_module_status = {
    .magic = {'M','K','8','L','A','B','0','1'},
    .abi_version = 1,
    .struct_size = sizeof(struct LabStatus),
};

__attribute__((visibility("default")))
void lab_module_init(void) {
    lab_module_status.module_base = (u64)lab_module_start;
    lab_module_status.init_count += 1;
    lab_module_status.installed_hooks = 0;
    lab_module_status.enabled_experiments = 0;
}
