#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#define RGE_FLAG_MSI 1
struct rge_softc { uint32_t rge_flags; };
static int msix_calls, msi_calls, msix_result, msi_result;
static int pci_intr_map_msix(void* pa, int index, int* ih)
{ (void)pa; (void)ih; assert(index == 0); msix_calls++; return msix_result; }
static int pci_intr_map_msi(void* pa, int* ih)
{ (void)pa; (void)ih; msi_calls++; return msi_result; }
static int select_interrupt(struct rge_softc* sc)
{
	void* pa = NULL;
	int ih;
#include "production_select.inc"
	return 0;
fail:
	return -1;
}
int main(void)
{
	for (int x = 0; x > -2; x--) {
		for (int m = 0; m > -2; m--) {
			struct rge_softc sc = {0};
			msix_result = x; msi_result = m; msix_calls = msi_calls = 0;
			int result = select_interrupt(&sc);
#if defined(HAIKU_PIONEER_RTL_MSI_ONLY) && defined(__riscv)
			assert(msix_calls == 0 && msi_calls == 1);
			assert(result == (m == 0 ? 0 : -1));
#else
			assert(msix_calls == 1 && msi_calls == (x == 0 ? 0 : 1));
			assert(result == (x == 0 || m == 0 ? 0 : -1));
#endif
			assert(sc.rge_flags == (result == 0 ? RGE_FLAG_MSI : 0));
		}
	}
}
