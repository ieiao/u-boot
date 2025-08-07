// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2022 Ondrej Jirman <megi@xff.cz>
 */
#include <backlight.h>
#include <dm.h>
#include <mipi_dsi.h>
#include <panel.h>
#include <asm/gpio.h>
#include <dm/device_compat.h>
#include <linux/delay.h>
#include <power/regulator.h>

struct rm72014_panel_priv {
	struct udevice *reg;
	struct gpio_desc reset;
	struct udevice *backlight;
};

static const struct display_timing default_timing = {
	.pixelclock.typ		= 80000000,
	.hactive.typ		= 800,
	.hfront_porch.typ	= 210,
	.hback_porch.typ	= 18,
	.hsync_len.typ		= 18,
	.vactive.typ		= 1280,
	.vfront_porch.typ	= 8,
	.vback_porch.typ	= 6,
	.vsync_len.typ		= 6,
	.flags			= DISPLAY_FLAGS_VSYNC_LOW | DISPLAY_FLAGS_HSYNC_LOW,
};

#define dsi_dcs_write_seq(device, seq...) do {					\
		static const u8 d[] = { seq };					\
		int ret;							\
		ret = mipi_dsi_dcs_write_buffer(device, d, ARRAY_SIZE(d));	\
		if (ret < 0)							\
			return ret;						\
	} while (0)

static int rm72014_init_sequence(struct udevice *dev)
{
	struct mipi_dsi_panel_plat *plat = dev_get_plat(dev);
	struct mipi_dsi_device *device = plat->device;
	int ret;

	dsi_dcs_write_seq(device, 0x53, 0x24);
	dsi_dcs_write_seq(device, 0xf0, 0x5a, 0x5a);
	mdelay(30);
	dsi_dcs_write_seq(device, 0x11);
	mdelay(120);
	dsi_dcs_write_seq(device, 0x29);
	mdelay(30);
	dsi_dcs_write_seq(device, 0xc3, 0x40, 0x00, 0x28);
	dsi_dcs_write_seq(device, 0x50, 0x77);
	dsi_dcs_write_seq(device, 0xe1, 0x66);
	dsi_dcs_write_seq(device, 0xdc, 0x67);
	dsi_dcs_write_seq(device, 0xd3, 0xc8);
	dsi_dcs_write_seq(device, 0x50, 0x00);
	dsi_dcs_write_seq(device, 0xf0, 0x5a);
	dsi_dcs_write_seq(device, 0xf5, 0x80);
	mdelay(120);

	ret = mipi_dsi_dcs_exit_sleep_mode(device);
	if (ret)
		return ret;

	/* Panel is operational 120 msec after reset */
	mdelay(120);

	ret = mipi_dsi_dcs_set_display_on(device);
	if (ret)
		return ret;

	return 0;
}

static int rm72014_panel_enable_backlight(struct udevice *dev)
{
	struct mipi_dsi_panel_plat *plat = dev_get_plat(dev);
	struct mipi_dsi_device *device = plat->device;
	struct rm72014_panel_priv *priv = dev_get_priv(dev);
	int ret;

	ret = mipi_dsi_attach(device);
	if (ret < 0) {
		printf("mipi_dsi_attach failed %d\n", ret);
		return ret;
	}

	ret = rm72014_init_sequence(dev);
	if (ret) {
		printf("rm72014_init_sequence failed %d\n", ret);
		return ret;
	}

	if (priv->backlight) {
		ret = backlight_enable(priv->backlight);
		if (ret) {
			printf("backlight enabled failed %d\n", ret);
			return ret;
		}

		backlight_set_brightness(priv->backlight, 60);
	}

	mdelay(10);

	return 0;
}

static int rm72014_panel_get_display_timing(struct udevice *dev,
					   struct display_timing *timings)
{
	memcpy(timings, &default_timing, sizeof(*timings));

	return 0;
}

static int rm72014_panel_of_to_plat(struct udevice *dev)
{
	struct rm72014_panel_priv *priv = dev_get_priv(dev);
	int ret;

	if (CONFIG_IS_ENABLED(DM_REGULATOR)) {
		ret = uclass_get_device_by_phandle(UCLASS_REGULATOR, dev,
						   "power-supply", &priv->reg);
		if (ret) {
			debug("%s: Warning: cannot get power supply: ret=%d\n",
			      __func__, ret);
			if (ret != -ENOENT)
				return ret;
		}
	}

	ret = uclass_get_device_by_phandle(UCLASS_PANEL_BACKLIGHT, dev,
					   "backlight", &priv->backlight);
	if (ret)
		dev_warn(dev, "failed to get backlight\n");

	ret = gpio_request_by_name(dev, "reset-gpios", 0, &priv->reset,
				   GPIOD_IS_OUT);
	if (ret) {
		dev_err(dev, "warning: cannot get reset GPIO (%d)\n", ret);
		if (ret != -ENOENT)
			return ret;
	}

	return 0;
}

static int rm72014_panel_probe(struct udevice *dev)
{
	struct rm72014_panel_priv *priv = dev_get_priv(dev);
	struct mipi_dsi_panel_plat *plat = dev_get_plat(dev);
	int ret;

	ret = regulator_set_enable_if_allowed(priv->reg, true);
	if (ret && ret != -ENOSYS) {
		debug("%s: failed to enable regulator '%s' %d\n",
		      __func__, priv->reg->name, ret);
		return ret;
	}

	dm_gpio_set_value(&priv->reset, 0);
	mdelay(5);
	dm_gpio_set_value(&priv->reset, 1);

	mdelay(180);

	/* fill characteristics of DSI data link */
	plat->lanes = 4;
	plat->format = MIPI_DSI_FMT_RGB888;
	plat->mode_flags = MIPI_DSI_MODE_VIDEO |
			   MIPI_DSI_MODE_VIDEO_BURST;

	return 0;
}

static const struct panel_ops rm72014_panel_ops = {
	.enable_backlight = rm72014_panel_enable_backlight,
	.get_display_timing = rm72014_panel_get_display_timing,
};

static const struct udevice_id rm72014_panel_ids[] = {
	{ .compatible = "unknown,rm72014" },
	{ }
};

U_BOOT_DRIVER(rm72014_panel) = {
	.name		= "rm72014_panel",
	.id		= UCLASS_PANEL,
	.of_match	= rm72014_panel_ids,
	.ops		= &rm72014_panel_ops,
	.of_to_plat	= rm72014_panel_of_to_plat,
	.probe		= rm72014_panel_probe,
	.plat_auto	= sizeof(struct mipi_dsi_panel_plat),
	.priv_auto	= sizeof(struct rm72014_panel_priv),
};
