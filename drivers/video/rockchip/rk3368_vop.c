// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2017 Theobroma Systems Design und Consulting GmbH
 * Copyright (c) 2015 Google, Inc
 * Copyright 2014 Rockchip Inc.
 */

#include <display.h>
#include <dm.h>
#include <regmap.h>
#include <syscon.h>
#include <video.h>
#include <asm/global_data.h>
#include <asm/arch-rockchip/clock.h>
#include <asm/arch-rockchip/hardware.h>
#include <linux/delay.h>
#include "rk_vop.h"

DECLARE_GLOBAL_DATA_PTR;

static void rk3368_set_pin_polarity(struct udevice *dev,
				    enum vop_modes mode, u32 polarity)
{
	struct rk_vop_priv *priv = dev_get_priv(dev);
	struct rk3288_vop *regs = priv->regs;

	/* The RK3368 VOP (v3.2) has its polarity configuration in ctrl0 */
	clrsetbits_le32(&regs->dsp_ctrl0,
			M_DSP_DCLK_POL | M_DSP_DEN_POL |
			M_DSP_VSYNC_POL | M_DSP_HSYNC_POL,
			V_DSP_PIN_POL(polarity));
}

/*
 * Try some common regulators. We should really get these from the
 * device tree somehow.
 */
static const char * const rk3368_regulator_names[] = {
	"vcc18_lcd",
	"VCC18_LCD",
	"vdd10_lcd_pwren_h",
	"vdd10_lcd",
	"VDD10_LCD",
	"vcc33_lcd"
};

static int rk3368_vop_probe(struct udevice *dev)
{
	/* Before relocation we don't need to do anything */
	if (!(gd->flags & GD_FLG_RELOC))
		return 0;

	/* Probe regulators required for the RK3368 VOP */
	rk_vop_probe_regulators(dev, rk3368_regulator_names,
				ARRAY_SIZE(rk3368_regulator_names));

	return rk_vop_probe(dev);
}

struct rkvop_driverdata rk3368_driverdata = {
	.set_pin_polarity = rk3368_set_pin_polarity,
};

static const struct udevice_id rk3368_vop_ids[] = {
	{ .compatible = "rockchip,rk3368-vop",
	  .data = (ulong)&rk3368_driverdata },
	{ }
};

static const struct video_ops rk3368_vop_ops = {
};

U_BOOT_DRIVER(rockchip_rk3368_vop) = {
	.name		= "rockchip_rk3368_vop",
	.id		= UCLASS_VIDEO,
	.of_match 	= rk3368_vop_ids,
	.ops		= &rk3368_vop_ops,
	.bind		= rk_vop_bind,
	.probe		= rk3368_vop_probe,
	.priv_auto	= sizeof(struct rk_vop_priv),
};
