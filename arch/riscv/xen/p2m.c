// SPDX-License-Identifier: GPL-2.0-only
#include <linux/memblock.h>
#include <linux/gfp.h>
#include <linux/export.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/dma-mapping.h>
#include <linux/vmalloc.h>
#include <linux/swiotlb.h>

#include <xen/xen.h>
#include <xen/interface/memory.h>
#include <xen/grant_table.h>
#include <xen/page.h>
#include <xen/swiotlb-xen.h>

#include <asm/cacheflush.h>
#include <asm/xen/hypercall.h>
#include <asm/xen/interface.h>

struct xen_p2m_entry {
	unsigned long pfn;
	unsigned long mfn;
	unsigned long nr_pages;
	struct rb_node rbnode_phys;
};

static rwlock_t p2m_lock;
struct rb_root phys_to_mach = RB_ROOT;
EXPORT_SYMBOL_GPL(phys_to_mach);

static int xen_add_phys_to_mach_entry(struct xen_p2m_entry *new)
{

	return 0;
}

unsigned long __pfn_to_mfn(unsigned long pfn)
{
	return 0;
}
EXPORT_SYMBOL_GPL(__pfn_to_mfn);

int set_foreign_p2m_mapping(struct gnttab_map_grant_ref *map_ops,
					struct gnttab_map_grant_ref *kmap_ops,
					struct page **pages, unsigned int count)
{
	return 0;
}

int clear_foreign_p2m_mapping(struct gnttab_unmap_grant_ref *unmap_ops,
					struct gnttab_unmap_grant_ref *kunmap_ops,
					struct page **pages, unsigned int count)
{
	return 0;
}

bool __set_phys_to_machine_multi(unsigned long pfn,
					unsigned long mfn, unsigned long nr_pages)
{
	return true;
}
EXPORT_SYMBOL_GPL(__set_phys_to_machine_multi);

bool __set_phys_to_machine(unsigned long pfn, unsigned long mfn)
{
	return true;
}
EXPORT_SYMBOL_GPL(__set_phys_to_machine);

static int p2m_init(void)
{
	return 0;
}
arch_initcall(p2m_init);
