// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2018
 * Lukasz Majewski, DENX Software Engineering, lukma@denx.de
 */

#include <init.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/crm_regs.h>
#include <asm/arch/clock.h>
#include <asm/arch/iomux-mx53.h>
#include <asm/arch/clock.h>
#include <asm/gpio.h>
#include <env.h>
#include <power/pmic.h>
#include <fsl_pmic.h>
#include <bootstage.h>
#include "kp_id_rev.h"

#define BOOSTER_OFF IMX_GPIO_NR(2, 23)
#define LCD_BACKLIGHT IMX_GPIO_NR(1, 1)
#define KEY1 IMX_GPIO_NR(2, 26)
#define LED_RED IMX_GPIO_NR(3, 28)

DECLARE_GLOBAL_DATA_PTR;

int dram_init(void)
{
	u32 size;

	size = get_ram_size((void *)PHYS_SDRAM_1, PHYS_SDRAM_1_SIZE);
	gd->ram_size = size;

	return 0;
}

int dram_init_banksize(void)
{
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;

	return 0;
}

static int power_init(void)
{
	struct udevice *dev;
	int ret;

	ret = pmic_get("mc34708@8", &dev);
	if (ret) {
		printf("%s: mc34708 not found !\n", __func__);
		return ret;
	}

	/* Set VDDGP to 1.110V for 800 MHz on SW1 */
	pmic_clrsetbits(dev, REG_SW_0, SWx_VOLT_MASK_MC34708,
			SWx_1_110V_MC34708);

	/* Set VCC as 1.30V on SW2 */
	pmic_clrsetbits(dev, REG_SW_1, SWx_VOLT_MASK_MC34708,
			SWx_1_300V_MC34708);

	/* Set global reset timer to 4s */
	pmic_clrsetbits(dev, REG_POWER_CTL2, TIMER_MASK_MC34708,
			TIMER_4S_MC34708);

	return ret;
}

static void setup_clocks(void)
{
	int ret;
	u32 ref_clk = MXC_HCLK;
	/*
	 * CPU clock set to 800MHz and DDR to 400MHz
	 */
	ret = mxc_set_clock(ref_clk, 800, MXC_ARM_CLK);
	if (ret)
		printf("CPU:   Switch CPU clock to 800MHZ failed\n");

	ret = mxc_set_clock(ref_clk, 400, MXC_PERIPH_CLK);
	ret |= mxc_set_clock(ref_clk, 400, MXC_DDR_CLK);
	if (ret)
		printf("CPU:   Switch DDR clock to 400MHz failed\n");
}

static void setup_ups(void)
{
	gpio_request(BOOSTER_OFF, "BOOSTER_OFF");
	gpio_direction_output(BOOSTER_OFF, 0);
}

int board_early_init_f(void)
{
	return 0;
}

/*
 * Do not overwrite the console
 * Use always serial for U-Boot console
 */
int overwrite_console(void)
{
	return 1;
}

int board_init(void)
{
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x100;

	return 0;
}

void board_disable_display(void)
{
	gpio_request(LCD_BACKLIGHT, "LCD_BACKLIGHT");
	gpio_direction_output(LCD_BACKLIGHT, 0);
}

void board_misc_setup(void)
{
	gpio_request(KEY1, "KEY1_GPIO");
	gpio_direction_input(KEY1);

	if (gpio_get_value(KEY1))
		env_set("key1", "off");
	else
		env_set("key1", "on");
}

int board_late_init(void)
{
	int ret = 0;

	board_disable_display();
	setup_ups();

	if (!power_init())
		setup_clocks();

	ret = read_eeprom();
	if (ret)
		printf("Error %d reading EEPROM content!\n", ret);

	show_eeprom();
	read_board_id();

	board_misc_setup();

	return ret;
}

#if CONFIG_IS_ENABLED(BOOTSTAGE)
#define GPIO_DR 0x0
#define GPIO_GDIR 0x4
#define GPIO_ALT1 0x1
#define GPIO5_BASE 0x53FDC000
#define IOMUXC_EIM_WAIT 0x53FA81E4
/* Green LED: GPIO5_0 */
#define GPIO_GREEN BIT(0)

void show_boot_progress(int status)
{
	/*
	 * This BOOTSTAGE_ID is called at very early stage of execution. DM gpio
	 * is not yet initialized.
	 */
	if (status == BOOTSTAGE_ID_START_UBOOT_F) {
		/*
		 * After ROM execution the EIM_WAIT PAD is set as ALT0
		 * (according to RM it shall be ALT1 after reset). To use it as
		 * GPIO we need to set it to ALT1.
		 */
		setbits_le32(((uint32_t *)(IOMUXC_EIM_WAIT)), GPIO_ALT1);

		/* Configure green LED GPIO pin direction */
		setbits_le32(((uint32_t *)(GPIO5_BASE + GPIO_GDIR)),
			     GPIO_GREEN);
		/* Turn on green LED */
		setbits_le32(((uint32_t *)(GPIO5_BASE + GPIO_DR)), GPIO_GREEN);
	}

	/*
	 * This BOOTSTAGE_ID is called just before handling execution to kernel
	 * - i.e. gpio subsystem is already initialized
	 */
	if (status == BOOTSTAGE_ID_BOOTM_HANDOFF) {
		/*
		 * Off green LED - the same approach - i.e. non dm gpio
		 * (*bits_le32) is used as in the very early stage.
		 */
		clrbits_le32(((uint32_t *)(GPIO5_BASE + GPIO_DR)),
			     GPIO_GREEN);

		/*
		 * On red LED
		 */
		gpio_request(LED_RED, "LED_RED_ERROR");
		gpio_direction_output(LED_RED, 1);
	}
}
#endif
