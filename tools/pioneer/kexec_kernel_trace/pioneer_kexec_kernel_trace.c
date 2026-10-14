// SPDX-License-Identifier: GPL-2.0-only
/* Temporary diagnostic for the recorded Pioneer Linux 6.1.31, not a fix.
 * No reboot, kexec load, disk/flash access, parameterized probe addresses,
 * register editing, or CPU manipulation is performed by this module.
 */
#include <linux/atomic.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/kprobes.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/string.h>
#include <linux/utsname.h>
#include "polled.h"

#ifndef CONFIG_RISCV
#error This diagnostic is only for native RISC-V Linux
#endif

static char *token;
module_param(token, charp, 0400);
MODULE_PARM_DESC(token, "Required fresh 32-lowercase-hex diagnostic token");
static bool arm;
module_param(arm, bool, 0400);
MODULE_PARM_DESC(arm, "Explicitly enable the fixed handoff checkpoints after self-test");

static void __iomem *uart;
static atomic_t handoff_seen = ATOMIC_INIT(0);
static unsigned long seen;
static unsigned int registered;
static int self_test_pass;
static int self_test_hits;
static char self_test_message[192];

struct checkpoint {
	struct kprobe probe;
	char message[192];
};

static struct checkpoint checkpoints[] = {
	{ .probe.symbol_name = "kernel_kexec" },
	{ .probe.symbol_name = "kernel_restart_prepare" },
	{ .probe.symbol_name = "device_shutdown" },
	{ .probe.symbol_name = "migrate_to_reboot_cpu" },
	{ .probe.symbol_name = "cpu_hotplug_enable" },
	{ .probe.symbol_name = "machine_shutdown" },
	{ .probe.symbol_name = "smp_shutdown_nonboot_cpus" },
	{ .probe.symbol_name = "kmsg_dump" },
	{ .probe.symbol_name = "machine_kexec" },
};

static unsigned int uart_read(void *context, unsigned int offset)
{
	return readl((void __iomem *)context + offset);
}

static void uart_write(void *context, unsigned int offset, unsigned int value)
{
	writel(value, (void __iomem *)context + offset);
}

static void uart_delay(void *context)
{
	(void)context;
	udelay(1);
}

static int emit(const char *message)
{
	static const struct PioneerKernelUartOps ops = {
		uart_read, uart_write, uart_delay
	};
	return pioneer_kernel_uart_emit(&ops, (void *)uart, message,
		strnlen(message, 192));
}
NOKPROBE_SYMBOL(emit);

static int checkpoint_enter(struct kprobe *probe, struct pt_regs *regs)
{
	struct checkpoint *point = container_of(probe, struct checkpoint, probe);
	unsigned int index = point - checkpoints;
	(void)regs; /* Never edit the saved registers or instruction pointer. */
	if (!index)
		atomic_set(&handoff_seen, 1);
	if (atomic_read(&handoff_seen) && !test_and_set_bit(index, &seen))
		emit(point->message);
	return 0;
}
NOKPROBE_SYMBOL(checkpoint_enter);

static noinline void checkpoint_self_test(void)
{
	asm volatile("nop\nnop\n" : : : "memory");
}

static int self_test_enter(struct kprobe *probe, struct pt_regs *regs)
{
	(void)probe;
	(void)regs;
	self_test_hits++;
	self_test_pass = emit(self_test_message);
	return 0;
}
NOKPROBE_SYMBOL(self_test_enter);

static void remove_checkpoints(void)
{
	while (registered)
		unregister_kprobe(&checkpoints[--registered].probe);
}

static int __init pioneer_kernel_trace_init(void)
{
	struct device_node *node;
	struct resource resource;
	u32 shift, width;
	int error;
	unsigned int i;
	struct kprobe test_probe = {
		.addr = (kprobe_opcode_t *)checkpoint_self_test,
		.pre_handler = self_test_enter,
	};
	if (strcmp(utsname()->release, "6.1.31")
		|| !token || !pioneer_kernel_token_valid(token, strnlen(token, 33))
		|| !(of_machine_is_compatible("sophgo,sg2042")
			|| of_machine_is_compatible("sophgo,mango")))
		return -EINVAL;
	node = of_find_node_by_path("/soc/serial@7040000000");
	if (!node)
		return -ENODEV;
	error = of_address_to_resource(node, 0, &resource)
		|| of_property_read_u32(node, "reg-shift", &shift)
		|| of_property_read_u32(node, "reg-io-width", &width);
	of_node_put(node);
	if (error || resource.start != 0x7040000000ULL
		|| resource_size(&resource) != 0x1000 || shift != 2 || width != 4)
		return -ENODEV;
	uart = ioremap(resource.start, resource_size(&resource));
	if (!uart)
		return -ENOMEM;
	scnprintf(self_test_message, sizeof(self_test_message),
		"\r\nPIONEER_KERNEL_TRACE v1 stage=self_test token=%s armed=%u\r\n",
		token, arm ? 1 : 0);
	error = register_kprobe(&test_probe);
	if (error)
		goto fail;
	checkpoint_self_test();
	unregister_kprobe(&test_probe);
	pr_info("Pioneer kexec trace: self-test hits=%d pass=%d LCR=%x LSR=%x\n",
		self_test_hits, self_test_pass, readl(uart + 12), readl(uart + 20));
	if (!self_test_pass) {
		error = -EIO;
		goto fail;
	}
	if (!arm) {
		pr_info("Pioneer kexec trace: self-test only; no handoff probes armed\n");
		return 0;
	}
	for (i = 0; i < ARRAY_SIZE(checkpoints); i++) {
		struct checkpoint *point = &checkpoints[i];
		scnprintf(point->message, sizeof(point->message),
			"\r\nPIONEER_KERNEL_TRACE v1 stage=%s token=%s\r\n",
			point->probe.symbol_name, token);
		point->probe.pre_handler = checkpoint_enter;
		error = register_kprobe(&point->probe);
		if (error) {
			pr_err("Pioneer kexec trace: cannot probe %s: %d\n",
				point->probe.symbol_name, error);
			goto fail;
		}
		registered++;
	}
	pr_info("Pioneer kexec trace: armed %u fixed checkpoints\n", registered);
	return 0;
fail:
	remove_checkpoints();
	iounmap(uart);
	uart = NULL;
	return error;
}

static void __exit pioneer_kernel_trace_exit(void)
{
	remove_checkpoints();
	iounmap(uart);
}

module_init(pioneer_kernel_trace_init);
module_exit(pioneer_kernel_trace_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Temporary bounded UART checkpoints for Pioneer Linux 6.1.31 kexec");
