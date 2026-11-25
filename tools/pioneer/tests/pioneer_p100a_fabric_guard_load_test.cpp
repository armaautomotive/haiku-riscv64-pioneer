/* SPDX-License-Identifier: MIT */
// The experimental kernel guard must refuse every module initialization.
#define PIONEER_QWEN35_FABRIC_GUARD_WITNESS 1
#define _KERNEL_MODE 1
#include "../../../src/add-ons/kernel/drivers/misc/pioneer_p100a_reload_guard.cpp"
int main() {
    (void)&create_sem;
    for (unsigned i=0;i<4;i++) {
        assert(modules[i] && modules[i]->std_ops(B_MODULE_INIT)==B_NOT_ALLOWED);
        assert(modules[i]->std_ops(B_MODULE_UNINIT)==B_NOT_ALLOWED);
    }
    assert(!modules[4]);
}
