/* SPDX-License-Identifier: GPL-2.0 */
#include <linux/linkage.h>
#include <asm/assembler.h>
#include <xen/interface/xen.h>
EXPORT_SYMBOL_GPL(HYPERVISOR_event_channel_op);
EXPORT_SYMBOL_GPL(HYPERVISOR_grant_table_op);
EXPORT_SYMBOL_GPL(HYPERVISOR_xen_version);
EXPORT_SYMBOL_GPL(HYPERVISOR_console_io);
EXPORT_SYMBOL_GPL(HYPERVISOR_sched_op);
EXPORT_SYMBOL_GPL(HYPERVISOR_hvm_op);
EXPORT_SYMBOL_GPL(HYPERVISOR_memory_op);
EXPORT_SYMBOL_GPL(HYPERVISOR_physdev_op);
EXPORT_SYMBOL_GPL(HYPERVISOR_vcpu_op);
EXPORT_SYMBOL_GPL(HYPERVISOR_platform_op_raw);
EXPORT_SYMBOL_GPL(HYPERVISOR_multicall);
EXPORT_SYMBOL_GPL(HYPERVISOR_vm_assist);
EXPORT_SYMBOL_GPL(HYPERVISOR_dm_op);
EXPORT_SYMBOL_GPL(privcmd_call);
#define SBI_ECALL 0xE

    .data

#define HYPERCALL_SIMPLE(hypercall) \
SYM_FUNC_START(HYPERVISOR_##hypercall) \
    li a6, __HYPERVISOR_##hypercall; \
    li a7, SBI_ECALL; \
    mv a5, a4; \
    mv a4, a3; \
    mv a3, a2; \
    mv a2, a1; \
    mv a1, a0; \
    li a0, 0; \
    ecall; \
    ret; \
SYM_FUNC_END(HYPERVISOR_##hypercall)

#define HYPERCALL0 HYPERCALL_SIMPLE
#define HYPERCALL1 HYPERCALL_SIMPLE
#define HYPERCALL2 HYPERCALL_SIMPLE
#define HYPERCALL3 HYPERCALL_SIMPLE
#define HYPERCALL4 HYPERCALL_SIMPLE
#define HYPERCALL5 HYPERCALL_SIMPLE

    .text

HYPERCALL2(xen_version);
HYPERCALL3(console_io);
HYPERCALL3(grant_table_op);
HYPERCALL2(sched_op);
HYPERCALL2(event_channel_op);
HYPERCALL2(hvm_op);
HYPERCALL2(memory_op);
HYPERCALL2(physdev_op);
HYPERCALL3(vcpu_op);
HYPERCALL1(platform_op_raw);
HYPERCALL2(multicall);
HYPERCALL2(vm_assist);
HYPERCALL3(dm_op);

SYM_FUNC_START(privcmd_call)
    mv a6, a0
    li a7, SBI_ECALL
    mv a5, a4;
    mv a4, a3;
    mv a3, a2;
    mv a2, a1;
    mv a1, a0;
    li a0, 0;
    ecall
    ret
SYM_FUNC_END(privcmd_call);
