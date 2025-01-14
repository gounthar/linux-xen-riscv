/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_RISCV_SWIOTLB_XEN_H
#define _ASM_RISCV_SWIOTLB_XEN_H

#include <xen/features.h>
#include <xen/xen.h>

static inline int xen_swiotlb_detect(void)
{
	return 0;
}

#endif /* _ASM_RISCV_SWIOTLB_XEN_H */
