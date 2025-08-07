// SPDX-License-Identifier: GPL-2.0+
/*
 * Authors: Weihao Li <cn.liweihao@gmail.com>
 */

#include <init.h>
#include <syscon.h>
#include <asm/global_data.h>
#include <asm/arch-rockchip/hardware.h>
#include <asm/arch-rockchip/clock.h>
#include <asm/arch-rockchip/grf_rk3368.h>
#include <asm/arch-rockchip/cru_rk3368.h>

int board_early_init_r(void)
{
	return 0;
}

#if defined(CONFIG_OF_BOARD_SETUP)
int ft_board_setup(void *blob, struct bd_info *bd)
{
	if (IS_ENABLED(CONFIG_FDT_SIMPLEFB))
		fdt_simplefb_enable_and_mem_rsv(blob);

	return 0;
}
#endif
