// SPDX-License-Identifier: GPL-2.0-only
#include <xen/xen.h>
#include <xen/events.h>
#include <xen/grant_table.h>
#include <xen/hvm.h>
#include <xen/interface/vcpu.h>
#include <xen/interface/xen.h>
#include <xen/interface/memory.h>
#include <xen/interface/hvm/params.h>
#include <xen/features.h>
#include <xen/platform_pci.h>
#include <xen/xenbus.h>
#include <xen/page.h>
#include <xen/interface/sched.h>
#include <xen/xen-ops.h>
#include <asm/xen/hypervisor.h>
#include <asm/xen/hypercall.h>
#include <asm/efi.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/cpuidle.h>
#include <linux/cpufreq.h>
#include <linux/cpu.h>
#include <linux/console.h>
#include <linux/pvclock_gtod.h>
#include <linux/reboot.h>
#include <linux/time64.h>
#include <linux/timekeeping.h>
#include <linux/timekeeper_internal.h>
#include <linux/acpi.h>
#include <linux/virtio_anchor.h>

#include <linux/mm.h>

static struct start_info _xen_start_info;
struct start_info *xen_start_info = &_xen_start_info;
EXPORT_SYMBOL(xen_start_info);

enum xen_domain_type xen_domain_type = XEN_HVM_DOMAIN; //hacked
EXPORT_SYMBOL(xen_domain_type);

struct shared_info xen_dummy_shared_info;
struct shared_info *HYPERVISOR_shared_info = (void *)&xen_dummy_shared_info;

DEFINE_PER_CPU(struct vcpu_info *, xen_vcpu);
static struct vcpu_info __percpu *xen_vcpu_info;

/* Linux <-> Xen vCPU id mapping */
DEFINE_PER_CPU(uint32_t, xen_vcpu_id);
EXPORT_PER_CPU_SYMBOL(xen_vcpu_id);

/* These are unused until we support booting "pre-ballooned" */
unsigned long xen_released_pages;
struct xen_memory_region xen_extra_mem[XEN_EXTRA_MEM_MAX_REGIONS] __initdata;

static __read_mostly unsigned int xen_events_irq;
static __read_mostly phys_addr_t xen_grant_frames;

#define GRANT_TABLE_INDEX   0
#define EXT_REGION_INDEX    1

uint32_t xen_start_flags;
EXPORT_SYMBOL(xen_start_flags);

extern int imsic_irqdomain_init(void);

int xen_unmap_domain_gfn_range(struct vm_area_struct *vma,
					int nr, struct page **pages)
{
	return 0;
}
EXPORT_SYMBOL_GPL(xen_unmap_domain_gfn_range);

static void xen_read_wallclock(struct timespec64 *ts)
{
}

static int xen_pvclock_gtod_notify(struct notifier_block *nb,
					unsigned long was_set, void *priv)
{
	return 0;
}

static struct notifier_block xen_pvclock_gtod_notifier = {
	.notifier_call = xen_pvclock_gtod_notify,
};

static int xen_starting_cpu(unsigned int cpu)
{
	enable_percpu_irq(xen_events_irq, IRQ_TYPE_NONE);
	return 0;
}

static int xen_dying_cpu(unsigned int cpu)
{
	disable_percpu_irq(xen_events_irq);
	return 0;
}

void xen_reboot(int reason)
{
}

static int xen_restart(struct notifier_block *nb, unsigned long action,
					void *data)
{
	return 0;
}

static struct notifier_block xen_restart_nb = {
	.notifier_call = xen_restart,
	.priority = 192,
};

static void xen_power_off(void)
{
}

static irqreturn_t xen_riscv_callback(int irq, void *arg)
{
	pr_info("IRQ RECEIVED : %d\n", irq);
	xen_evtchn_do_upcall();
	return IRQ_HANDLED;
}
static __initdata struct {
	const char *compat;
	const char *prefix;
	const char *version;
	bool found;
} hyper_node = {"xen,xen", "xen,xen-", NULL, false};

static int __init fdt_find_hyper_node(unsigned long node, const char *uname,
					int depth, void *data)
{
	return 0;
}

void __init xen_early_init(void)
{
	xen_domain_type = XEN_HVM_DOMAIN;

	xen_setup_features();
	printk("DEBUG %s:%d\n", __FILE__, __LINE__);

	if (xen_feature(XENFEAT_dom0))
		xen_start_flags |= SIF_INITDOMAIN|SIF_PRIVILEGED;

	if (!console_set_on_cmdline && !xen_initial_domain())
		add_preferred_console("hvc", 0, NULL);

}

static void __init xen_dt_guest_init(void)
{
	struct device_node *xen_node;
	struct resource res;

	xen_node = of_find_compatible_node(NULL, NULL, "xen,xen");
	if (!xen_node) {
		pr_err("Xen support was detected before, but it has disappeared\n");
		return;
	}

	imsic_irqdomain_init();
	xen_events_irq = irq_of_parse_and_map(xen_node, 0);

	if (of_address_to_resource(xen_node, GRANT_TABLE_INDEX, &res)) {
		pr_err("Xen grant table region is not found\n");
		of_node_put(xen_node);
		return;
	}
	of_node_put(xen_node);
	xen_grant_frames = res.start;
}

static int __init xen_guest_init(void)
{
	if (!xen_domain())
		return 0;

	if (IS_ENABLED(CONFIG_XEN_VIRTIO))
		virtio_set_mem_acc_cb(xen_virtio_restricted_mem_acc);

	xen_dt_guest_init();
	// if (!acpi_disabled)
	// 	xen_acpi_guest_init();
	// else
	// 	xen_dt_guest_init();

	if (!xen_events_irq){
		pr_err("Xen event channel interrupt not found\n");
		return -ENODEV;
	}

	/*
	 * Making sure board specific code will not set up ops for
	 * cpu idle and cpu freq.
	 */
	disable_cpuidle();
	disable_cpufreq();

	xen_init_IRQ();

	pr_info("DEBUG : %s:%d - xen_events_irq : %d\n", __FILE__, __LINE__, xen_events_irq);
	if (request_percpu_irq(xen_events_irq, xen_riscv_callback,
			       "events", &xen_vcpu)) {
		pr_err("Error request IRQ %d\n", xen_events_irq);
		return -EINVAL;
	}
	enable_percpu_irq(xen_events_irq, IRQ_TYPE_NONE);

    return cpuhp_setup_state(CPUHP_AP_ONLINE_DYN,
				 "riscv/xen:starting", xen_starting_cpu,
				 xen_dying_cpu);
}
early_initcall(xen_guest_init);

static int xen_starting_runstate_cpu(unsigned int cpu)
{
	return 0;
}

static int __init xen_late_init(void)
{
	return 0;
}
late_initcall(xen_late_init);


/* empty stubs */
void xen_arch_pre_suspend(void) { }
void xen_arch_post_suspend(int suspend_cancelled) { }
void xen_timer_resume(void) { }
void xen_arch_resume(void) { }
void xen_arch_suspend(void) { }
