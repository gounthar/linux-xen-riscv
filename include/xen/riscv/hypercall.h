/* SPDX-License-Identifier: GPL-2.0 */
/******************************************************************************
 * hypercall.h
 *
 * Linux-specific hypervisor handling.
 *
 */

#ifndef _ASM_RISCV_XEN_HYPERCALL_H
#define _ASM_RISCV_XEN_HYPERCALL_H

#include <linux/bug.h>

#include <xen/interface/xen.h>
#include <xen/interface/sched.h>
#include <xen/interface/platform.h>

struct xen_dm_op_buf;

long privcmd_call(unsigned int call, unsigned long a1,
		unsigned long a2, unsigned long a3,
		unsigned long a4, unsigned long a5);
int HYPERVISOR_xen_version(int cmd, void *arg);
int HYPERVISOR_console_io(int cmd, int count, char *str);
int HYPERVISOR_grant_table_op(unsigned int cmd, void *uop, unsigned int count);
int HYPERVISOR_sched_op(int cmd, void *arg);
int HYPERVISOR_event_channel_op(int cmd, void *arg);
unsigned long HYPERVISOR_hvm_op(int op, void *arg);
int HYPERVISOR_memory_op(unsigned int cmd, void *arg);
int HYPERVISOR_physdev_op(int cmd, void *arg);
int HYPERVISOR_vcpu_op(int cmd, int vcpuid, void *extra_args);
int HYPERVISOR_vm_assist(unsigned int cmd, unsigned int type);
int HYPERVISOR_dm_op(domid_t domid, unsigned int nr_bufs,
			 struct xen_dm_op_buf *bufs);
int HYPERVISOR_platform_op_raw(void *arg);
static inline int HYPERVISOR_platform_op(struct xen_platform_op *op)
{
	return 0;
}
int HYPERVISOR_multicall(struct multicall_entry *calls, uint32_t nr);

static inline int HYPERVISOR_suspend(unsigned long start_info_mfn)
{
	return 0;
}

#endif /* _ASM_RISCV_XEN_HYPERCALL_H */
