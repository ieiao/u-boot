// SPDX-License-Identifier: GPL-2.0+
/*
 * Authors: Weihao Li <cn.liweihao@gmail.com>
 */

#include <dm.h>
#include <init.h>
#include <syscon.h>
#include <backlight.h>
#include <asm/global_data.h>
#include <dm/uclass-internal.h>
#include <asm/arch-rockchip/hardware.h>
#include <asm/arch-rockchip/clock.h>
#include <asm/arch-rockchip/grf_rk3368.h>
#include <asm/arch-rockchip/cru_rk3368.h>
#include <asm/arch-rockchip/vop_rk3288.h>

int board_early_init_r(void)
{
	return 0;
}

#if defined(CONFIG_OF_BOARD_SETUP)
int ft_board_setup(void *blob, struct bd_info *bd)
{
	struct udevice *dev;
	void *regs;
	struct rk3288_vop *vop;

	uclass_find_first_device(UCLASS_VIDEO, &dev);
	if (dev) {
		regs = dev_read_addr_ptr(dev);
		vop = regs;
		/* VOP enter standby mode. */
		clrsetbits_le32(&vop->sys_ctrl, BIT(22), BIT(22));
	}

	uclass_find_first_device(UCLASS_PANEL_BACKLIGHT, &dev);
	if (dev)
		backlight_set_brightness(dev, 0);

	return 0;
}
#endif
