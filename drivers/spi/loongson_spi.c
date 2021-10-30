// SPDX-License-Identifier: GPL-2.0
/*
 * Author: WeiHao Li
 *
 * Generic SPI driver for Loongson SoC
 */

#include <clk.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <spi.h>
#include <linux/bitops.h>
#include <linux/iopoll.h>
#include <linux/io.h>
#include <linux/log2.h>

#define SPCR			0
#define SPSR			1
#define SPDR			2
#define SPER			3
#define SFC_PARAM		4
#define SFC_SOFTCS		5
#define SFC_TIMING		6

#define SPCR_SPE		BIT(6)
#define SPCR_CPOL		BIT(3)
#define SPCR_CPHA		BIT(2)

#define SPSR_WCOL		BIT(6)
#define SPSR_WFFULL		BIT(3)
#define SPSR_WFEMPTY		BIT(2)
#define SPSR_RFFULL		BIT(1)
#define SPSR_RFEMPTY		BIT(0)

#define SPCR_SPR_MASK		0x3
#define SPER_SPRE_MASK		0x3

#define SPI_NUM_CS		4

struct loongson_spi {
	void __iomem *regs;
	u32 sys_freq;
	uint mode;
	uint speed;
};

static void loongson_spi_master_setup(struct loongson_spi *priv, int cs)
{
	u32 rate, scale;

	/* Calculate the clock divsior */
	rate = DIV_ROUND_UP(priv->sys_freq, priv->speed);
	rate = roundup_pow_of_two(rate);

	scale = ilog2(rate);

	/* Calculate the real clock */
	priv->speed = priv->sys_freq >> scale;

	clrbits_8(priv->regs + SPCR, SPCR_SPE);
	writeb(0, priv->regs + SPSR);
	
	writeb((readb(priv->regs + SPCR) & ~SPCR_SPR_MASK) |
		(scale & SPCR_SPR_MASK), priv->regs + SPCR);
	writeb((readb(priv->regs + SPER) & ~SPER_SPRE_MASK) |
		((scale >> 2) & SPER_SPRE_MASK), priv->regs + SPER);

	if (priv->mode & SPI_CPOL)
		setbits_8(priv->regs + SPCR, SPCR_CPOL);
	else
		clrbits_8(priv->regs + SPCR, SPCR_CPOL);

	if (priv->mode & SPI_CPHA)
		setbits_8(priv->regs + SPCR, SPCR_CPHA);
	else
		clrbits_8(priv->regs + SPCR, SPCR_CPHA);

	setbits_8(priv->regs + SPCR, SPCR_SPE);
}

static void loongson_spi_set_cs(struct loongson_spi *priv, int cs, bool enable)
{
	if (enable)
		loongson_spi_master_setup(priv, cs);

	if (enable)
		clrbits_8(priv->regs + SFC_SOFTCS, (0x1 << (cs + 4)));
	else
		setbits_8(priv->regs + SFC_SOFTCS, (0x1 << (cs + 4)));
}

static int loongson_spi_set_mode(struct udevice *bus, uint mode)
{
	struct loongson_spi *priv = dev_get_priv(bus);

	priv->mode = mode;

	return 0;
}

static int loongson_spi_set_speed(struct udevice *bus, uint speed)
{
	struct loongson_spi *priv = dev_get_priv(bus);

	priv->speed = speed;

	return 0;
}

static int loongson_spi_xfer(struct udevice *dev, unsigned int bitlen,
			   const void *dout, void *din, unsigned long flags)
{
	struct udevice *bus = dev->parent;
	struct loongson_spi *priv = dev_get_priv(bus);
	int total_size = bitlen >> 3;
	u8 *out_buffer = (u8 *)dout;
	u8 *in_buffer = (u8 *)din;
	int cs, ret = 0;

	cs = spi_chip_select(dev);
	if (cs < 0 || cs >= SPI_NUM_CS) {
		dev_err(dev, "loongson_spi: Invalid chip select %d\n", cs);
		return -EINVAL;
	}

	if (flags & SPI_XFER_BEGIN)
		loongson_spi_set_cs(priv, cs, true);

	while (total_size) {
		if (out_buffer) {
			writeb(*out_buffer, priv->regs + SPDR);
			out_buffer++;
		} else
			writeb(0xff, priv->regs + SPDR);

		while(readb(priv->regs + SPSR) & SPSR_RFEMPTY);

		if (din) {
			*in_buffer = readb(priv->regs + SPDR);
			in_buffer++;
		} else
			readb(priv->regs + SPDR);

		total_size--;
	}

	if (flags & SPI_XFER_END)
		loongson_spi_set_cs(priv, cs, false);

	return ret;
}

static int loongson_spi_probe(struct udevice *dev)
{
	struct loongson_spi *priv = dev_get_priv(dev);

	priv->regs = dev_remap_addr(dev);
	if (!priv->regs)
		return -EINVAL;

	priv->sys_freq = 120000000;
	writeb(BIT(0), priv->regs + SFC_PARAM);
	writeb(0xff, priv->regs + SFC_SOFTCS);

	return 0;
}

static const struct dm_spi_ops loongson_spi_ops = {
	.set_mode = loongson_spi_set_mode,
	.set_speed = loongson_spi_set_speed,
	.xfer = loongson_spi_xfer,
};

static const struct udevice_id loongson_spi_ids[] = {
	{ .compatible = "loongson,loongson-spi" },
	{ }
};

U_BOOT_DRIVER(loongson_spi) = {
	.name = "loongson_spi",
	.id = UCLASS_SPI,
	.of_match = loongson_spi_ids,
	.ops = &loongson_spi_ops,
	.priv_auto = sizeof(struct loongson_spi),
	.probe = loongson_spi_probe,
};
