/*
 * Copyright 2018-2025 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		B Krishnan Iyer, krishnaniyer97@gmail.com
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 *		Ron Ben Aroya, sed4906birdie@gmail.com
 */


#include <algorithm>
#include <new>
#include <stdio.h>
#include <string.h>

#include <bus/PCI.h>
#include <ACPI.h>
#include "acpi.h"

#include <KernelExport.h>

#if defined(__riscv)
#include <vm/vm.h>
#endif

#include "IOSchedulerSimple.h"
#include "mmc.h"
#include "sdhci.h"


//#define TRACE_SDHCI
#ifdef TRACE_SDHCI
#	define TRACE(x...) dprintf("\33[33msdhci:\33[0m " x)
#else
#	define TRACE(x...) do { if (false) dprintf(x); } while (false)
#endif
#define TRACE_ALWAYS(x...)	dprintf("\33[33msdhci:\33[0m " x)
#define ERROR(x...)			dprintf("\33[33msdhci:\33[0m " x)
#define CALLED(x...)		TRACE("CALLED %s\n", __PRETTY_FUNCTION__)


#define SDHCI_DEVICE_MODULE_NAME "busses/mmc/sdhci/driver_v1"


device_manager_info* gDeviceManager;
device_module_info* gMMCBusController;


struct adma2_64_descriptor {
	volatile uint16 attributes;
	volatile uint16 length;
	volatile uint32 addressLow;
	volatile uint32 addressHigh;
} __attribute__((packed));


static const uint16 kAdma2Valid = 1 << 0;
static const uint16 kAdma2End = 1 << 1;
static const uint16 kAdma2Transfer = 2 << 4;


static int32
sdhci_generic_interrupt(void* data)
{
	SdhciBus* bus = (SdhciBus*)data;
	return bus->HandleInterrupt();
}


SdhciBus::SdhciBus(struct registers* registers, uint8_t irq, bool poll,
	uint32 quirks)
	:
	fRegisters(registers),
	fCommandResult(0),
	fIrq(irq),
	fUsePolling(poll),
	fQuirks(quirks),
	fInterruptInstalled(false),
	fScanSemaphore(-1),
	fStatus(B_OK),
	fWorkerThread(-1),
	fCardType(CARD_TYPE_UNKNOWN),
	fSg2042StateDumped(false),
	fAdmaArea(-1),
	fAdmaDescriptors(NULL),
	fAdmaPhysical(0),
	fUseAdma2(false)
{
	dprintf("P202:SC0 constructor irq %u poll %u\n", irq, poll);
	if (irq == 0 || irq == 0xff) {
		ERROR("IRQ not assigned\n");
		fStatus = B_BAD_DATA;
		return;
	}

	fInterruptNotifier.Init(this, "SDHCI interrupts");

	DisableInterrupts();
	dprintf("P202:SC1 interrupts disabled\n");

	if (!fUsePolling) {
		fStatus = install_io_interrupt_handler(fIrq,
			sdhci_generic_interrupt, this, 0);

		if (fStatus != B_OK) {
			ERROR("can't install interrupt handler\n");
			return;
		}
		fInterruptInstalled = true;
	}

	// First of all, we have to make sure we are in a sane state. The easiest
	// way is to reset everything.
	Reset();
	dprintf("P202:SC2 reset complete\n");

	TRACE("Controller spec version: %d, vendor version: %#02x\n",
		fRegisters->host_controller_version.specVersion,
		fRegisters->host_controller_version.vendorVersion);

	TRACE("Capabilities: %s%s%s%s%s%s%s%s%s%s%s%s%s%s\n"
		"    Clock multiplier: %" PRIx8 "\n"
		"    Retuning modes: %" PRIx8 "\n"
		"    Retuning timer count: %" PRIx8 "\n"
		"    Slot type: %" PRIx8 "\n"
		"    Supported voltages: %" PRIx8 "\n"
		"    Max block length: %" PRIx8 "\n"
		"    Base clock frequency: %" PRId8 " MHz\n"
		"    Timeout clock: %" PRId8 " kHz\n",
		fRegisters->capabilities.UseTuningForSDR50() ? "SDR50 needs retuning, " : "",
		fRegisters->capabilities.TypeDSupport() ? "Type-D, " : "",
		fRegisters->capabilities.TypeCSupport() ? "Type-C, " : "",
		fRegisters->capabilities.TypeASupport() ? "Type-A, " : "",
		fRegisters->capabilities.DDR50Support() ? "DDR50, " : "",
		fRegisters->capabilities.SDR104Support() ? "SDR104, " : "",
		fRegisters->capabilities.SDR50Support() ? "SDR50, " : "",
		fRegisters->capabilities.AsynchronousInterrupts() ? "Asynchronous interrupts, " : "",
		fRegisters->capabilities.SystemBus64Bits() ? "64-bit system bus, " : "",
		fRegisters->capabilities.SuspendResume() ? "Suspend/Resume, " : "",
		fRegisters->capabilities.SimpleDMA() ? "Simple DMA, " : "",
		fRegisters->capabilities.HighSpeed() ? "High speed, " : "",
		fRegisters->capabilities.AdvancedDMA() ? "Advanced DMA, " : "",
		fRegisters->capabilities.Embedded8Bit() ? "8-bit Embedded mode, " : "",
		fRegisters->capabilities.ClockMultiplier(),
		fRegisters->capabilities.RetuningModes(),
		fRegisters->capabilities.RetuningTimerCount(),
		fRegisters->capabilities.SlotType(),
		fRegisters->capabilities.SupportedVoltages(),
		fRegisters->capabilities.MaxBlockLength(),
		fRegisters->capabilities.BaseClockFrequency(),
		fRegisters->capabilities.TimeoutClockFrequency());
	TRACE("Initial host control: %x\n", fRegisters->host_control.Bits());
	TRACE("Initial host control 2: %x\n", fRegisters->host_control_2);

	if ((fQuirks & SDHCI_QUIRK_SG2042_PHY) != 0
			&& fRegisters->capabilities.AdvancedDMA()
			&& fRegisters->capabilities.SystemBus64Bits()) {
		status_t admaStatus = _InitSg2042Adma2();
		if (admaStatus != B_OK) {
			ERROR("P297:SG2042 ADMA2 initialization failed: %s; using SDMA\n",
				strerror(admaStatus));
		}
	}

	if (fRegisters->host_controller_version.specVersion > 3) {
		// TODO proper class for manipulating host_control_2
		fRegisters->host_control_2 &= ~(1<<12);
		TRACE("Host control 2 after disabling v4 DMA mode: %x\n", fRegisters->host_control_2);
	}

	// Turn on the power supply to the card, if there is a card inserted
	bool powered = PowerOn();
	dprintf("P202:SC3 power %u\n", powered);
	if (powered) {
		// Then we configure the clock to the frequency needed for
		// initialization
		dprintf("P202:SC4 set initial clock\n");
		SetClock(400, false);
		dprintf("P202:SC5 initial clock ready\n");
	}

	fRegisters->timeout_control.SetDivider(fRegisters->capabilities.TimeoutClockFrequency(), 500);

	// Finally, configure some useful interrupts
	EnableInterrupts(SDHCI_INT_CMD_CMP | SDHCI_INT_CARD_REM
		| SDHCI_INT_TRANS_CMP | SDHCI_INT_DATA_TIMEOUT | SDHCI_INT_COMMAND_TIMEOUT);

	// We want to see the other bits in the status register, but not have an
	// interrupt trigger on them (we get a "command complete" interrupt on
	// errors already)
	fRegisters->interrupt_status_enable |= SDHCI_INT_ERROR_MASK | SDHCI_INT_NORMAL_MASK;
	dprintf("P202:SC6 constructor complete\n");

	// Polling controllers consume command and transfer status synchronously.
	// A separate poller would race with the issuing thread while acknowledging
	// the write-one-to-clear interrupt status register.
}


SdhciBus::~SdhciBus()
{
	TerminateBus();
	if (fAdmaArea >= 0)
		delete_area(fAdmaArea);

	if (fInterruptInstalled)
		remove_io_interrupt_handler(fIrq, sdhci_generic_interrupt, this);

	area_id regs_area = area_for(fRegisters);
	delete_area(regs_area);

	fStatus = B_SHUTTING_DOWN;

	status_t result;
	if (fWorkerThread >= 0)
		wait_for_thread(fWorkerThread, &result);
}


status_t
SdhciBus::_InitSg2042Adma2()
{
	void* descriptors = NULL;
	area_id area = create_area("SG2042 SDHCI ADMA2 descriptors", &descriptors,
		B_ANY_KERNEL_ADDRESS, B_PAGE_SIZE, B_CONTIGUOUS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (area < B_OK)
		return area;

	physical_entry entry;
	status_t status = get_memory_map(descriptors, B_PAGE_SIZE, &entry, 1);
	if (status != B_OK) {
		delete_area(area);
		return status;
	}

#if defined(__riscv)
	status = vm_set_area_memory_type(area, entry.address,
		B_WRITE_THROUGH_MEMORY);
	if (status != B_OK) {
		delete_area(area);
		return status;
	}
#endif

	memset(descriptors, 0, B_PAGE_SIZE);
	fAdmaArea = area;
	fAdmaDescriptors = descriptors;
	fAdmaPhysical = entry.address;
	fRegisters->host_control.SetDMAMode(HostControl::kAdma64);
	memory_full_barrier();
	(void)fRegisters->host_control.Bits();
	fUseAdma2 = true;
	dprintf("P297:SG2042 ADMA2 enabled descriptors physical %#"
		B_PRIxPHYSADDR " host control %#x\n", fAdmaPhysical,
		fRegisters->host_control.Bits());
	return B_OK;
}


void
SdhciBus::EnableInterrupts(uint32_t mask)
{
	fRegisters->interrupt_status_enable |= mask;
	if (!fUsePolling)
		fRegisters->interrupt_signal_enable |= mask;
}


void
SdhciBus::DisableInterrupts()
{
	fRegisters->interrupt_status_enable = 0;
	fRegisters->interrupt_signal_enable = 0;
}


// #pragma mark -
/*
PartA2, SD Host Controller Simplified Specification, Version 4.20
§3.7.1.1 The sequence to issue an SD Command
*/
status_t
SdhciBus::ExecuteCommand(uint8_t command, uint32_t argument, uint32_t* response)
{
	TRACE("ExecuteCommand(%d, %x)\n", command, argument);

	// First of all clear the result
	fCommandResult = 0;
	if (fUsePolling) {
		fRegisters->interrupt_status = SDHCI_INT_CMD_MASK;
		memory_full_barrier();
	}

	// Check if it's possible to send a command right now.
	// It is not possible to send a command as long as the command line is busy.
	// The spec says we should wait, but we can't do that on kernel side, since
	// it leaves no chance for the upper layers to handle the problem. So we
	// just say we're busy and the caller can retry later.
	// Note that this should normally never happen: the command line is busy
	// only during command execution, and we don't leave this function with a
	// command running.
	if (fRegisters->present_state.CommandInhibit()) {
		TRACE_ALWAYS("Command execution impossible, command inhibit\n");
		return B_BUSY;
	}
	if (fRegisters->present_state.DataInhibit()) {
		TRACE_ALWAYS("Command execution unwise, data inhibit\n");
		return B_BUSY;
	}

	// Get ready to accet interrupts that will occur during the command
	ConditionVariableEntry waiter;
	fInterruptNotifier.Add(&waiter);

	uint32_t replyType;
	uint16 transferMode = 0;

	switch (command) {
		// Basic reply types
		case GO_IDLE_STATE:
			replyType = Command::kNoReplyType;
			break;
		case SD_APP_CMD:
		case SD_ERASE_WR_BLK_START:
		case SD_ERASE_WR_BLK_END:
			replyType = Command::kR1Type;
			break;
		case SELECT_DESELECT_CARD:
		case SD_ERASE:
			replyType = Command::kR1bType;
			break;
		case ALL_SEND_CID:
		case SEND_CSD:
			replyType = Command::kR2Type;
			break;
		case MMC_SEND_OP_COND:
		case SD_SEND_OP_COND: // SD Application command
			replyType = Command::kR3Type;
			break;

		// Commands defined with different reply types in SD and MMC specifications
		case SD_SET_BUS_WIDTH: // SD application command. Also MMC_SWITCH, which is not.
			if (fCardType == CARD_TYPE_MMC)
				replyType = Command::kR1bType;
			else
				replyType = Command::kR1Type;
			break;
		case SD_SEND_RELATIVE_ADDR: // also MMC_SET_RELATIVE_ADDR
			if (fCardType == CARD_TYPE_MMC)
				replyType = Command::kR1Type;
			else
				replyType = Command::kR6Type;
			break;
		case SD_SEND_IF_COND: // also MMC_SEND_EXT_CSD
			if (fCardType == CARD_TYPE_MMC)
				replyType = Command::kR1Type;
			else
				replyType = Command::kR7Type;
			break;

		// Commands with data transfer replies, also set transferMode
		case SD_READ_SINGLE_BLOCK:
			transferMode = TransferMode::kRead | TransferMode::kDmaEnable;
			replyType = Command::kR1Type | Command::kDataPresent;
			break;
		case SD_READ_MULTIPLE_BLOCKS:
			transferMode = TransferMode::kRead | TransferMode::kMulti
				| TransferMode::kAutoCmd12Enable | TransferMode::kBlockCountEnable
				| TransferMode::kDmaEnable;
			replyType = Command::kR1Type | Command::kDataPresent;
			break;
		case SD_WRITE_SINGLE_BLOCK:
			transferMode = TransferMode::kWrite | TransferMode::kDmaEnable;
			replyType = Command::kR1Type | Command::kDataPresent;
			break;
		case SD_WRITE_MULTIPLE_BLOCKS:
			transferMode = TransferMode::kWrite | TransferMode::kMulti
				| TransferMode::kAutoCmd12Enable | TransferMode::kBlockCountEnable
				| TransferMode::kDmaEnable;
			replyType = Command::kR1Type | Command::kDataPresent;
			break;
		default:
			ERROR("Unknown command %x\n", command);
			return B_BAD_DATA;
	}

	// Check if DATA line is available (if needed)
	if ((replyType & Command::k32BitResponseCheckBusy) != 0
		&& command != SD_STOP_TRANSMISSION && command != SD_IO_ABORT) {
		if (fRegisters->present_state.DataInhibit()) {
			ERROR("Execution aborted, data inhibit\n");
			return B_BUSY;
		}
	}

	if (fRegisters->present_state.CommandInhibit())
		panic("Command line busy at start of execute command\n");

	fRegisters->argument = argument;

	if ((replyType == Command::kR1bType)
		|| (replyType == (Command::kR1Type | Command::kDataPresent)))
		fRegisters->transfer_mode = transferMode;

	// RISC-V permits device writes to be observed out of order. The command
	// register is the doorbell for the argument and transfer registers, so all
	// preceding MMIO writes must reach the controller first.
	memory_full_barrier();
	fRegisters->command.SendCommand(command, replyType);
	memory_full_barrier();

	// Wait for command response to be available ("command complete" interrupt).
	TRACE("Wait for command complete...");
	if (fUsePolling) {
		uint32 iterations = 0;
		while ((fCommandResult & SDHCI_INT_CMD_MASK) == 0) {
			uint32 intmask = fRegisters->interrupt_status;
			uint32 completed = intmask
				& (SDHCI_INT_CMD_MASK | SDHCI_INT_TRANSFER_MASK
					| SDHCI_INT_ERROR);
			if (completed != 0) {
				fCommandResult |= completed;
				fRegisters->interrupt_status = completed;
				continue;
			}
			if (++iterations >= 50000) {
				ERROR("Command completion timed out: command %u, argument %#"
					B_PRIx32 ", status %#" B_PRIx32 ", present %#" B_PRIx32
					"\n", command, argument, intmask,
					fRegisters->present_state.Bits());
				_DumpSg2042State("command completion timeout", command,
					argument);
				fRegisters->software_reset.ResetCommandAndDataLines();
				return B_TIMED_OUT;
			}
			if (iterations % 10000 == 0) {
				TRACE("Command complete status did not appear, status %x, "
					"command line busy: %d, data line busy: %d\n", intmask,
					fRegisters->present_state.CommandInhibit(),
					fRegisters->present_state.DataInhibit());
			}
			snooze(100);
		}
	} else {
		do {
			status_t result = waiter.Wait(B_RELATIVE_TIMEOUT, 1000000);
			if (result == B_TIMED_OUT) {
				TRACE("Command complete interrupt did not trigger for a while, status %x\n",
					fRegisters->interrupt_status);
			} else if (result != B_OK) {
				panic("sdhci: Failed to wait for command complete: %s",
					strerror(result));
			}

			fInterruptNotifier.Add(&waiter);
			TRACE("Command status: %x\n", fCommandResult);
			TRACE("real status = %x command line busy: %d\n",
				fRegisters->interrupt_status,
				fRegisters->present_state.CommandInhibit());
		} while (fCommandResult == 0);
	}

	TRACE("Command response available\n");

	if (fCommandResult & SDHCI_INT_ERROR) {
		_DumpSg2042State("command interrupt error", command, argument);
		// TODO is it a good idea to clear interrupts here from outside the interrupt handler?
		fRegisters->interrupt_status = fCommandResult;
		if (fCommandResult & SDHCI_INT_COMMAND_TIMEOUT) {
			ERROR("Command execution timed out\n");
			// At this point, the "command inhibit" bit is not set yet, it will be set only after
			// another command is sent while the controller is in the timeout state.
			// But resetting the controller state pre-emptively will allow to send another command.
			//
			// Clear the data line at the same time if it is busy
			fRegisters->software_reset.ResetCommandAndDataLines();
			return B_TIMED_OUT;
		}
		if (fCommandResult & SDHCI_INT_COMMAND_CRC) {
			ERROR("CRC error\n");
			return B_BAD_VALUE;
		}
		ERROR("Command execution failed %x\n", fCommandResult);
		// TODO look at errors in interrupt_status register for more details
		// and return a more appropriate error code
		return B_ERROR;
	}

	if (fRegisters->present_state.CommandInhibit()) {
		_DumpSg2042State("command inhibit after completion", command,
			argument);
		TRACE("Command execution failed, card stalled\n");
		// Clear the stall
		fRegisters->software_reset.ResetCommandLine();
		return B_ERROR;
	}

	switch (replyType & Command::kReplySizeMask) {
		case Command::k32BitResponse:
			*response = fRegisters->response[0];
			break;
		case Command::k128BitResponse:
			response[0] = fRegisters->response[0];
			response[1] = fRegisters->response[1];
			response[2] = fRegisters->response[2];
			response[3] = fRegisters->response[3];
			break;

		default:
			// No response
			break;
	}

	if ((replyType == Command::kR1bType)
			&& (fCommandResult & SDHCI_INT_TRANSFER_MASK) == 0) {
		// R1b commands may use the data line so we must wait for the
		// "transfer complete" interrupt here.
		TRACE("Waiting for data line...\n");
		fInterruptNotifier.Add(&waiter);
		uint32 pollingIterations = 0;
		while (fRegisters->present_state.DataInhibit()) {
			if (fUsePolling) {
				if (++pollingIterations >= 50000) {
					ERROR("Data line release timed out after command %u: "
						"status %#" B_PRIx32 ", present %#" B_PRIx32 "\n",
						command, fRegisters->interrupt_status,
						fRegisters->present_state.Bits());
					_DumpSg2042State("data line release timeout", command,
						argument);
					fRegisters->software_reset.ResetDataLine();
					return B_TIMED_OUT;
				}
				snooze(100);
			} else {
				status_t result = waiter.Wait();
				if (result != B_OK) {
					panic("sdhci: Failed to wait for data line release: %s",
						strerror(result));
				}
				fInterruptNotifier.Add(&waiter);
			}
		}
		TRACE("Dataline is released.\n");
	}

	TRACE("Command execution %d complete\n", command);
	return B_OK;
}


status_t
SdhciBus::InitCheck()
{
	return fStatus;
}


void
SdhciBus::Reset()
{
	dprintf("P214:SR0 reset all begin bits %#x\n",
		fRegisters->software_reset.Bits());
	if (!fRegisters->software_reset.ResetAll())
		ERROR("SdhciBus::Reset: SoftwareReset timeout (bits %#x)\n",
			fRegisters->software_reset.Bits());
	else
		dprintf("P214:SR1 reset all complete bits %#x\n",
			fRegisters->software_reset.Bits());
	// The SG2042 vendor PHY registers must not be accessed immediately after
	// the host reset completes. Use a busy-wait because this runs during early
	// device-manager initialization, where a scheduler sleep may not wake.
	spin(10000);

	if ((fQuirks & SDHCI_QUIRK_SG2042_PHY) != 0) {
		dprintf("P204:SR2 SG2042 PHY begin\n");
		_InitSg2042Phy();
		dprintf("P204:SR3 SG2042 PHY complete\n");
	}
}


void
SdhciBus::_InitSg2042Phy()
{
	// The SG2042 DWC MSHC PHY loses its configuration on a full host reset.
	// Configure it for the 3.3 V SD slot before issuing the first command.
	volatile uint8* base = (volatile uint8*)fRegisters;
	volatile uint32* phyConfig = (volatile uint32*)(base + 0x300);
	volatile uint16* commandPad = (volatile uint16*)(base + 0x304);
	volatile uint16* dataPad = (volatile uint16*)(base + 0x306);
	volatile uint16* clockPad = (volatile uint16*)(base + 0x308);
	volatile uint16* strobePad = (volatile uint16*)(base + 0x30a);
	volatile uint16* resetPad = (volatile uint16*)(base + 0x30c);

	dprintf("P205:SP0 read PHY config\n");
	uint32 config = *phyConfig;
	config &= ~1u;
	// The Pioneer vendor driver initially uses 9/8, but changes both drive
	// controls to 0xe when the SD card selects driver type C. Haiku does not
	// currently perform that UHS drive-strength negotiation, so program the
	// resulting known-good Pioneer setting directly.
	config |= (1u << 1) | (0xeu << 16) | (0xeu << 20);
	*phyConfig = config;
	memory_full_barrier();
	(void)*phyConfig;
	memory_full_barrier();
	dprintf("P205:SP1 PHY config asserted\n");

	const uint16 pullUpPad = 2u | (1u << 3) | (3u << 5) | (2u << 9);
	*commandPad = pullUpPad;
	*dataPad = pullUpPad;
	*resetPad = pullUpPad;
	*clockPad = 2u | (3u << 5) | (2u << 9);
	*strobePad = 2u | (2u << 3) | (3u << 5) | (2u << 9);
	memory_full_barrier();
	(void)*resetPad;
	memory_full_barrier();
	dprintf("P205:SP2 PHY pads configured\n");

	volatile uint8* sdClockDelayConfig = base + 0x31d;
	volatile uint8* sdClockDelayCode = base + 0x31e;
	*sdClockDelayConfig = 1u;
	*sdClockDelayConfig |= (1u << 4);
	// The vendor driver changes this from its reset-time value of 10 to 0x10
	// whenever it enables a non-zero card clock. Use that active-clock value;
	// leaving the reset-time delay selected eventually causes command timeouts.
	*sdClockDelayCode = 0x10;
	*sdClockDelayConfig &= ~(1u << 4);
	memory_full_barrier();
	(void)*sdClockDelayConfig;
	memory_full_barrier();
	dprintf("P205:SP3 clock delay configured\n");
	*(base + 0x320) = (1u << 1);
	*(base + 0x321) = (2u << 2);
	// Preserve the SG2042 firmware's Vendor Host Control 3 and automatic-tuning
	// state. Other DWC MSHC integrations disable command-conflict checking here,
	// but the upstream Linux SG2042 path deliberately leaves it untouched. A
	// zero value caused the first Haiku filesystem read to stop completing.
	memory_full_barrier();
	(void)*(base + 0x321);
	memory_full_barrier();
	dprintf("P294:SG2042 firmware vendor and tuning state preserved\n");

	*phyConfig |= 1u;
	memory_full_barrier();
	(void)*phyConfig;
	memory_full_barrier();
	dprintf("P205:SP5 PHY reset deasserted\n");
	TRACE("SG2042 SD PHY initialized: config %#" B_PRIx32 "\n",
		*phyConfig);
}


void
SdhciBus::_DumpSg2042State(const char* reason, uint8 command, uint32 argument)
{
	if ((fQuirks & SDHCI_QUIRK_SG2042_PHY) == 0 || fSg2042StateDumped)
		return;
	fSg2042StateDumped = true;

	volatile uint8* base = (volatile uint8*)fRegisters;
	memory_full_barrier();
	dprintf("P291:FAIL %s cmd %u arg %#" B_PRIx32 " present %#" B_PRIx32
		" int %#" B_PRIx32 " result %#" B_PRIx32 " clock %#x host1 %#x"
		" host2 %#x reset %#x\n", reason, command, argument,
		fRegisters->present_state.Bits(), fRegisters->interrupt_status,
		fCommandResult, fRegisters->clock_control.Bits(),
		fRegisters->host_control.Bits(), fRegisters->host_control_2,
		fRegisters->software_reset.Bits());
	dprintf("P291:PHY cfg %#" B_PRIx32 " pads %#" B_PRIx32 " %#"
		B_PRIx32 " %#" B_PRIx32 " delay %#" B_PRIx32 " dll %#"
		B_PRIx32 " status %#" B_PRIx32 "\n",
		*(volatile uint32*)(base + 0x300),
		*(volatile uint32*)(base + 0x304),
		*(volatile uint32*)(base + 0x308),
		*(volatile uint32*)(base + 0x30c),
		*(volatile uint32*)(base + 0x31c),
		*(volatile uint32*)(base + 0x324),
		*(volatile uint32*)(base + 0x32c));
	dprintf("P291:VENDOR mshc %#" B_PRIx32 " atctrl %#" B_PRIx32
		" atstat %#" B_PRIx32 "\n",
		*(volatile uint32*)(base + 0x508),
		*(volatile uint32*)(base + 0x540),
		*(volatile uint32*)(base + 0x544));
	dprintf("P296:CMD sysaddr %#" B_PRIx32 " block %#" B_PRIx32
		" arg %#" B_PRIx32 " xfer %#x cmd %#x\n",
		fRegisters->system_address,
		*(volatile uint32*)(base + 0x04), fRegisters->argument,
		fRegisters->transfer_mode, fRegisters->command.Bits());
	if (fUseAdma2) {
		adma2_64_descriptor* descriptor
			= (adma2_64_descriptor*)fAdmaDescriptors;
		dprintf("P297:ADMA table %#" B_PRIx64 " error %#x descriptor"
			" attr %#x len %u address %#" B_PRIx64 "\n",
			fRegisters->adma_system_address,
			fRegisters->adma_error_status, descriptor->attributes,
			descriptor->length,
			((uint64)descriptor->addressHigh << 32)
				| descriptor->addressLow);
	}
	memory_full_barrier();
	dump_sg2042_clock_state();
}


void
SdhciBus::SetClock(int kilohertz, bool allowAuto)
{
	if ((fQuirks & SDHCI_QUIRK_SG2042_PHY) != 0 && kilohertz > 6250) {
		dprintf("P285:SG2042 limiting SD clock from %d to 6250 kHz\n",
			kilohertz);
		kilohertz = 6250;
	}
	// SG2042 advertises preset support, but its preset values are broken.
	// Keep programming the divider explicitly on this controller.
	if (allowAuto && (fRegisters->host_controller_version.specVersion > 2)
		&& (fQuirks & SDHCI_QUIRK_SG2042_PHY) == 0) {
		TRACE("Ignoring set_clock, controller support presets\n");
		fRegisters->host_control_2 |= (1<<15);
		TRACE("Host control 2 after enabling preset mode: %x\n", fRegisters->host_control_2);
		return;
	}

	int base_clock = fRegisters->capabilities.BaseClockFrequency();
	// Try to get as close to 400kHz as possible, but not faster
	int divider = base_clock * 1000 / kilohertz;

	if (fRegisters->host_controller_version.specVersion <= 1) {
		// Old controller only support power of two dividers up to 256,
		// round to next power of two up to 256
		if (divider > 256)
			divider = 256;

		divider--;
		divider |= divider >> 1;
		divider |= divider >> 2;
		divider |= divider >> 4;
		divider++;
	}

	divider = fRegisters->clock_control.SetDivider(divider);

	// Log the value after possible rounding by SetDivider (only even values
	// are allowed).
	TRACE("SDCLK frequency: requested %dkHz, effective %dMHz / %d = %dkHz\n", kilohertz,
		base_clock, divider, base_clock * 1000 / divider);

	// We have set the divider, now we can enable the internal clock.
	fRegisters->clock_control.EnableInternal();

	// wait until internal clock is stabilized
	while (!(fRegisters->clock_control.InternalStable()));

	fRegisters->clock_control.EnablePLL();
	while (!(fRegisters->clock_control.InternalStable()));

	// Finally, route the clock to the SD card
	fRegisters->clock_control.EnableSD();
	if ((fQuirks & SDHCI_QUIRK_SG2042_PHY) != 0) {
		volatile uint8* base = (volatile uint8*)fRegisters;
		*(base + 0x31e) = 0x10;
		memory_full_barrier();
		(void)*(base + 0x31e);
		memory_full_barrier();
		dprintf("P290:SG2042 active clock PHY drive e/e delay 0x10\n");
	}
}


status_t
SdhciBus::DoIO(uint8_t command, IOOperation* operation, bool offsetAsSectors)
{
	bool isWrite = operation->IsWrite();

	static const uint32 kBlockSize = 512;
	off_t offset = operation->Offset();
	generic_size_t length = operation->Length();

	TRACE("%s %" B_PRIuGENADDR " bytes at %" B_PRIdOFF "\n",
		isWrite ? "Write" : "Read", length, offset);

	// Check that the IO scheduler did its job in following our DMA restrictions
	// We can start a read only at a sector boundary
	ASSERT(offset % kBlockSize == 0);
	// We can only read complete sectors
	ASSERT(length % kBlockSize == 0);

	const generic_io_vec* vecs = operation->Vecs();
	generic_size_t vecOffset = 0;
	uint32 transferRetryCount = 0;

	status_t result = B_OK;
	while (length > 0) {
		size_t toCopy = std::min((generic_size_t)length,
			vecs->length - vecOffset);

		// If the current vec is empty, we can move to the next
		if (toCopy == 0) {
			vecs++;
			vecOffset = 0;
			continue;
		}

		uint8 effectiveCommand = command;
		if ((fQuirks & SDHCI_QUIRK_SG2042_PHY) != 0) {
			if (fUseAdma2) {
				// Keep the descriptor length explicit instead of relying on the
				// special zero-means-64-KiB encoding. Smaller batches completed
				// reliably on SG2042, while the first boot allowed to issue that
				// encoding stopped during device-module initialization. Batching up
				// to 127 sectors still avoids the CMD17 command storm.
				toCopy = std::min(toCopy, (size_t)65024);
				if (toCopy == kBlockSize) {
					if (command == SD_READ_MULTIPLE_BLOCKS)
						effectiveCommand = SD_READ_SINGLE_BLOCK;
					else if (command == SD_WRITE_MULTIPLE_BLOCKS)
						effectiveCommand = SD_WRITE_SINGLE_BLOCK;
				}
			} else {
				// SG2042 SDMA eventually times out a multi-block transfer together
				// with an Auto-CMD12 error, leaving DAT inhibit asserted. Keep the
				// established single-block fallback when ADMA2 is unavailable.
				toCopy = std::min(toCopy, (size_t)kBlockSize);
				if (command == SD_READ_MULTIPLE_BLOCKS)
					effectiveCommand = SD_READ_SINGLE_BLOCK;
				else if (command == SD_WRITE_MULTIPLE_BLOCKS)
					effectiveCommand = SD_WRITE_SINGLE_BLOCK;
			}
		}

		// Follow steps from SD Host Controller Simplified Specification Version 4.20
		// section 3.7.2.2.

		// With SDMA we can only transfer multiples of 1 sector
		ASSERT(toCopy % kBlockSize == 0);

		// Step 1: set the data address. SG2042 advertises ADMA2 with 64-bit
		// addressing, and its Linux driver uses that path. Its SDMA engine can
		// intermittently stop accepting otherwise valid CMD17 transfers, so use
		// one ADMA2 descriptor for the already single-block SG2042 requests.
		if (fUseAdma2) {
			adma2_64_descriptor* descriptor
				= (adma2_64_descriptor*)fAdmaDescriptors;
			uint64 dataAddress = vecs->base + vecOffset;
			descriptor->attributes = kAdma2Valid | kAdma2End | kAdma2Transfer;
			descriptor->length = toCopy;
			descriptor->addressLow = (uint32)dataAddress;
			descriptor->addressHigh = (uint32)(dataAddress >> 32);
			memory_full_barrier();
			fRegisters->adma_system_address = fAdmaPhysical;
		} else
			fRegisters->system_address = vecs->base + vecOffset;

		// Step 2: Set block size
		// For simplicity we use a transfer size equal to the sector size. We could
		// go up to 2K here if the length to read in each individual vec is a
		// multiple of 2K, but we have no easy way to know this (we would need to
		// iterate through the IOOperation vecs and check the size of each of them).
		// We could also do smaller transfers, but it is not possible to start a
		// transfer anywhere else than the start of a sector, so it's a lot simpler
		// to always work in complete sectors. We set the B_DMA_ALIGNMENT device
		// node property accordingly, making sure that we don't get asked to do
		// transfers that are not aligned with sectors.
		//
		// Additionnally, set SDMA buffer boundary aligment to 512K. This is the
		// largest possible size. We also set the B_DMA_BOUNDARY property on the
		// published device node, so that the DMA resource manager knows that it
		// must respect this boundary. As a result, we will never be asked to
		// do a transfer that crosses this boundary, and we don't need to handle
		// the DMA boundary interrupt (the transfer will be split in two at an
		// upper layer).
		fRegisters->block_size.ConfigureTransfer(kBlockSize,
			BlockSize::kDmaBoundary512K);

		// Step 3: set block count
		fRegisters->block_count = toCopy / kBlockSize;

		// Steps done in ExecuteCommand:
		// Steps 4, 5 and 6: set argument register, transfer_mode and command register
		// Step 7, 8, 9: wait for command complete interrupt, clear interrupt, read response
		ConditionVariableEntry waiter;
		fInterruptNotifier.Add(&waiter);

		uint32_t response;
		uint32 commandAttempt = 0;
		do {
		result = ExecuteCommand(effectiveCommand,
				offset / (offsetAsSectors ? kBlockSize : 1), &response);
			if (result != B_TIMED_OUT
					|| (fQuirks & SDHCI_QUIRK_SG2042_PHY) == 0)
				break;
			commandAttempt++;
			if (commandAttempt < 3) {
				dprintf("P284:SG2042 retry command %u sector %#" B_PRIx64
					" attempt %u\n", effectiveCommand,
					(uint64)(offset / (offsetAsSectors ? kBlockSize : 1)),
					commandAttempt + 1);
				snooze(1000);
			}
		} while (commandAttempt < 3);
		if (result != B_OK) {
			if (result == B_TIMED_OUT
					&& (fQuirks & SDHCI_QUIRK_SG2042_PHY) != 0
					&& ++transferRetryCount < 3) {
				dprintf("P285:SG2042 retry full command %u sector %#"
					B_PRIx64 " attempt %u\n", effectiveCommand,
					(uint64)(offset / (offsetAsSectors ? kBlockSize : 1)),
					transferRetryCount + 1);
				continue;
			}
			break;
		}

		// Step 10: Wait for DMA transfer to complete
		// In theory we could go on and send other commands as long as they
		// don't need the DAT lines, but it's overcomplicating things.
		TRACE("Wait for transfer complete...");
		uint32 pollingIterations = 0;
		while ((fCommandResult & SDHCI_INT_TRANSFER_MASK) == 0) {
			if (fUsePolling) {
				uint32 intmask = fRegisters->interrupt_status;
				uint32 completed = intmask & SDHCI_INT_TRANSFER_MASK;
				if (completed != 0) {
					fCommandResult |= completed;
					fRegisters->interrupt_status = completed;
					continue;
				}
				if (++pollingIterations >= 50000) {
					ERROR("Transfer completion timed out: status %#" B_PRIx32
						", command result %#" B_PRIx32 "\n", intmask,
						fCommandResult);
					_DumpSg2042State("transfer completion timeout",
						effectiveCommand, (uint32)(offset
							/ (offsetAsSectors ? kBlockSize : 1)));
					fRegisters->software_reset.ResetCommandAndDataLines();
					result = B_TIMED_OUT;
					break;
				}
				snooze(100);
			} else {
				status_t result = waiter.Wait(B_RELATIVE_TIMEOUT, 1000000);
				if (result == B_TIMED_OUT) {
					TRACE("Transfer complete interrupt did not trigger for a while, status %x\n",
						fRegisters->interrupt_status);
				} else if (result != B_OK) {
					panic("sdhci: Failed to wait for end of DMA transfer: %s",
						strerror(result));
				}
				fInterruptNotifier.Add(&waiter);
			}
		}

		if (result == B_TIMED_OUT) {
			if ((fQuirks & SDHCI_QUIRK_SG2042_PHY) != 0
					&& ++transferRetryCount < 3) {
				dprintf("P285:SG2042 retry full transfer command %u sector %#"
					B_PRIx64 " attempt %u\n", effectiveCommand,
					(uint64)(offset / (offsetAsSectors ? kBlockSize : 1)),
					transferRetryCount + 1);
				continue;
			}
			return result;
		}

		uint32 dataErrors = fCommandResult & SDHCI_INT_DATA_ERROR_MASK;
		if (dataErrors != 0) {
			ERROR("Data transfer failed: %#" B_PRIx32 "\n", dataErrors);
			_DumpSg2042State("data transfer error", effectiveCommand,
				(uint32)(offset / (offsetAsSectors ? kBlockSize : 1)));
			bool reset = fRegisters->software_reset.ResetCommandAndDataLines();
			dprintf("P283:SD transfer recovery reset %u present %#" B_PRIx32
				"\n", reset, fRegisters->present_state.Bits());
			status_t dataResult = (dataErrors & SDHCI_INT_DATA_TIMEOUT) != 0
				? B_TIMED_OUT : B_IO_ERROR;
			if ((fQuirks & SDHCI_QUIRK_SG2042_PHY) != 0
					&& ++transferRetryCount < 3) {
				dprintf("P285:SG2042 retry errored transfer command %u sector %#"
					B_PRIx64 " attempt %u\n", effectiveCommand,
					(uint64)(offset / (offsetAsSectors ? kBlockSize : 1)),
					transferRetryCount + 1);
				continue;
			}
			return dataResult;
		}

		TRACE("transfer complete OK.\n");
		transferRetryCount = 0;
		length -= toCopy;
		vecOffset += toCopy;
		offset += toCopy;
	}

	return result;
}


void
SdhciBus::SetScanSemaphore(sem_id sem)
{
	fScanSemaphore = sem;
	uint32 presentState = fRegisters->present_state.Bits();
	dprintf("P243:SS0 scan semaphore %" B_PRId32 " present %#" B_PRIx32
		"\n", fScanSemaphore, presentState);

	// If there is already a card in, start a scan immediately
	if ((presentState & (1 << 16)) != 0) {
		status_t status = release_sem(fScanSemaphore);
		dprintf("P243:SS1 initial scan release %s\n", strerror(status));
	} else
		dprintf("P243:SS1 no card present\n");

	// We can now enable the card insertion interrupt for next time a card
	// is inserted
	EnableInterrupts(SDHCI_INT_CARD_INS);
	dprintf("P243:SS2 card insertion interrupt enabled\n");
}


void
SdhciBus::SetBusWidth(int width)
{
	uint8_t widthBits;
	switch(width) {
		case 1:
			widthBits = HostControl::kDataTransfer1Bit;
			break;
		case 4:
			widthBits = HostControl::kDataTransfer4Bit;
			break;
		case 8:
			widthBits = HostControl::kDataTransfer8Bit;
			break;
		default:
			panic("Incorrect bitwidth value");
			return;
	}
	fRegisters->host_control.SetDataTransferWidth(widthBits);
}


void
SdhciBus::SetCardType(card_type type)
{
	fCardType = type;
}


bool
SdhciBus::PowerOn()
{
	dprintf("P227:PO0 read present state\n");
	uint32 presentState = fRegisters->present_state.Bits();
	dprintf("P227:PO1 present state %#" PRIx32 "\n", presentState);
	if ((presentState & (1 << 16)) == 0) {
		TRACE("Card not inserted, not powering on for now\n");
		return false;
	}

	dprintf("P227:PO2 read capabilities\n");
	uint8_t supportedVoltages = fRegisters->capabilities.SupportedVoltages();
	dprintf("P227:PO3 supported voltages %#x\n", supportedVoltages);
	dprintf("P227:PO4 set power control\n");
	if ((supportedVoltages & Capabilities::k3v3) != 0)
		fRegisters->power_control.SetVoltage(PowerControl::k3v3);
	else if ((supportedVoltages & Capabilities::k3v0) != 0)
		fRegisters->power_control.SetVoltage(PowerControl::k3v0);
	else if ((supportedVoltages & Capabilities::k1v8) != 0)
		fRegisters->power_control.SetVoltage(PowerControl::k1v8);
	else {
		fRegisters->power_control.PowerOff();
		ERROR("No voltage is supported\n");
		return false;
	}
	dprintf("P227:PO5 power control %#x\n",
		fRegisters->power_control.Bits());

	return true;
}


void
SdhciBus::PowerOff()
{
	fRegisters->power_control.PowerOff();
}


void
SdhciBus::TerminateBus()
{
	CALLED();

	DisableInterrupts();
	fRegisters->clock_control.DisableSD();
	PowerOff();
	/*
	// Debugging.
	uint8_t powerBits = fRegisters->power_control.Bits();
	uint16_t clockBits = fRegisters->clock_control.Bits();
	if ((powerBits & 0x1) != 0 || (clockBits & (1 << 2)) != 0) {
		ERROR("TerminateBus: Not killed. "
			"(power=%#x, clock=%#x)\n", powerBits, clockBits);
	} else {
		TRACE("TerminateBus: killed. (power=%#x, "
			"clock=%#x)\n", powerBits, clockBits);
	}
	*/
}


void
SdhciBus::RecoverError()
{
	fRegisters->interrupt_signal_enable &= ~(SDHCI_INT_CMD_CMP
		| SDHCI_INT_TRANS_CMP | SDHCI_INT_CARD_INS | SDHCI_INT_CARD_REM);

	if (fRegisters->interrupt_status & 7)
		fRegisters->software_reset.ResetCommandLine();

	uint32 errorStatus = fRegisters->interrupt_status;
	fRegisters->interrupt_status = errorStatus;
}


int32
SdhciBus::HandleInterrupt()
{
#if 0
	// We could use the slot register to quickly see for which slot the
	// interrupt is. But since we have an interrupt handler call for each slot
	// anyway, it's just as simple to let each of them scan its own interrupt
	// status register.
	if ( !(fRegisters->slot_interrupt_status & (1 << fSlot)) ) {
		TRACE("interrupt not for me.\n");
		return B_UNHANDLED_INTERRUPT;
	}
#endif
	
	uint32_t intmask = fRegisters->interrupt_status;

	// Shortcut: exit early if there is no interrupt or if the register is
	// clearly invalid.
	if ((intmask == 0) || (intmask == 0xffffffff)) {
		return B_UNHANDLED_INTERRUPT;
	}

	TRACE("interrupt function called %x\n", intmask);

	// handling card presence interrupts
	if ((intmask & SDHCI_INT_CARD_REM) != 0) {
		// We can get spurious interrupts as the card is inserted or removed,
		// so check the actual state before acting
		if (!fRegisters->present_state.IsCardInserted())
			fRegisters->power_control.PowerOff();
		else
			TRACE("Card removed interrupt, but card is inserted\n");

		fRegisters->interrupt_status = SDHCI_INT_CARD_REM;
		TRACE("Card removal interrupt handled\n");
	}

	if ((intmask & SDHCI_INT_CARD_INS) != 0) {
		// We can get spurious interrupts as the card is inserted or removed,
		// so check the actual state before acting
		if (fRegisters->present_state.IsCardInserted()) {
			if (PowerOn())
				SetClock(400, false);
			release_sem_etc(fScanSemaphore, 1, B_DO_NOT_RESCHEDULE);
		} else
			TRACE("Card insertion interrupt, but card is removed\n");

		fRegisters->interrupt_status = SDHCI_INT_CARD_INS;
		TRACE("Card presence interrupt handled\n");
	}

	// handling command interrupt
	if (intmask & SDHCI_INT_CMD_MASK) {
		fCommandResult |= intmask;
			// Save the status before clearing so the thread can handle it

		fRegisters->interrupt_status = intmask & SDHCI_INT_CMD_MASK;

		// Notify the thread
		fInterruptNotifier.NotifyAll();
		TRACE("Command complete interrupt handled\n");
	}

	if (intmask & SDHCI_INT_TRANSFER_MASK) {
		fCommandResult |= intmask;
		fRegisters->interrupt_status = intmask & SDHCI_INT_TRANSFER_MASK;
		fInterruptNotifier.NotifyAll();
		TRACE("Transfer complete interrupt handled\n");
	}

	// handling bus power interrupt
	if (intmask & SDHCI_INT_BUS_POWER) {
		fRegisters->interrupt_status = SDHCI_INT_BUS_POWER;
		TRACE("card is consuming too much power\n");
	}

	// Check that all interrupts have been cleared (we check all the ones we
	// enabled, so that should always be the case)
	intmask = fRegisters->interrupt_status;
	if (intmask != 0) {
		ERROR("Remaining interrupts at end of handler: %x\n", intmask);
	}

	return B_HANDLED_INTERRUPT;
}


status_t
SdhciBus::_WorkerThread(void* cookie) {
	SdhciBus* bus = (SdhciBus*)cookie;
	while (bus->fStatus != B_SHUTTING_DOWN) {
		uint32_t intmask = bus->fRegisters->interrupt_status;
		if (intmask & SDHCI_INT_CMD_CMP) {
			bus->fCommandResult = intmask;
			bus->fRegisters->interrupt_status = intmask & SDHCI_INT_CMD_MASK;
			bus->fInterruptNotifier.NotifyAll();
		}
		if (intmask & SDHCI_INT_TRANS_CMP) {
			bus->fCommandResult = intmask;
			bus->fRegisters->interrupt_status = SDHCI_INT_TRANS_CMP;
			bus->fInterruptNotifier.NotifyAll();
		}
		snooze(100);
	}
	TRACE("poller thread terminating");
	return B_OK;
}


// #pragma mark -


void
uninit_bus(void* bus_cookie)
{
	SdhciBus* bus = (SdhciBus*)bus_cookie;
	delete bus;

	// FIXME do we need to put() the PCI module here?
}


void
bus_removed(void* bus_cookie)
{
	return;
}


static status_t
register_child_devices(void* cookie)
{
	CALLED();
	SdhciDevice* context = (SdhciDevice*)cookie;
	status_t status = B_OK;
	const char* bus;
	device_node* parent = gDeviceManager->get_parent_node(context->fNode);
	status = gDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false);
	if (status != B_OK) {
		TRACE("Could not find required attribute device/bus\n");
		return status;
	}

	if (strcmp(bus, "pci") == 0)
		status = register_child_devices_pci(cookie);
	else if (strcmp(bus, "acpi") == 0)
		status = register_child_devices_acpi(cookie);
	else if (strcmp(bus, "fdt") == 0)
		status = register_child_devices_fdt(cookie);
	else
		status = B_BAD_VALUE;

	return status;
}


static status_t
init_device(device_node* node, void** device_cookie)
{
	CALLED();

	SdhciDevice* context = new(std::nothrow)SdhciDevice;
	if (context == NULL)
		return B_NO_MEMORY;
	context->fNode = node;
	*device_cookie = context;

	status_t status = B_OK;
	const char* bus;
	device_node* parent = gDeviceManager->get_parent_node(node);
	status = gDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false);
	if (status != B_OK) {
		TRACE("Could not find required attribute device/bus\n");
		return status;
	}

	if (strcmp(bus, "pci") == 0)
		return init_device_pci(node, context);

	return B_OK;
}


static void
uninit_device(void* device_cookie)
{
	SdhciDevice* context = (SdhciDevice*)device_cookie;
	device_node* parent = gDeviceManager->get_parent_node(context->fNode);

	const char* bus;
	if (gDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false) != B_OK) {
		TRACE("Could not find required attribute device/bus\n");
	}

	if (strcmp(bus, "pci") == 0)
		uninit_device_pci(context, parent);

	gDeviceManager->put_node(parent);

	delete context;
}


static status_t
register_device(device_node* parent)
{
	device_attr attrs[] = {
		{B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {.string = "SD Host Controller"}},
		{}
	};

	return gDeviceManager->register_node(parent, SDHCI_DEVICE_MODULE_NAME,
		attrs, NULL, NULL);
}


static float
supports_device(device_node* parent)
{
	const char* bus;

	// make sure parent is either an ACPI or PCI SDHCI device node
	if (gDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false)
		!= B_OK) {
		TRACE("Could not find required attribute device/bus\n");
		return -1;
	}

	if (strcmp(bus, "pci") == 0)
		return supports_device_pci(parent);
	else if (strcmp(bus, "acpi") == 0)
		return supports_device_acpi(parent);
	else if (strcmp(bus, "fdt") == 0)
		return supports_device_fdt(parent);

	return 0.0f;
}


module_dependency module_dependencies[] = {
	{ MMC_BUS_MODULE_NAME, (module_info**)&gMMCBusController},
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&gDeviceManager },
	{}
};

status_t
set_clock(void* controller, uint32_t kilohertz)
{
	SdhciBus* bus = (SdhciBus*)controller;

	bus->SetClock(kilohertz, true);
	return B_OK;
}


status_t
execute_command(void* controller, uint8_t command, uint32_t argument,
	uint32_t* response)
{
	SdhciBus* bus = (SdhciBus*)controller;
	return bus->ExecuteCommand(command, argument, response);
}


status_t
do_io(void* controller, uint8_t command, IOOperation* operation,
	bool offsetAsSectors)
{
	SdhciBus* bus = (SdhciBus*)controller;
	return bus->DoIO(command, operation, offsetAsSectors);
}


void
set_scan_semaphore(void* controller, sem_id sem)
{
	SdhciBus* bus = (SdhciBus*)controller;
	return bus->SetScanSemaphore(sem);
}


void
set_bus_width(void* controller, int width)
{
	SdhciBus* bus = (SdhciBus*)controller;
	return bus->SetBusWidth(width);
}


void
set_card_type(void* controller, card_type type)
{
	SdhciBus* bus = (SdhciBus*)controller;
	bus->SetCardType(type);
}


void
terminate_bus(void* controller)
{
	SdhciBus* bus = (SdhciBus*)controller;
	bus->TerminateBus();
}


// Root device that binds to the ACPI or PCI bus. It will register an mmc_bus_interface
// node for each SD slot in the device.
static driver_module_info sSDHCIDevice = {
	{
		SDHCI_DEVICE_MODULE_NAME,
		0,
		NULL
	},
	supports_device,
	register_device,
	init_device,
	uninit_device,
	register_child_devices,
	NULL,	// rescan
	NULL,	// device removed
};


module_info* modules[] = {
	(module_info* )&sSDHCIDevice,
	(module_info* )&gSDHCIPCIDeviceModule,
	(module_info* )&gSDHCIACPIDeviceModule,
	(module_info* )&gSDHCIFDTDeviceModule,
	NULL
};
