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
	return 0;
}

static int xen_dying_cpu(unsigned int cpu)
{
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
}

static void __init xen_dt_guest_init(void)
{
}

static int __init xen_guest_init(void)
{
    return 0;
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
