// SPDX-License-Identifier: GPL-2.0

#include <div64.h>
#include <dm.h>
#include <reset.h>
#include <wdt.h>
#include <linux/bitops.h>
#include <linux/io.h>

struct ls1c_wdt {
	void __iomem *regs;
	u64 timeout;
};

#define TIMER_FREQ			120000000
#define TIMER_MASK			0xffffffff

#define WDT_EN				0x00
#define WDT_TIMER			0x04
#define WDT_SET				0x08

#define TIMER_ENABLE			BIT(0)

static void ls1c_wdt_ping(struct ls1c_wdt *priv)
{
	u64 val;

	val = TIMER_FREQ * priv->timeout;
	do_div(val, 1000);

	if (val > TIMER_MASK)
		val = TIMER_MASK;

	writel(val, priv->regs + WDT_TIMER);
	setbits_32(priv->regs + WDT_SET, TIMER_ENABLE);
}

static int ls1c_wdt_start(struct udevice *dev, u64 ms, ulong flags)
{
	struct ls1c_wdt *priv = dev_get_priv(dev);

	priv->timeout = ms;

	setbits_32(priv->regs + WDT_EN, TIMER_ENABLE);
	ls1c_wdt_ping(priv);

	return 0;
}

static int ls1c_wdt_stop(struct udevice *dev)
{
	struct ls1c_wdt *priv = dev_get_priv(dev);

	clrbits_32(priv->regs + WDT_EN, TIMER_ENABLE);

	return 0;
}

static int ls1c_wdt_reset(struct udevice *dev)
{
	struct ls1c_wdt *priv = dev_get_priv(dev);

	ls1c_wdt_ping(priv);

	return 0;
}

static int ls1c_wdt_expire(struct udevice *dev, ulong flags)
{
	ls1c_wdt_start(dev, 1, flags);
	return 0;
}

static int ls1c_wdt_probe(struct udevice *dev)
{
	struct ls1c_wdt *priv = dev_get_priv(dev);

	priv->regs = dev_remap_addr(dev);
	if (!priv->regs)
		return -EINVAL;

	ls1c_wdt_stop(dev);

	return 0;
}

static const struct wdt_ops ls1c_wdt_ops = {
	.start = ls1c_wdt_start,
	.reset = ls1c_wdt_reset,
	.stop = ls1c_wdt_stop,
	.expire_now = ls1c_wdt_expire
};

static const struct udevice_id ls1c_wdt_ids[] = {
	{ .compatible = "loongson,ls1c-wdt" },
	{}
};

U_BOOT_DRIVER(ls1c_wdt) = {
	.name = "ls1c_wdt",
	.id = UCLASS_WDT,
	.of_match = ls1c_wdt_ids,
	.probe = ls1c_wdt_probe,
	.priv_auto = sizeof(struct ls1c_wdt),
	.ops = &ls1c_wdt_ops,
};
