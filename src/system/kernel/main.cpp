/*
 * Copyright 2018, Jérôme Duval, jerome.duval@gmail.com.
 * Copyright 2002-2020, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


/*! This is main - initializes the kernel and launches the launch_daemon */


#include <string.h>

#include <FindDirectory.h>
#include <OS.h>

#include <arch/platform.h>
#include <arch/debug_console.h>
#include <boot_device.h>
#include <boot_item.h>
#include <boot_splash.h>
#include <commpage.h>
#ifdef _COMPAT_MODE
#	include <commpage_compat.h>
#endif
#include <condition_variable.h>
#include <cpu.h>
#include <debug.h>
#include <DPC.h>
#include <elf.h>
#include <find_directory_private.h>
#include <fs/devfs.h>
#include <fs/KPath.h>
#include <interrupts.h>
#include <kdevice_manager.h>
#include <kdriver_settings.h>
#include <kernel_daemon.h>
#include <kmodule.h>
#include <kscheduler.h>
#include <ksyscalls.h>
#include <ksystem_info.h>
#include <lock.h>
#include <low_resource_manager.h>
#include <messaging.h>
#include <Notifications.h>
#include <port.h>
#include <posix/realtime_sem.h>
#include <posix/xsi_message_queue.h>
#include <posix/xsi_semaphore.h>
#include <real_time_clock.h>
#include <sem.h>
#include <smp.h>
#include <stack_protector.h>
#include <system_profiler.h>
#include <team.h>
#include <timer.h>
#include <user_debugger.h>
#include <user_mutex.h>
#include <vfs.h>
#include <vm/vm.h>
#include <boot/kernel_args.h>

#include "vm/VMAnonymousCache.h"


extern void debug_early_boot_checkpoint(const char* string);


//#define TRACE_BOOT
#ifdef TRACE_BOOT
#	define TRACE(x...) dprintf("INIT: " x)
#else
#	define TRACE(x...) ;
#endif


void *__dso_handle;

bool gKernelStartup = true;
bool gKernelShutdown = false;

static kernel_args sKernelArgs;
static uint32 sCpuRendezvous;
static uint32 sCpuRendezvous2;
static uint32 sCpuRendezvous3;

static int32 main2(void *);


static void
non_boot_cpu_init(void* args, int currentCPU)
{
	kernel_args* kernelArgs = (kernel_args*)args;
	if (currentCPU != 0)
		cpu_init_percpu(kernelArgs, currentCPU);
}


extern "C" int
_start(kernel_args *bootKernelArgs, int currentCPU)
{
	// EFI transfers control directly to the kernel and does not establish the
	// RISC-V ABI global pointer.  PIC PLT/GOT calls made during early boot rely
	// on it, so initialize it before calling any external kernel routine.
	asm volatile("la gp, __global_pointer$");

	// Bring up the platform-selected UART before the first boot diagnostic.
	// Do not use the ELF PLT here: its relocation state is not guaranteed at
	// the instant EFI transfers control to the kernel.
	typedef status_t (*debug_console_init_func)(kernel_args*);
	typedef void (*early_message_func)(const char*);
	debug_console_init_func initConsole;
	early_message_func earlyMessage;
	asm volatile("lla %0, arch_debug_console_init" : "=r"(initConsole));
	asm volatile("lla %0, debug_early_boot_message" : "=r"(earlyMessage));
	initConsole(bootKernelArgs);
	earlyMessage("riscv: kernel _start\n");
	if (bootKernelArgs->version == CURRENT_KERNEL_ARGS_VERSION
		&& bootKernelArgs->kernel_args_size == kernel_args_size_v1) {
		sKernelArgs.ucode_data = NULL;
		sKernelArgs.ucode_data_size = 0;
	} else if (bootKernelArgs->kernel_args_size != sizeof(kernel_args)
		|| bootKernelArgs->version != CURRENT_KERNEL_ARGS_VERSION) {
		// This is something we cannot handle right now - release kernels
		// should always be able to handle the kernel_args of earlier
		// released kernels.
		debug_early_boot_message("Version mismatch between boot loader and "
			"kernel!\n");
		return -1;
	}

	smp_set_num_cpus(bootKernelArgs->num_cpus);
	debug_early_boot_message("riscv: cpu count set\n");

	// The Pioneer bootstrap currently starts only the boot CPU. Avoid the
	// atomic rendezvous operation until secondary-hart bring-up is complete.
	if (bootKernelArgs->num_cpus > 1)
		smp_cpu_rendezvous(&sCpuRendezvous);
	debug_early_boot_message("riscv: rendezvous 1\n");

	// the passed in kernel args are in a non-allocated range of memory
	if (currentCPU == 0)
		memcpy((void*)&sKernelArgs, bootKernelArgs, bootKernelArgs->kernel_args_size);

	if (bootKernelArgs->num_cpus > 1)
		smp_cpu_rendezvous(&sCpuRendezvous2);
	debug_early_boot_message("riscv: rendezvous 2\n");

	// do any pre-booting cpu config
	debug_early_boot_message("riscv: cpu preboot\n");
	cpu_preboot_init_percpu(&sKernelArgs, currentCPU);
	debug_early_boot_message("riscv: thread preboot\n");
	thread_preboot_init_percpu(&sKernelArgs, currentCPU);
	debug_early_boot_message("riscv: cpu ready\n");

	// if we're not a boot cpu, spin here until someone wakes us up
	// The Pioneer port currently runs only the boot hart. Do not enter the
	// multi-hart trap/rendezvous path, which uses unsupported early atomics.
	if (bootKernelArgs->num_cpus <= 1
		|| smp_trap_non_boot_cpus(currentCPU, &sCpuRendezvous3)) {
		// init platform
		debug_early_boot_message("riscv: platform init\n");
		arch_platform_init(&sKernelArgs);
		debug_early_boot_message("riscv: debug init\n");

		// setup debug output
		// The Pioneer firmware resets the machine when the very verbose bootstrap
		// diagnostics keep it in early kernel startup for too long.  Suppress the
		// low-level probes before debug_init() starts allocating and initializing
		// its subsystems; explicit checkpoints below remain visible.
		debug_suppress_early_boot_messages(true);
		debug_init(&sKernelArgs);
		debug_early_boot_checkpoint("riscv: debug ready\n");
		debug_early_boot_message("riscv: dprintf enable\n");
		set_dprintf_enabled(true);
		debug_early_boot_message("riscv: dprintf enabled\n");
		// Formatted kernel debug output still uses early atomic state that is not
		// available on the Pioneer single-hart bootstrap. Keep the safe early
		// console path until normal SMP/atomic support is brought up.
		if (bootKernelArgs->num_cpus > 1) {
			dprintf("Welcome to kernel debugger output!\n");
			dprintf("Haiku revision: %s, debug level: %d\n", get_haiku_revision(),
				KDEBUG_LEVEL);
		}
		debug_early_boot_message("riscv: cpu init\n");

		// init modules
		TRACE("init CPU\n");
		cpu_init(&sKernelArgs);
		debug_early_boot_message("riscv: cpu initialized\n");
		debug_early_boot_message("riscv: cpu percpu\n");
		cpu_init_percpu(&sKernelArgs, currentCPU);
		debug_early_boot_message("riscv: cpu percpu ready\n");
		TRACE("init interrupts\n");
		debug_early_boot_message("riscv: interrupts init\n");
		interrupts_init(&sKernelArgs);
		debug_early_boot_message("riscv: interrupts ready\n");

		TRACE("init VM\n");
		debug_early_boot_message("riscv: vm init\n");
		// Avoid the unresolved PLT entry for the first VM call during the
		// Pioneer bootstrap. Earlier setup has established that the direct
		// kernel address space is valid; this distinguishes a linker-stub hang
		// from a VM initialization fault.
		register kernel_args* vmArgs asm("a0") = &sKernelArgs;
		asm volatile("call vm_init" : "+r"(vmArgs) : : "ra", "memory");
		debug_early_boot_checkpoint("riscv: vm ready\n");
			// Before vm_init_post_sem() is called, we have to make sure that
			// the boot loader allocated region is not used anymore
		boot_item_init();
		debug_early_boot_message("riscv: boot items ready\n");
#if defined(__riscv)
		// debug_init_post_vm() starts by allocating debugger command records.
		// The Pioneer bootstrap heap cannot service those allocations yet, so
		// defer this optional debugger setup while continuing kernel bring-up.
		debug_early_boot_message("riscv: debug post vm deferred\n");
#else
		debug_init_post_vm(&sKernelArgs);
#endif
		debug_early_boot_message("riscv: low resource init\n");
		low_resource_manager_init();
		debug_early_boot_checkpoint("riscv: low resource ready\n");

		// now we can use the heap and create areas
		debug_early_boot_message("riscv: platform post vm init\n");
		arch_platform_init_post_vm(&sKernelArgs);
		debug_early_boot_message("riscv: platform post vm ready\n");
		debug_early_boot_message("riscv: lock debug init\n");
#if defined(__riscv)
		// This routine only registers debugger commands, which allocate from the
		// general heap before the Pioneer bootstrap can safely service them.
		debug_early_boot_message("riscv: lock debug deferred\n");
#else
		lock_debug_init();
#endif
		debug_early_boot_message("riscv: lock debug ready\n");
		TRACE("init driver_settings\n");
		debug_early_boot_message("riscv: driver settings init\n");
		driver_settings_init(&sKernelArgs);
		debug_early_boot_message("riscv: driver settings ready\n");
		debug_init_post_settings(&sKernelArgs);
		debug_early_boot_message("riscv: debug post settings ready\n");
		TRACE("init notification services\n");
		debug_early_boot_message("riscv: notifications init\n");
		notifications_init();
		debug_early_boot_message("riscv: notifications ready\n");
		TRACE("init teams\n");
		debug_early_boot_message("riscv: teams init\n");
		team_init(&sKernelArgs);
		debug_early_boot_message("riscv: teams ready\n");
		TRACE("init ELF loader\n");
		debug_early_boot_message("riscv: elf init\n");
		elf_init(&sKernelArgs);
		debug_early_boot_message("riscv: elf ready\n");
		TRACE("init modules\n");
		debug_early_boot_message("riscv: modules init\n");
		module_init(&sKernelArgs);
		debug_early_boot_checkpoint("riscv: modules ready\n");
		TRACE("init semaphores\n");
		debug_early_boot_message("riscv: semaphores init\n");
		haiku_sem_init(&sKernelArgs);
		debug_early_boot_checkpoint("riscv: semaphores ready\n");
		TRACE("init interrupts post vm\n");
		debug_early_boot_message("riscv: interrupts post vm init\n");
		interrupts_init_post_vm(&sKernelArgs);
		debug_early_boot_message("riscv: interrupts post vm ready\n");
		debug_early_boot_message("riscv: cpu post vm init\n");
		cpu_init_post_vm(&sKernelArgs);
		debug_early_boot_message("riscv: cpu post vm ready\n");
		debug_early_boot_message("riscv: commpage init\n");
		commpage_init();
		debug_early_boot_message("riscv: commpage ready\n");
#ifdef _COMPAT_MODE
		commpage_compat_init();
#endif
		debug_early_boot_message("riscv: secondary cpu init\n");
		call_all_cpus_sync(non_boot_cpu_init, &sKernelArgs);
		debug_early_boot_message("riscv: secondary cpu ready\n");

		TRACE("init system info\n");
		debug_early_boot_message("riscv: system info init\n");
		system_info_init(&sKernelArgs);
		debug_early_boot_message("riscv: system info ready\n");

		TRACE("init SMP\n");
		debug_early_boot_message("riscv: smp init\n");
		smp_init(&sKernelArgs);
		debug_early_boot_message("riscv: smp ready\n");
		debug_early_boot_message("riscv: cpu topology init\n");
		cpu_build_topology_tree();
		debug_early_boot_message("riscv: cpu topology ready\n");
		TRACE("init timer\n");
		debug_early_boot_message("riscv: timer init\n");
		timer_init(&sKernelArgs);
		debug_early_boot_message("riscv: timer ready\n");
		TRACE("init real time clock\n");
		debug_early_boot_message("riscv: rtc init\n");
		rtc_init(&sKernelArgs);
		timer_init_post_rtc();
		debug_early_boot_message("riscv: rtc ready\n");

		TRACE("init condition variables\n");
		debug_early_boot_message("riscv: condition variables init\n");
		condition_variable_init();
		debug_early_boot_message("riscv: condition variables ready\n");

		// now we can create and use semaphores
		TRACE("init VM semaphores\n");
		debug_early_boot_message("riscv: vm post sem init\n");
		vm_init_post_sem(&sKernelArgs);
		debug_early_boot_message("riscv: vm post sem ready\n");
		TRACE("init generic syscall\n");
		debug_early_boot_message("riscv: generic syscall init\n");
		generic_syscall_init();
		smp_init_post_generic_syscalls();
		debug_early_boot_message("riscv: generic syscall ready\n");
		TRACE("init scheduler\n");
		debug_early_boot_message("riscv: scheduler init\n");
		scheduler_init();
		debug_early_boot_checkpoint("riscv: scheduler ready\n");
		TRACE("init threads\n");
		debug_early_boot_message("riscv: threads init\n");
		thread_init(&sKernelArgs);
		debug_early_boot_checkpoint("riscv: threads ready\n");
		TRACE("init kernel daemons\n");
		debug_early_boot_message("riscv: kernel daemons init\n");
		debug_suppress_early_boot_messages(true);
		kernel_daemon_init();
		debug_early_boot_checkpoint("riscv: kernel daemons ready\n");
		TRACE("init stack protector\n");
		debug_early_boot_checkpoint("riscv: stack protector init\n");
		stack_protector_init();
		debug_early_boot_checkpoint("riscv: stack protector ready\n");
		debug_early_boot_checkpoint("riscv: platform post thread init\n");
		arch_platform_init_post_thread(&sKernelArgs);
		debug_early_boot_checkpoint("riscv: platform post thread ready\n");

		TRACE("init I/O interrupts\n");
		debug_early_boot_checkpoint("riscv: io interrupts init\n");
		interrupts_init_io(&sKernelArgs);
		debug_early_boot_checkpoint("riscv: io interrupts ready\n");
		TRACE("init VM threads\n");
		debug_early_boot_checkpoint("riscv: vm post thread init\n");
		vm_init_post_thread(&sKernelArgs);
		low_resource_manager_init_post_thread();
		debug_early_boot_checkpoint("riscv: vm post thread ready\n");
		TRACE("init DPC\n");
		debug_early_boot_checkpoint("riscv: dpc init\n");
		dpc_init();
		debug_early_boot_checkpoint("riscv: dpc ready\n");
		TRACE("init VFS\n");
		debug_early_boot_checkpoint("riscv: vfs init\n");
		vfs_init(&sKernelArgs);
		debug_early_boot_checkpoint("riscv: vfs ready\n");
#if ENABLE_SWAP_SUPPORT
		TRACE("init swap support\n");
		swap_init();
#endif
		TRACE("init POSIX semaphores\n");
		debug_early_boot_checkpoint("riscv: posix ipc init\n");
		realtime_sem_init();
		xsi_sem_init();
		xsi_msg_init();
		debug_early_boot_checkpoint("riscv: posix ipc ready\n");

		// Start a thread to finish initializing the rest of the system. Note,
		// it won't be scheduled before calling scheduler_start() (on any CPU).
		TRACE("spawning main2 thread\n");
		debug_early_boot_checkpoint("riscv: main2 spawn\n");
		thread_id thread = spawn_kernel_thread(&main2, "main2",
			B_NORMAL_PRIORITY, NULL);
		resume_thread(thread);
		debug_early_boot_checkpoint("riscv: main2 ready\n");

		// We're ready to start the scheduler and enable interrupts on all CPUs.
		scheduler_enable_scheduling();
		debug_early_boot_checkpoint("riscv: scheduling enabled\n");

		// bring up the AP cpus in a lock step fashion
		TRACE("waking up AP cpus\n");
		int32 cpuCount = smp_get_num_cpus();
		if (cpuCount > 1) {
			debug_early_boot_checkpoint("riscv: smp release init\n");
			sCpuRendezvous = sCpuRendezvous2 = 0;
			smp_wake_up_non_boot_cpus();
			debug_early_boot_checkpoint("riscv: smp aps released\n");
			smp_cpu_rendezvous(&sCpuRendezvous); // wait until they're booted
			debug_early_boot_checkpoint("riscv: smp aps ready\n");
		} else {
			debug_early_boot_checkpoint("riscv: smp single cpu\n");
		}

		// exit the kernel startup phase (mutexes, etc work from now on out)
		TRACE("exiting kernel startup\n");
		gKernelStartup = false;
		debug_suppress_early_boot_messages(false);
		debug_early_boot_checkpoint("riscv: kernel startup complete\n");

		if (cpuCount > 1) {
			smp_cpu_rendezvous(&sCpuRendezvous2);
				// release the AP cpus to go enter the scheduler
		}

		TRACE("starting scheduler on cpu 0 and enabling interrupts\n");
		debug_early_boot_checkpoint("riscv: scheduler start\n");
		scheduler_start();
		debug_early_boot_checkpoint("riscv: scheduler started\n");
		enable_interrupts();
		debug_early_boot_checkpoint("riscv: interrupts enabled\n");
	} else {
		// lets make sure we're in sync with the main cpu
		// the boot processor has probably been sending us
		// tlb sync messages all along the way, but we've
		// been ignoring them
		arch_cpu_global_tlb_invalidate();

		// this is run for each non boot processor after they've been set loose
		smp_per_cpu_init(&sKernelArgs, currentCPU);

		// wait for all other AP cpus to get to this point
		smp_cpu_rendezvous(&sCpuRendezvous);
		smp_cpu_rendezvous(&sCpuRendezvous2);

		// welcome to the machine
		scheduler_start();
		enable_interrupts();
	}

#ifdef TRACE_BOOT
	// We disable interrupts for this dprintf(), since otherwise dprintf()
	// would acquires a mutex, which is something we must not do in an idle
	// thread, or otherwise the scheduler would be seriously unhappy.
	disable_interrupts();
	TRACE("main: done... begin idle loop on cpu %d\n", currentCPU);
	enable_interrupts();
#endif

	for (;;)
		cpu_idle();

	return 0;
}


static int32
main2(void* /*unused*/)
{
	debug_early_boot_checkpoint("riscv: main2 entered\n");
	TRACE("start of main2: initializing devices\n");

#if SYSTEM_PROFILER
	start_system_profiler(SYSTEM_PROFILE_SIZE, SYSTEM_PROFILE_STACK_DEPTH,
		SYSTEM_PROFILE_INTERVAL);
#endif
	boot_splash_init(sKernelArgs.boot_splash);
	debug_early_boot_checkpoint("riscv: main2 boot splash ready\n");

	commpage_init_post_cpus();
#ifdef _COMPAT_MODE
	commpage_compat_init_post_cpus();
#endif
	debug_early_boot_checkpoint("riscv: main2 commpage ready\n");

	TRACE("init ports\n");
	port_init(&sKernelArgs);
	debug_early_boot_checkpoint("riscv: main2 ports ready\n");

	TRACE("init user mutex\n");
	user_mutex_init();
	debug_early_boot_checkpoint("riscv: main2 user mutex ready\n");

	TRACE("init system notifications\n");
	system_notifications_init();
	debug_early_boot_checkpoint("riscv: main2 notifications ready\n");

	scheduler_loadavg_init();
	debug_early_boot_checkpoint("riscv: main2 loadavg ready\n");

	TRACE("Init modules\n");
	boot_splash_set_stage(BOOT_SPLASH_STAGE_1_INIT_MODULES);
	module_init_post_threads();
	debug_early_boot_checkpoint("riscv: main2 modules ready\n");

	// init userland debugging
	TRACE("Init Userland debugging\n");
	init_user_debug();
	debug_early_boot_checkpoint("riscv: main2 user debug ready\n");

	// init the messaging service
	TRACE("Init Messaging Service\n");
	init_messaging_service();
	debug_early_boot_checkpoint("riscv: main2 messaging ready\n");

	/* bootstrap all the filesystems */
	TRACE("Bootstrap file systems\n");
	boot_splash_set_stage(BOOT_SPLASH_STAGE_2_BOOTSTRAP_FS);
	vfs_bootstrap_file_systems();
	debug_early_boot_checkpoint("riscv: main2 filesystems ready\n");

	TRACE("Init Device Manager\n");
	boot_splash_set_stage(BOOT_SPLASH_STAGE_3_INIT_DEVICES);
	device_manager_init(&sKernelArgs);
	debug_early_boot_checkpoint("riscv: main2 device manager ready\n");

	TRACE("Add preloaded old-style drivers\n");
	legacy_driver_add_preloaded(&sKernelArgs);

	interrupts_init_post_device_manager(&sKernelArgs);

	TRACE("Mount boot file system\n");
	boot_splash_set_stage(BOOT_SPLASH_STAGE_4_MOUNT_BOOT_FS);
	vfs_mount_boot_file_system(&sKernelArgs);

#if ENABLE_SWAP_SUPPORT
	TRACE("swap_init_post_modules\n");
	swap_init_post_modules();
#endif

	// CPU specific modules may now be available
	boot_splash_set_stage(BOOT_SPLASH_STAGE_5_INIT_CPU_MODULES);
	cpu_init_post_modules(&sKernelArgs);

	TRACE("vm_init_post_modules\n");
	boot_splash_set_stage(BOOT_SPLASH_STAGE_6_INIT_VM_MODULES);
	vm_init_post_modules(&sKernelArgs);

	TRACE("debug_init_post_modules\n");
	debug_init_post_modules(&sKernelArgs);

	TRACE("device_manager_init_post_modules\n");
	device_manager_init_post_modules(&sKernelArgs);

	boot_splash_set_stage(BOOT_SPLASH_STAGE_7_RUN_BOOT_SCRIPT);
	boot_splash_uninit();
		// NOTE: We could introduce a syscall to draw more icons indicating
		// stages in the boot script itself. Then we should not free the image.
		// In that case we should copy it over to the kernel heap, so that we
		// can still free the kernel args.

	// The boot splash screen is the last user of the kernel args.
	// Note: don't confuse the kernel_args structure (which is never freed)
	// with the kernel args ranges it contains (and which are freed here).
	vm_free_kernel_args(&sKernelArgs);

	// start the init process
	{
		KPath serverPath;
		status_t status = __find_directory(B_SYSTEM_SERVERS_DIRECTORY,
			gBootDevice, false, serverPath.LockBuffer(),
			serverPath.BufferSize());
		if (status != B_OK)
			dprintf("main2: find_directory() failed: %s\n", strerror(status));
		serverPath.UnlockBuffer();
		status = serverPath.Append("/launch_daemon");
		if (status != B_OK) {
			dprintf("main2: constructing path to launch_daemon failed: %s\n",
			strerror(status));
		}

		const char* args[] = { serverPath.Path(), NULL };
		int32 argc = 1;
		thread_id thread;

		thread = load_image(argc, args, NULL);
		if (thread >= B_OK) {
			resume_thread(thread);
			TRACE("launch_daemon started\n");
		} else {
			dprintf("error starting \"%s\" error = %" B_PRId32 " \n",
				args[0], thread);
		}
	}

	return 0;
}
