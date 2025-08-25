/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) 2017 Rockchip Electronics Co., Ltd
 */

#ifndef __CONFIGS_YMD8_MB_H
#define __CONFIGS_YMD8_MB_H

#include <configs/rk3368_common.h>

#undef CFG_EXTRA_ENV_SETTINGS
#define CFG_EXTRA_ENV_SETTINGS \
	"partitions=" PARTS_DEFAULT \
	"fdtfile=" CONFIG_DEFAULT_DEVICE_TREE ".dtb\0" \
	ENV_MEM_LAYOUT_SETTINGS \
	"boot_targets=" BOOT_TARGETS "\0" \
	"splashsource=mmc_fs\0" \
	"splashdevpart=0:4\0" \
	"splashpos=m,m\0" \
	"splashimage=" __stringify(CONFIG_SYS_LOAD_ADDR) "\0" \

#endif
