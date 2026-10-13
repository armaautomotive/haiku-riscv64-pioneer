// Host-side regression fixture for the production XHCI destructor body.
// Kernel APIs are stubs; this does not simulate USB or PCI hardware.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

using int32 = int32_t;
using uint32 = uint32_t;
constexpr int32 B_OK = 0;
constexpr uint32 XHCI_CMD = 0;
#define TRACE(...) ((void)0)

static int sRegisterWrites;
static int sMutexDestructions;
static int sMsiDisables;
static int sMsiUnconfigures;
static std::vector<int32> sDeletedAreas;

static void
Check(bool condition, const char* message)
{
	if (!condition) {
		std::fprintf(stderr, "FAIL: %s\n", message);
		std::exit(1);
	}
}

static void delete_sem(int32) {}
static void wait_for_thread(int32, int32*) {}
static void mutex_destroy(int32*) { sMutexDestructions++; }
static void remove_io_interrupt_handler(uint32, int32 (*)(void*), void*) {}
static void delete_area(int32 area)
{
	if (area >= B_OK)
		sDeletedAreas.push_back(area);
}

struct PciStub {
	void disable_msi(void*) { sMsiDisables++; }
	void unconfigure_msi(void*) { sMsiUnconfigures++; }
};

// Only members used by cleanup are represented. Each test supplies a state
// reached by early construction failure or successful register mapping.
struct XHCI {
	int32 fRegisterArea = -1;
	uint8_t* fRegisters = nullptr;
	uint32 fOperationalRegisterOffset = 0xcccccccc;
	bool fInitOK = false;
	bool fStopThreads = false;
	int32 fCmdCompSem = -1;
	int32 fFinishTransfersSem = -1;
	int32 fEventSem = -1;
	int32 fFinishThread = -1;
	int32 fEventThread = -1;
	int32 fFinishedLock = 0;
	int32 fEventLock = 0;
	uint32 fIRQ = 0;
	int32 fErstArea = -1;
	uint32 fScratchpadCount = 0;
	int32 fScratchpadArea[1] = {-1};
	int32 fDcbaArea = -1;
	bool fUseMSI = false;
	PciStub* fPci = nullptr;
	void* fDevice = nullptr;

	~XHCI();
	static int32 InterruptHandler(void*) { return 0; }
	void WriteOpReg(uint32 reg, uint32 value)
	{
		Check(fRegisterArea >= B_OK && fRegisters != nullptr,
			"cleanup accessed unmapped controller registers");
		Check(fOperationalRegisterOffset == 0x20 && reg == XHCI_CMD,
			"cleanup used an invalid register offset");
		*reinterpret_cast<uint32*>(fRegisters + fOperationalRegisterOffset
			+ reg) = value;
		sRegisterWrites++;
	}
};

// Generated directly from xhci.cpp by run_xhci_cleanup_test.sh.
#include "xhci_destructor.inc"

static void
RunCase(int32 registerArea, bool withAllocatedResources)
{
	sRegisterWrites = sMutexDestructions = sMsiDisables = sMsiUnconfigures = 0;
	sDeletedAreas.clear();
	uint32 registers[16];
	for (uint32& value : registers)
		value = 0xffffffff;
	PciStub pci;
	{
		XHCI controller;
		controller.fRegisterArea = registerArea;
		if (registerArea >= B_OK) {
			controller.fRegisters = reinterpret_cast<uint8_t*>(registers);
			controller.fOperationalRegisterOffset = 0x20;
		}
		if (withAllocatedResources) {
			controller.fInitOK = true;
			controller.fErstArea = 6;
			controller.fDcbaArea = 7;
			controller.fScratchpadCount = 1;
			controller.fScratchpadArea[0] = 8;
			controller.fUseMSI = true;
			controller.fPci = &pci;
		}
	}
	Check(sRegisterWrites == (registerArea >= B_OK ? 1 : 0),
		"mapped controllers must stop, even after partial initialization");
	Check(registers[8] == (registerArea >= B_OK ? 0 : 0xffffffff),
		"unexpected controller command register value");
	Check(sMutexDestructions == 2, "cleanup did not destroy both mutexes");
	Check(sDeletedAreas.size() == (withAllocatedResources ? 4u
		: registerArea >= B_OK ? 1u : 0u), "area cleanup changed");
	Check(sMsiDisables == (withAllocatedResources ? 1 : 0)
		&& sMsiUnconfigures == sMsiDisables, "MSI cleanup changed");
}

int
main()
{
	RunCase(-1, false); // Rejected BAR, before map_physical_memory.
	RunCase(-7, false); // map_physical_memory returned an error.
	RunCase(0, false); // Valid area zero, later constructor failure.
	RunCase(1, true); // Mapped controller with DMA/MSI resources.
	std::puts("PASS: unmapped xHCI cleanup, mapped halt, area and MSI cleanup");
}
