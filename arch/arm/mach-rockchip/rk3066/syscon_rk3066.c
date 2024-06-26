// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2015 Google, Inc
 * Written by Simon Glass <sjg@chromium.org>
 */

#include <dm.h>
#include <log.h>
#include <syscon.h>
#include <asm/arch-rockchip/clock.h>

static const struct udevice_id rk3066_syscon_ids[] = {
	{ .compatible = "rockchip,rk3066-noc", .data = ROCKCHIP_SYSCON_NOC },
	{ .compatible = "rockchip,rk3066-grf", .data = ROCKCHIP_SYSCON_GRF },
	{ .compatible = "rockchip,rk3066-pmu", .data = ROCKCHIP_SYSCON_PMU },
	{ }
};

U_BOOT_DRIVER(syscon_rk3066) = {
	.name = "rk3066_syscon",
	.id = UCLASS_SYSCON,
	.of_match = rk3066_syscon_ids,
};

#if CONFIG_IS_ENABLED(OF_PLATDATA)
static int rk3066_syscon_bind_of_plat(struct udevice *dev)
{
	dev->driver_data = dev->driver->of_match->data;
	debug("syscon: %s %d\n", dev->name, (uint)dev->driver_data);

	return 0;
}

U_BOOT_DRIVER(rockchip_rk3066_noc) = {
	.name = "rockchip_rk3066_noc",
	.id = UCLASS_SYSCON,
	.of_match = rk3066_syscon_ids,
	.bind = rk3066_syscon_bind_of_plat,
};

U_BOOT_DRIVER(rockchip_rk3066_grf) = {
	.name = "rockchip_rk3066_grf",
	.id = UCLASS_SYSCON,
	.of_match = rk3066_syscon_ids + 1,
	.bind = rk3066_syscon_bind_of_plat,
};

U_BOOT_DRIVER(rockchip_rk3066_pmu) = {
	.name = "rockchip_rk3066_pmu",
	.id = UCLASS_SYSCON,
	.of_match = rk3066_syscon_ids + 2,
	.bind = rk3066_syscon_bind_of_plat,
};
#endif
