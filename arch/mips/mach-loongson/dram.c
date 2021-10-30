// SPDX-License-Identifier: GPL-2.0+

#include <common.h>
#include <init.h>
#include <asm/global_data.h>
#include <linux/sizes.h>
#include <asm/addrspace.h>

DECLARE_GLOBAL_DATA_PTR;

int dram_init(void)
{
	gd->ram_size = SZ_32M;
	return 0;
}
