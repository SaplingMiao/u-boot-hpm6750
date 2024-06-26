// SPDX-License-Identifier: GPL-2.0+
/*
 * TI OMAP timer driver
 *
 * Copyright (C) 2015, Texas Instruments, Incorporated
 */

#include <dm.h>
#include <errno.h>
#include <timer.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/omap_common.h>
#include <linux/bitops.h>

/* Timer register bits */
#define TCLR_START			BIT(0)	/* Start=1 */
#define TCLR_AUTO_RELOAD		BIT(1)	/* Auto reload */
#define TCLR_PRE_EN			BIT(5)	/* Pre-scaler enable */
#define TCLR_PTV_SHIFT			(2)	/* Pre-scaler shift value */

struct omap_gptimer_regs {
	unsigned int tidr;		/* offset 0x00 */
	unsigned char res1[12];
	unsigned int tiocp_cfg;		/* offset 0x10 */
	unsigned char res2[12];
	unsigned int tier;		/* offset 0x20 */
	unsigned int tistatr;		/* offset 0x24 */
	unsigned int tistat;		/* offset 0x28 */
	unsigned int tisr;		/* offset 0x2c */
	unsigned int tcicr;		/* offset 0x30 */
	unsigned int twer;		/* offset 0x34 */
	unsigned int tclr;		/* offset 0x38 */
	unsigned int tcrr;		/* offset 0x3c */
	unsigned int tldr;		/* offset 0x40 */
	unsigned int ttgr;		/* offset 0x44 */
	unsigned int twpc;		/* offset 0x48 */
	unsigned int tmar;		/* offset 0x4c */
	unsigned int tcar1;		/* offset 0x50 */
	unsigned int tscir;		/* offset 0x54 */
	unsigned int tcar2;		/* offset 0x58 */
};

/* Omap Timer Priv */
struct omap_timer_priv {
	struct omap_gptimer_regs *regs;
};

static u64 omap_timer_get_count(struct udevice *dev)
{
	struct omap_timer_priv *priv = dev_get_priv(dev);

	return timer_conv_64(readl(&priv->regs->tcrr));
}

static int omap_timer_probe(struct udevice *dev)
{
	struct timer_dev_priv *uc_priv = dev_get_uclass_priv(dev);
	struct omap_timer_priv *priv = dev_get_priv(dev);

	if (!uc_priv->clock_rate)
		uc_priv->clock_rate = V_SCLK;

	uc_priv->clock_rate /= (2 << SYS_PTV);

	/* start the counter ticking up, reload value on overflow */
	writel(0, &priv->regs->tldr);
	writel(0, &priv->regs->tcrr);
	/* enable timer */
	writel((SYS_PTV << 2) | TCLR_PRE_EN | TCLR_AUTO_RELOAD |
	       TCLR_START, &priv->regs->tclr);

	return 0;
}

static int omap_timer_of_to_plat(struct udevice *dev)
{
	struct omap_timer_priv *priv = dev_get_priv(dev);

	priv->regs = map_physmem(dev_read_addr(dev),
				 sizeof(struct omap_gptimer_regs), MAP_NOCACHE);

	return 0;
}

#if CONFIG_IS_ENABLED(BOOTSTAGE)
ulong timer_get_boot_us(void)
{
	u64 ticks = 0;
	u32 rate = 1;
	u64 us;
	int ret;

	ret = dm_timer_init();
	if (!ret) {
		/* The timer is available */
		rate = timer_get_rate(gd->timer);
		timer_get_count(gd->timer, &ticks);
	} else {
		return 0;
	}

	us = (ticks * 1000) / rate;
	return us;
}
#endif

static const struct timer_ops omap_timer_ops = {
	.get_count = omap_timer_get_count,
};

static const struct udevice_id omap_timer_ids[] = {
	{ .compatible = "ti,am335x-timer" },
	{ .compatible = "ti,am4372-timer" },
	{ .compatible = "ti,omap5430-timer" },
	{ .compatible = "ti,am654-timer" },
	{}
};

U_BOOT_DRIVER(omap_timer) = {
	.name	= "omap_timer",
	.id	= UCLASS_TIMER,
	.of_match = omap_timer_ids,
	.of_to_plat = omap_timer_of_to_plat,
	.priv_auto	= sizeof(struct omap_timer_priv),
	.probe = omap_timer_probe,
	.ops	= &omap_timer_ops,
};
