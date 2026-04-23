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

struct device_node *xen_node;

/**
 * struct xen_irq_fwspec - Firmware (DT) interrupt specification
 *                          for the Xen event channel interrupt
 * @hwirq: IMSIC local_id as assigned by the Xen hypervisor
 * @oirq:  Parsed DT interrupt specifier (interrupts-extended)
 */
struct xen_irq_fwspec {
    irq_hw_number_t hwirq;
    uint32_t oirq;
};

static struct xen_irq_fwspec xen_irq;

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
	struct vcpu_register_vcpu_info info;
	struct vcpu_info *vcpup;
	int err;

	/* 
	 * VCPUOP_register_vcpu_info cannot be called twice for the same
	 * vcpu, so if vcpu_info is already registered, just get out. This
	 * can happen with cpu-hotplug.
	 */
	if (per_cpu(xen_vcpu, cpu) != NULL)
		goto after_register_vcpu_info;

	pr_info("Xen: initializing cpu%d\n", cpu);
	vcpup = per_cpu_ptr(xen_vcpu_info, cpu);

	info.mfn = percpu_to_gfn(vcpup);
	info.offset = xen_offset_in_page(vcpup);

	err = HYPERVISOR_vcpu_op(VCPUOP_register_vcpu_info, xen_vcpu_nr(cpu),
				 &info);
	BUG_ON(err);
	per_cpu(xen_vcpu, cpu) = vcpup;

after_register_vcpu_info:
	enable_percpu_irq(xen_events_irq, 0);
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
	struct resource res;
	struct of_phandle_args irq_args;

	xen_node = of_find_compatible_node(NULL, NULL, "xen,xen");
	if (!xen_node) {
		pr_err("Xen support was detected before, but it has disappeared\n");
		return;
	}
	// if (of_irq_parse_one(xen_node, 0, &irq_args)){
	// 	pr_err("Xen irq arg not found\n");
	// 	return;
	// }
	//
	// if (!irq_args.args[0]){
	// 	pr_err("Phandle referencing interrupt controller missing\n");
	// 	return;
	// }
	// xen_irq.oirq = irq_args.args[0];
	//
	// if (!irq_args.args[1]){
	// 	pr_err("Interrupt specifier missing\n");
	// 	return;
	// }
	// xen_irq.hwirq = irq_args.args[1];

	if (of_address_to_resource(xen_node, GRANT_TABLE_INDEX, &res)) {
		pr_err("Xen grant table region is not found\n");
		of_node_put(xen_node);
		return;
	}
	xen_grant_frames = res.start;
}

static int __init xen_guest_init(void)
{
	struct xen_add_to_physmap xatp;
	struct shared_info *shared_info_page = NULL;
	int rc, cpu;

	if (!xen_domain())
		return 0;

	if (IS_ENABLED(CONFIG_XEN_VIRTIO))
		virtio_set_mem_acc_cb(xen_virtio_restricted_mem_acc);

	xen_dt_guest_init();
	// if (!acpi_disabled)
	// 	xen_acpi_guest_init();
	// else
	// 	xen_dt_guest_init();

	shared_info_page = (struct shared_info *)get_zeroed_page(GFP_KERNEL);

	if (!shared_info_page) {
		pr_err("not enough memory\n");
		return -ENOMEM;
	}
	xatp.domid = DOMID_SELF;
	xatp.idx = 0;
	xatp.space = XENMAPSPACE_shared_info;
	xatp.gpfn = virt_to_gfn(shared_info_page);
	if (HYPERVISOR_memory_op(XENMEM_add_to_physmap, &xatp))
		BUG();

	HYPERVISOR_shared_info = (struct shared_info *)shared_info_page;

	/* xen_vcpu is a pointer to the vcpu_info struct in the shared_info
	 * page, we use it in the event channel upcall and in some pvclock
	 * related functions. 
	 * The shared info contains exactly 1 CPU (the boot CPU). The guest
	 * is required to use VCPUOP_register_vcpu_info to place vcpu info
	 * for secondary CPUs as they are brought up.
	 * For uniformity we use VCPUOP_register_vcpu_info even on cpu0.
	 */
	xen_vcpu_info = __alloc_percpu(sizeof(struct vcpu_info),
				       1 << fls(sizeof(struct vcpu_info) - 1));
	if (xen_vcpu_info == NULL)
		return -ENOMEM;

	/* Direct vCPU id mapping for ARM guests. */
	for_each_possible_cpu(cpu)
		per_cpu(xen_vcpu_id, cpu) = cpu;

	if (!xen_grant_frames) {
		xen_auto_xlat_grant_frames.count = gnttab_max_grant_frames();
		rc = xen_xlate_map_ballooned_pages(&xen_auto_xlat_grant_frames.pfn,
										   &xen_auto_xlat_grant_frames.vaddr,
										   xen_auto_xlat_grant_frames.count);
	} else
		rc = gnttab_setup_auto_xlat_frames(xen_grant_frames);
	if (rc) {
		free_percpu(xen_vcpu_info);
		return rc;
	}
	gnttab_init();

	/*
	 * Making sure board specific code will not set up ops for
	 * cpu idle and cpu freq.
	 */
	disable_cpuidle();
	disable_cpufreq();

	xen_init_IRQ();

	return 0;

}
early_initcall(xen_guest_init);

static int xen_starting_runstate_cpu(unsigned int cpu)
{
	return 0;
}

static int __init xen_late_init(void)
{
      xen_events_irq = irq_of_parse_and_map(xen_node, 0);
      of_node_put(xen_node);

      if (!xen_events_irq){
          pr_err("Xen event channel interrupt not found\n");
          return -ENODEV;
      }

      if (request_percpu_irq(xen_events_irq, xen_riscv_callback,
                      "events", &xen_vcpu))
          return -EINVAL;

      return cpuhp_setup_state(CPUHP_AP_ONLINE_DYN,
                   "riscv/xen:starting", xen_starting_cpu,
                   xen_dying_cpu);
}
late_initcall(xen_late_init);


/* empty stubs */
void xen_arch_pre_suspend(void) { }
void xen_arch_post_suspend(int suspend_cancelled) { }
void xen_timer_resume(void) { }
void xen_arch_resume(void) { }
void xen_arch_suspend(void) { }
