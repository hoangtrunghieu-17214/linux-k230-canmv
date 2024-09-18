// SPDX-License-Identifier: GPL-2.0

/*
 * Canaan CanMV_K230 clock driver.
 * Based on k230_sdk's implementation and DT parameters.
 *
 * Copyright (c) 2023, Canaan Bright Sight Co., Ltd
 * Copyright (C) 2024, Hoang Trung Hieu <hoangtrunghieu.gch17214@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define pr_fmt(fmt) "%s [%s]: " fmt, KBUILD_MODNAME, __func__

#include <linux/clk-provider.h>
#include <linux/clk.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <linux/io.h>

#include <linux/module.h>
#include <linux/kernel.h>

#include "clk-k230.h"

#include <dt-bindings/clock/canaan,k230-clk.h>

static struct clk_hw_onecell_data *all_clks;
static DEFINE_SPINLOCK(k230_cclk_lock);
static DEFINE_SPINLOCK(k230_pll_lock);

static const char *k230_ssi0_parents[] = { "pll0_div2", "pll2_div4" };
static const char *k230_usb_ref_clk_parents[] = { "osc24m", "usb_ref_50m" };

static const char *k230_timer0_clk_parents[] = { "timer0_clk_src",
						 "timerx_pulse_in" };
static const char *k230_timer1_clk_parents[] = { "timer1_clk_src",
						 "timerx_pulse_in" };
static const char *k230_timer2_clk_parents[] = { "timer2_clk_src",
						 "timerx_pulse_in" };
static const char *k230_timer3_clk_parents[] = { "timer3_clk_src",
						 "timerx_pulse_in" };
static const char *k230_timer4_clk_parents[] = { "timer4_clk_src",
						 "timerx_pulse_in" };
static const char *k230_timer5_clk_parents[] = { "timer5_clk_src",
						 "timerx_pulse_in" };

static const char *k230_shrm_clk_parents[] = { "pll0_div2", "pll3_div2" };

static const char *k230_ddrc_core_clk_parents[] = { "pll0_div2", "pll0_div3",
						    "pll2_div4" };

static const struct k230_internal_osc_cfg k230_osc_cfgs[] __initdata = {
	K230_INTERNAL_OSC(K230_TIMERX_PULSE_IN, "timerx_pulse_in", 50000000),
	K230_INTERNAL_OSC(K230_SYSCTL_PCLK, "sysctl_pclk", 100000000)
};

static const struct k230_pll_clk_cfg k230_pll_cfgs[] __initdata = {
	K230_PLL(K230_PLL0_CLK, "pll0", 0),
	K230_PLL(K230_PLL1_CLK, "pll1", 0x10),
	K230_PLL(K230_PLL2_CLK, "pll2", 0x20),
	K230_PLL(K230_PLL3_CLK, "pll3", 0x30)
};

static const struct k230_fixedfactor_clk_cfg k230_pll_child_cfgs[] __initdata = {
	K230_FIXEDFACTOR(K230_PLL0_DIV2_CLK, "pll0_div2", "pll0", 2),
	K230_FIXEDFACTOR(K230_PLL0_DIV3_CLK, "pll0_div3", "pll0", 3),
	K230_FIXEDFACTOR(K230_PLL0_DIV4_CLK, "pll0_div4", "pll0", 4),
	K230_FIXEDFACTOR(K230_PLL0_DIV16_CLK, "pll0_div16", "pll0", 16),
	K230_FIXEDFACTOR(K230_PLL1_DIV2_CLK, "pll1_div2", "pll1", 2),
	K230_FIXEDFACTOR(K230_PLL1_DIV3_CLK, "pll1_div3", "pll1", 3),
	K230_FIXEDFACTOR(K230_PLL1_DIV4_CLK, "pll1_div4", "pll1", 4),
	K230_FIXEDFACTOR(K230_PLL2_DIV2_CLK, "pll2_div2", "pll2", 2),
	K230_FIXEDFACTOR(K230_PLL2_DIV3_CLK, "pll2_div3", "pll2", 3),
	K230_FIXEDFACTOR(K230_PLL2_DIV4_CLK, "pll2_div4", "pll2", 4),
	K230_FIXEDFACTOR(K230_PLL3_DIV2_CLK, "pll3_div2", "pll3", 2),
	K230_FIXEDFACTOR(K230_PLL3_DIV3_CLK, "pll3_div3", "pll3", 3),
	K230_FIXEDFACTOR(K230_PLL3_DIV4_CLK, "pll3_div4", "pll3", 4),
	K230_FIXEDFACTOR(K230_SHRM_DIV2, "shrm_div2", "shrm_src", 2)
};

static const struct k230_composite_clk_cfg k230_cclk_cfgs[] __initdata = {
	K230_GDIV(K230_SD_SRC_CCLK, "sd_src_cclk", "pll0_div4", 0x18, 11, 0,
		  0x1C, 1, 1, 0, 0, 2, 8, 12, 0x7, 31),
	K230_GATE(K230_SD0_GATE_CCLK, "sd0_gate_cclk", "sd_src_cclk", 0x18, 15,
		  0),
	K230_GMUX(K230_SSI0_CCLK, "ssi0_cclk", k230_ssi0_parents, 0x18, 24, 0,
		  0x20, 18, 0x1),
	K230_GDIV(K230_SSI1_CCLK, "ssi1_cclk", "pll0_div4", 0x18, 25, 0, 0x20,
		  1, 1, 0, 0, 1, 8, 3, 0x7, 31),
	K230_GDIV(K230_SSI2_CCLK, "ssi2_cclk", "pll0_div4", 0x18, 26, 0, 0x20,
		  1, 1, 0, 0, 1, 8, 6, 0x7, 31),
	K230_GDIV(K230_I2C0_CCLK, "i2c0_cclk", "pll0_div4", 0x24, 21, 0, 0x2C,
		  1, 1, 0, 0, 1, 8, 15, 0x7, 31),
	K230_GDIV(K230_I2C1_CCLK, "i2c1_cclk", "pll0_div4", 0x24, 22, 0, 0x2C,
		  1, 1, 0, 0, 1, 8, 18, 0x7, 31),
	K230_GDIV(K230_I2C2_CCLK, "i2c2_cclk", "pll0_div4", 0x24, 23, 0, 0x2C,
		  1, 1, 0, 0, 1, 8, 21, 0x7, 31),
	K230_GDIV(K230_I2C3_CCLK, "i2c3_cclk", "pll0_div4", 0x24, 24, 0, 0x2C,
		  1, 1, 0, 0, 1, 8, 24, 0x7, 31),
	K230_GDIV(K230_I2C4_CCLK, "i2c4_cclk", "pll0_div4", 0x24, 25, 0, 0x2C,
		  1, 1, 0, 0, 1, 8, 27, 0x7, 31),
	K230_GDIV(K230_WDT0_CCLK, "wdt0_cclk", "osc24m", 0x50, 5, 0, 0x58, 1, 1,
		  0, 0, 1, 64, 3, 0x3F, 31),
	K230_GDIV(K230_LOWSYS_APB_CCLK, "ls_pclk_src", "pll0_div4", 0x24, 0, 0,
		  0x30, 1, 1, 0, 0, 1, 8, 0, 0x7, 31),
	K230_GATE(K230_PWM_CCLK, "pwm_cclk", "ls_pclk_src", 0x24, 12, 0),
	K230_GATE(K230_SHRM_AXIM_CCLK, "shrm_axim_clk_gate", "pll0_div4", 0x5C,
		  12, 0),
	K230_GATE(K230_PDMA_APB_CCLK, "pdma_aclk_gate", "shrm_axim_clk_gate",
		  0x5C, 3, 0),
	K230_GDIV(K230_ADC_CLK, "adc_clk", "pll0_div4", 0x24, 26, 0, 0x30, 1, 1,
		  0, 0, 1, 1024, 3, 0x3FF, 31),
	K230_GDIV(K230_AUDIO_DEV_CLK, "audio_dev_clk", "pll0_div4", 0x24, 28, 0,
		  0x34, 4, 0x1B9, 16, 0x7FFF, 0xC35, 0xF424, 0, 0xFFFF, 31),
	K230_GDIV(K230_CODEC_ADC_MCLK, "codec_adc_mclk", "pll0_div4", 0x24, 29,
		  0, 0x38, 10, 0x1B9, 14, 0x1FFF, 0xC35, 0x3D09, 0, 0x3FFF, 31),
	K230_GDIV(K230_CODEC_DAC_MCLK, "codec_dac_mclk", "pll0_div4", 0x24, 30,
		  0, 0x3C, 10, 0x1B9, 14, 0x1FFF, 0xC35, 0x3D09, 0, 0x3FFF, 31),

	K230_GDIV(K230_CPU0_SRC, "cpu0_src", "pll0_div2", 0x0, 0, 0, 0x0, 1, 16,
		  0, 0, 16, 16, 1, 0xF, 31),
	K230_GDIV(K230_CPU0_PLIC, "cpu0_plic", "cpu0_src", 0x0, 9, 0, 0x0, 1, 1,
		  0, 0, 1, 8, 10, 0x7, 31),
	K230_DIV(K230_CPU0_ACLK, "cpu0_aclk", "cpu0_src", 0x0, 1, 1, 0, 0, 1, 8,
		 6, 0x7, 31),
	K230_GATE(K230_CPU0_DDRCP4, "cpu0_ddrcp4", "cpu0_src", 0x60, 7, 0),
	K230_GDIV(K230_CPU0_PCLK, "cpu0_pclk", "pll0_div4", 0x0, 13, 0, 0x0, 1,
		  1, 0, 0, 1, 8, 15, 0x7, 31),
	K230_GATE(K230_PMU_PCLK, "pmu_pclk", "osc24m", 0x10, 0, 0),

	K230_DIV(K230_HS_HCLK_HIGH_SRC, "hs_hclk_high_src", "pll0_div4", 0x1C,
		 1, 1, 0, 0, 1, 8, 0, 0x7, 31),
	K230_GATE(K230_HS_HCLK_HIGH, "hs_hclk_high", "hs_hclk_high_src", 0x18,
		  1, 0),
	K230_GDIV(K230_HS_HCLK_SRC, "hs_hclk_src", "hs_hclk_high_src", 0x18, 0,
		  0, 0x1C, 1, 1, 0, 0, 1, 8, 3, 0x7, 31),
	K230_GATE(K230_SD0_HCLK_GATE, "sd0_hclk_gate", "hs_hclk_src", 0x18, 2,
		  0),
	K230_GATE(K230_SD1_HCLK_GATE, "sd1_hclk_gate", "hs_hclk_src", 0x18, 3,
		  0),
	K230_GATE(K230_USB0_HCLK_GATE, "usb0_hclk_gate", "hs_hclk_src", 0x18, 4,
		  0),
	K230_GATE(K230_USB1_HCLK_GATE, "usb1_hclk_gate", "hs_hclk_src", 0x18, 5,
		  0),
	K230_GATE(K230_SSI1_HCLK_GATE, "ssi1_hclk_gate", "hs_hclk_src", 0x18, 7,
		  0),
	K230_GATE(K230_SSI2_HCLK_GATE, "ssi2_hclk_gate", "hs_hclk_src", 0x18, 8,
		  0),
	K230_GDIV(K230_QSPI_ACLK_SRC, "qspi_aclk_src", "pll0_div4", 0x18, 28, 0,
		  0x20, 1, 1, 0, 0, 1, 8, 12, 0x7, 31),
	K230_GATE(K230_SSI1_ACLK_GATE, "ssi1_aclk_gate", "qspi_aclk_src", 0x18,
		  29, 0),
	K230_GATE(K230_SSI2_ACLK_GATE, "ssi2_aclk_gate", "qspi_aclk_src", 0x18,
		  30, 0),
	K230_GDIV(K230_SD_ACLK, "sd_aclk", "pll2_div4", 0x18, 9, 0, 0x1C, 1, 1,
		  0, 0, 1, 8, 6, 0x7, 31),
	K230_GATE(K230_SD0_ACLK_GATE, "sd0_aclk_gate", "sd_aclk", 0x18, 13, 0),
	K230_GATE(K230_SD1_ACLK_GATE, "sd1_aclk_gate", "sd_aclk", 0x18, 17, 0),
	K230_GATE(K230_SD0_BCLK_GATE, "sd0_bclk_gate", "sd_aclk", 0x18, 14, 0),
	K230_GATE(K230_SD1_BCLK_GATE, "sd1_bclk_gate", "sd_aclk", 0x18, 18, 0),

	K230_DIV(K230_USB_REF_50M_CLK, "usb_ref_50m", "pll0_div16", 0x20, 1, 1,
		 0, 0, 1, 8, 15, 0x7, 31),
	K230_GMUX(K230_USB0_REF_CLK, "usb0_ref_clk", k230_usb_ref_clk_parents,
		  0x18, 21, 0, 0x18, 23, 0x1),
	K230_GMUX(K230_USB1_REF_CLK, "usb1_ref_clk", k230_usb_ref_clk_parents,
		  0x18, 22, 0, 0x18, 23, 0x1),
	K230_GDIV(K230_SD_TMCLK_SRC, "sd_tmclk_src", "osc24m", 0x18, 12, 0,
		  0x1C, 1, 1, 0, 0, 24, 32, 15, 0x1F, 31),
	K230_GATE(K230_SD0_TMCLK_GATE, "sd0_tmclk_gate", "sd_tmclk_src", 0x18,
		  16, 0),
	K230_GATE(K230_SD1_TMCLK_GATE, "sd1_tmclk_gate", "sd_tmclk_src", 0x18,
		  20, 0),

	K230_GATE(K230_UART0_PCLK_GATE, "uart0_pclk_gate", "ls_pclk_src", 0x24,
		  1, 0),
	K230_GATE(K230_UART1_PCLK_GATE, "uart1_pclk_gate", "ls_pclk_src", 0x24,
		  2, 0),
	K230_GATE(K230_UART2_PCLK_GATE, "uart2_pclk_gate", "ls_pclk_src", 0x24,
		  3, 0),
	K230_GATE(K230_UART3_PCLK_GATE, "uart3_pclk_gate", "ls_pclk_src", 0x24,
		  4, 0),
	K230_GATE(K230_UART4_PCLK_GATE, "uart4_pclk_gate", "ls_pclk_src", 0x24,
		  5, 0),
	K230_GATE(K230_I2C0_PCLK_GATE, "i2c0_pclk_gate", "ls_pclk_src", 0x24, 6,
		  0),
	K230_GATE(K230_I2C1_PCLK_GATE, "i2c1_pclk_gate", "ls_pclk_src", 0x24, 7,
		  0),
	K230_GATE(K230_I2C2_PCLK_GATE, "i2c2_pclk_gate", "ls_pclk_src", 0x24, 8,
		  0),
	K230_GATE(K230_I2C3_PCLK_GATE, "i2c3_pclk_gate", "ls_pclk_src", 0x24, 9,
		  0),
	K230_GATE(K230_I2C4_PCLK_GATE, "i2c4_pclk_gate", "ls_pclk_src", 0x24,
		  10, 0),
	K230_GATE(K230_GPIO_PCLK_GATE, "gpio_pclk_gate", "ls_pclk_src", 0x24,
		  11, 0),
	K230_GATE(K230_JAMLINK0_PCLK_GATE, "jamlink0_pclk_gate", "ls_pclk_src",
		  0x28, 4, 0),
	K230_GATE(K230_JAMLINK1_PCLK_GATE, "jamlink1_pclk_gate", "ls_pclk_src",
		  0x28, 5, 0),
	K230_GATE(K230_JAMLINK2_PCLK_GATE, "jamlink2_pclk_gate", "ls_pclk_src",
		  0x28, 6, 0),
	K230_GATE(K230_JAMLINK3_PCLK_GATE, "jamlink3_pclk_gate", "ls_pclk_src",
		  0x28, 7, 0),
	K230_GATE(K230_AUDIO_PCLK_GATE, "audio_pclk_gate", "ls_pclk_src", 0x24,
		  13, 0),
	K230_GATE(K230_ADC_PCLK_GATE, "adc_pclk_gate", "ls_pclk_src", 0x24, 15,
		  0),
	K230_GATE(K230_CODEC_PCLK_GATE, "codec_pclk_gate", "ls_pclk_src", 0x24,
		  14, 0),

	K230_GDIV(K230_UART0_CLK, "uart0_clk", "pll0_div16", 0x24, 16, 0, 0x2C,
		  1, 1, 0, 0, 1, 8, 0, 0x7, 31),
	K230_GDIV(K230_UART1_CLK, "uart1_clk", "pll0_div16", 0x24, 17, 0, 0x2C,
		  1, 1, 0, 0, 1, 8, 3, 0x7, 31),
	K230_GDIV(K230_UART2_CLK, "uart2_clk", "pll0_div16", 0x24, 18, 0, 0x2C,
		  1, 1, 0, 0, 1, 8, 6, 0x7, 31),
	K230_GDIV(K230_UART3_CLK, "uart3_clk", "pll0_div16", 0x24, 19, 0, 0x2C,
		  1, 1, 0, 0, 1, 8, 9, 0x7, 31),
	K230_GDIV(K230_UART4_CLK, "uart4_clk", "pll0_div16", 0x24, 20, 0, 0x2C,
		  1, 1, 0, 0, 1, 8, 12, 0x7, 31),

	K230_DIV(K230_JAMLINKCO_DIV, "jamlinkCO_div", "pll0_div16", 0x30, 1, 1,
		 0, 0, 2, 512, 23, 0xFF, 31),
	K230_GATE(K230_JAMLINK0CO_GATE, "jamlink0CO_gate", "jamlinkCO_div",
		  0x28, 0, 0),
	K230_GATE(K230_JAMLINK1CO_GATE, "jamlink1CO_gate", "jamlinkCO_div",
		  0x28, 1, 0),
	K230_GATE(K230_JAMLINK2CO_GATE, "jamlink2CO_gate", "jamlinkCO_div",
		  0x28, 2, 0),
	K230_GATE(K230_JAMLINK3CO_GATE, "jamlink3CO_gate", "jamlinkCO_div",
		  0x28, 3, 0),

	// Special PDM clock that stores mul and div in 2 different registers
	K230_GDIV1(K230_PDM_CLK, "pdm_clk", "pll0_div4", 0x24, 31, 0, 0x40,
		   0x44, 0x2, 0x1B9, 0, 0xFFFF, 0xC35, 0x1E848, 0, 0x1FFFF, 31),

	K230_GDIV(K230_GPIO_DBCLK, "gpio_dbclk", "osc24m", 0x24, 27, 0, 0x30, 1,
		  1, 0, 0, 1, 1024, 13, 0x3FF, 31),

	K230_GATE(K230_WDT0_PCLK_GATE, "wdt0_pclk_gate", "sysctl_pclk", 0x50, 1,
		  0),
	K230_GATE(K230_WDT1_PCLK_GATE, "wdt1_pclk_gate", "sysctl_pclk", 0x50, 2,
		  0),
	K230_GATE(K230_TIMER_PCLK_GATE, "timer_pclk_gate", "sysctl_pclk", 0x50,
		  3, 0),
	K230_GATE(K230_IOMUX_PCLK_GATE, "iomux_pclk_gate", "sysctl_pclk", 0x50,
		  20, 0),
	K230_GATE(K230_MAILBOX_PCLK_GATE, "mailbox_pclk_gate", "sysctl_pclk",
		  0x50, 4, 0),

	K230_GDIV(K230_HDI_CLK, "hdi_clk", "pll0_div4", 0x50, 21, 0, 0x58, 1, 1,
		  0, 0, 1, 8, 28, 0x7, 31),
	K230_GDIV(K230_STC_CLK, "stc_clk", "pll1_div4", 0x50, 19, 0, 0x58, 1, 1,
		  0, 0, 1, 32, 15, 0x1F, 31),
	K230_DIV(K230_TS_CLK, "ts_clk", "osc24m", 0x58, 1, 1, 0, 0, 1, 256, 20,
		 0xFF, 31),
	K230_GDIV(K230_WDT1_CCLK, "wdt1_cclk", "osc24m", 0x50, 6, 0, 0x58, 1, 1,
		  0, 0, 1, 64, 9, 0x3F, 31),

	K230_DIV(K230_TIMER0_CLK_SRC, "timer0_clk_src", "pll0_div16", 0x54, 1,
		 1, 0, 0, 1, 8, 0, 0x7, 31),
	K230_GMUX(K230_TIMER0_CLK, "timer0_clk", k230_timer0_clk_parents, 0x50,
		  13, 0, 0x50, 7, 0x1),
	K230_DIV(K230_TIMER1_CLK_SRC, "timer1_clk_src", "pll0_div16", 0x54, 1,
		 1, 0, 0, 1, 8, 3, 0x7, 31),
	K230_GMUX(K230_TIMER1_CLK, "timer1_clk", k230_timer1_clk_parents, 0x50,
		  14, 0, 0x50, 8, 0x1),
	K230_DIV(K230_TIMER2_CLK_SRC, "timer2_clk_src", "pll0_div16", 0x54, 1,
		 1, 0, 0, 1, 8, 6, 0x7, 31),
	K230_GMUX(K230_TIMER2_CLK, "timer2_clk", k230_timer2_clk_parents, 0x50,
		  15, 0, 0x50, 9, 0x1),
	K230_DIV(K230_TIMER3_CLK_SRC, "timer3_clk_src", "pll0_div16", 0x54, 1,
		 1, 0, 0, 1, 8, 9, 0x7, 31),
	K230_GMUX(K230_TIMER3_CLK, "timer3_clk", k230_timer3_clk_parents, 0x50,
		  16, 0, 0x50, 10, 0x1),
	K230_DIV(K230_TIMER4_CLK_SRC, "timer4_clk_src", "pll0_div16", 0x54, 1,
		 1, 0, 0, 1, 8, 12, 0x7, 31),
	K230_GMUX(K230_TIMER4_CLK, "timer4_clk", k230_timer4_clk_parents, 0x50,
		  17, 0, 0x50, 11, 0x1),
	K230_DIV(K230_TIMER5_CLK_SRC, "timer5_clk_src", "pll0_div16", 0x54, 1,
		 1, 0, 0, 1, 8, 15, 0x7, 31),
	K230_GMUX(K230_TIMER5_CLK, "timer5_clk", k230_timer5_clk_parents, 0x50,
		  18, 0, 0x50, 12, 0x1),

	K230_GMUX(K230_SHRM_SRC, "shrm_src", k230_shrm_clk_parents, 0x5C, 10, 0,
		  0x5C, 14, 0x1),

	K230_GDIV(K230_SHRM_PCLK, "shrm_pclk", "pll0_div4", 0x5C, 0, 0, 0x5C, 1,
		  1, 0, 0, 1, 8, 18, 0x7, 31),
	K230_GATE(K230_GSDMA_ACLK_GATE, "gsdma_aclk_gate", "shrm_axim_clk_gate",
		  0x5C, 5, 0),
	K230_GATE(K230_NONAI2D_ACLK_GATE, "nonai2d_aclk_gate",
		  "shrm_axim_clk_gate", 0x5C, 9, 0),

	K230_GDIV(K230_DISP_HCLK, "disp_hclk", "pll0_div4", 0x74, 0, 0, 0x78, 1,
		  1, 0, 0, 1, 8, 0, 0x7, 31),
	K230_GATE(K230_DISP_ACLK_GATE, "disp_aclk_gate", "pll0_div4", 0x74, 1,
		  0),
	K230_GDIV(K230_DISP_CLK_EXT, "disp_clk_ext", "pll0_div3", 0x74, 5, 0,
		  0x78, 1, 1, 0, 0, 1, 16, 16, 0xF, 31),
	K230_GDIV(K230_DISP_GPU, "disp_gpu", "pll0_div3", 0x74, 6, 0, 0x78, 1,
		  1, 0, 0, 1, 16, 20, 0xF, 31),
	K230_GDIV(K230_DPIPCLK, "dpipclk", "pll1_div4", 0x74, 2, 0, 0x78, 1, 1,
		  0, 0, 1, 256, 3, 0xFF, 31),
	K230_GDIV(K230_DISP_CFGCLK, "disp_cfgclk", "pll1_div4", 0x74, 4, 0,
		  0x78, 1, 1, 0, 0, 1, 32, 11, 0x1F, 31),
	K230_GATE(K230_DISP_REFCLK_GATE, "disp_refclk_gate", "osc24m", 0x74, 3,
		  0),

	K230_GMD(K230_DDRC_CORE_CLK, "ddrc_core_clk",
		 k230_ddrc_core_clk_parents, 0x60, 2, 0, 0x60, 0, 0x3, 0x60, 1,
		 1, 0, 0, 1, 16, 10, 0xF, 31),
	K230_GATE(K230_DDRC_BYPASS_GATE, "ddrc_bypass_gate", "pll2_div4", 0x60,
		  8, 0),
	K230_GDIV(K230_DDRC_PCLK, "ddrc_pclk", "pll0_div4", 0x60, 9, 0, 0x60, 1,
		  1, 0, 0, 1, 16, 14, 0xF, 31),

	K230_GDIV(K230_VPU_SRC, "vpu_src", "pll0_div2", 0xC, 0, 0, 0xC, 1, 16,
		  0, 0, 16, 16, 1, 0xF, 31),
	K230_DIV(K230_VPU_ACLK_SRC, "vpu_aclk_src", "vpu_src", 0xC, 1, 1, 0, 0,
		 1, 16, 6, 0xF, 31),
	K230_GATE(K230_VPU_ACLK, "vpu_aclk", "vpu_aclk_src", 0xC, 5, 0),
	K230_GATE(K230_VPU_DDRCP2, "vpu_ddrcp2", "vpu_aclk_src", 0x60, 5, 0),
	K230_GDIV(K230_VPU_CFG, "vpu_cfg", "pll0_div4", 0xC, 10, 0, 0xC, 1, 1,
		  0, 0, 1, 16, 11, 0xF, 31),

	K230_GDIV(K230_SEC_PCLK, "sec_pclk", "pll0_div4", 0x80, 0, 0, 0x80, 1,
		  1, 0, 0, 1, 8, 1, 0x7, 31),
	K230_GDIV(K230_SEC_FIXCLK, "sec_fixclk", "pll1_div4", 0x80, 5, 0, 0x80,
		  1, 1, 0, 0, 1, 32, 6, 0x1F, 31),
	K230_GDIV(K230_SEC_ACLK_GATE, "sec_aclk_gate", "pll1_div4", 0x80, 4, 0,
		  0x80, 1, 1, 0, 0, 1, 8, 11, 0x3, 31),

	K230_GDIV(K230_USB_CLK480, "usb_clk480", "pll1", 0x100, 0, 0, 0x100, 1,
		  1, 0, 0, 1, 8, 1, 0x7, 31),
	K230_GDIV(K230_USB_CLK100, "usb_clk100", "pll0_div4", 0x100, 0, 0,
		  0x100, 1, 1, 0, 0, 1, 8, 4, 0x7, 31),

	K230_GDIV(K230_DPHY_TEST_CLK, "dphy_test_clk", "pll0", 0x104, 0, 0,
		  0x104, 1, 1, 0, 0, 1, 16, 1, 0xF, 31),
	K230_GDIV(K230_SPI2AXI_ACLK, "spi2axi_aclk", "pll0_div4", 0x108, 0, 0,
		  0x108, 1, 1, 0, 0, 1, 8, 1, 0x7, 31),

	K230_GATE(K230_SHRM_AXIS_CLK_GATE, "shrm_axis_clk_gate", "shrm_div2",
		  0x5C, 11, 0),
	K230_GATE(K230_DECOMPRESS_CLK_GATE, "decompress_clk_gate", "shrm_src",
		  0x5C, 7, 0),
};

static const struct k230_fracdiv_table codec_fracdiv_table[] = {
	{ .rate = 2048000, .mul = 16, .div = 3125 },
	{ .rate = 3072000, .mul = 24, .div = 3125 },
	{ .rate = 4096000, .mul = 32, .div = 3125 },
	{ .rate = 6144000, .mul = 48, .div = 3125 },
	{ .rate = 8192000, .mul = 64, .div = 3125 },
	{ .rate = 11289600, .mul = 441, .div = 15625 },
	{ .rate = 12288000, .mul = 96, .div = 3125 },
	{ .rate = 24576000, .mul = 192, .div = 3125 },
	{ .rate = 49152000, .mul = 384, .div = 3125 },
	{ /* sentinel */ }
};

static const struct k230_fracdiv_table pdm_fracdiv_table[] = {
	{ .rate = 128000, .mul = 1, .div = 3125 },
	{ .rate = 192000, .mul = 3, .div = 6250 },
	{ .rate = 256000, .mul = 2, .div = 3125 },
	{ .rate = 384000, .mul = 3, .div = 3125 },
	{ .rate = 512000, .mul = 4, .div = 3125 },
	{ .rate = 768000, .mul = 6, .div = 3125 },
	{ .rate = 1024000, .mul = 8, .div = 3125 },
	{ .rate = 1441200, .mul = 441, .div = 125000 },
	{ .rate = 1536000, .mul = 12, .div = 3125 },
	{ .rate = 2048000, .mul = 16, .div = 3125 },
	{ .rate = 2822400, .mul = 441, .div = 62500 },
	{ .rate = 3072000, .mul = 24, .div = 3125 },
	{ .rate = 4096000, .mul = 32, .div = 3125 },
	{ .rate = 5644800, .mul = 441, .div = 31250 },
	{ .rate = 6144000, .mul = 48, .div = 3125 },
	{ .rate = 8192000, .mul = 64, .div = 3125 },
	{ .rate = 11289600, .mul = 441, .div = 15625 },
	{ .rate = 12288000, .mul = 96, .div = 3125 },
	{ .rate = 24576000, .mul = 192, .div = 3125 },
	{ .rate = 49152000, .mul = 384, .div = 3125 },
	{ /* sentinel */ }
};

static int k230_pll_clk_is_enabled(struct clk_hw *hw)
{
	struct k230_pll_clk *k230_pll = to_k230_pll_clk(hw);
	u32 gate_value;
	unsigned long lock_flags;

	spin_lock_irqsave(k230_pll->pll_lock, lock_flags);
	gate_value = readl(k230_pll->reg + K230_PLL_CLK_GATE_OFFSET);
	spin_unlock_irqrestore(k230_pll->pll_lock, lock_flags);

	pr_debug("PLL clock %s gate value: %d", clk_hw_get_name(hw),
		 gate_value);

	return (gate_value & BIT(K230_PLL_GATE_REG_ENABLE_BIT)) > 0;
}

static int k230_pll_clk_enable(struct clk_hw *hw)
{
	struct k230_pll_clk *k230_pll = to_k230_pll_clk(hw);
	u32 gate_value;
	unsigned long lock_flags;

	spin_lock_irqsave(k230_pll->pll_lock, lock_flags);
	gate_value = readl(k230_pll->reg + K230_PLL_CLK_GATE_OFFSET);

	gate_value |= BIT(K230_PLL_GATE_REG_ENABLE_BIT) |
		      BIT(K230_PLL_GATE_REG_WRITE_ENABLE_BIT);
	writel(gate_value, k230_pll->reg + K230_PLL_CLK_GATE_OFFSET);

	spin_unlock_irqrestore(k230_pll->pll_lock, lock_flags);
	return 0;
}

static void k230_pll_clk_disable(struct clk_hw *hw)
{
	struct k230_pll_clk *k230_pll = to_k230_pll_clk(hw);
	u32 gate_value;
	unsigned long lock_flags;

	spin_lock_irqsave(k230_pll->pll_lock, lock_flags);
	gate_value = readl(k230_pll->reg + K230_PLL_CLK_GATE_OFFSET);

	gate_value &= ~BIT(K230_PLL_GATE_REG_ENABLE_BIT);
	gate_value |= BIT(K230_PLL_GATE_REG_WRITE_ENABLE_BIT);
	writel(gate_value, k230_pll->reg + K230_PLL_CLK_GATE_OFFSET);

	spin_unlock_irqrestore(k230_pll->pll_lock, lock_flags);
}

static unsigned long k230_pll_clk_recalc_rate(struct clk_hw *hw,
					      unsigned long parent_rate)
{
	struct k230_pll_clk *k230_pll = to_k230_pll_clk(hw);
	u32 pll_div_value, pll_div_bypass_value, pll_div_lock_value;
	u32 fb_div, ref_div, out_div;
	unsigned long rate, flags;

	spin_lock_irqsave(k230_pll->pll_lock, flags);

	pll_div_bypass_value =
		readl(k230_pll->reg + K230_PLL_CLK_DIVIDE_BYPASS_OFFSET);
	if (pll_div_bypass_value & BIT(K230_PLL_DIVIDE_BYPASS_REG_ENABLE_BIT)) {
		spin_unlock_irqrestore(k230_pll->pll_lock, flags);
		return parent_rate;
	}

	pll_div_lock_value = readl(k230_pll->reg + K230_PLL_CLK_LOCK_OFFSET);
	if (pll_div_lock_value & BIT(K230_PLL_LOCK_REG_STATUS_BIT)) {
		pll_div_value =
			readl(k230_pll->reg + K230_PLL_CLK_DIVIDE_OFFSET);
		fb_div = (pll_div_value >> K230_PLL_DIVIDE_FB_SHIFT &
			  K230_PLL_DIVIDE_FB_MASK) +
			 1;
		ref_div = (pll_div_value >> K230_PLL_DIVIDE_REF_SHIFT &
			   K230_PLL_DIVIDE_REF_MASK) +
			  1;
		out_div = (pll_div_value >> K230_PLL_DIVIDE_OUT_SHIFT &
			   K230_PLL_DIVIDE_OUT_MASK) +
			  1;

		rate = mult_frac(parent_rate, fb_div, ref_div) / out_div;

		spin_unlock_irqrestore(k230_pll->pll_lock, flags);
		return rate;
	}

	spin_unlock_irqrestore(k230_pll->pll_lock, flags);
	pr_err("The clock %s is unlocked - must be locked before use\n",
	       clk_hw_get_name(hw));
	return 0;
}

static int k230_pll_clk_init(struct clk_hw *hw)
{
	return clk_prepare_enable(hw->clk);
}

static const struct clk_ops k230_pll_clk_ops = {
	.init = k230_pll_clk_init,
	.is_enabled = k230_pll_clk_is_enabled,
	.enable = k230_pll_clk_enable,
	.disable = k230_pll_clk_disable,
	.recalc_rate = k230_pll_clk_recalc_rate,
};

static int k230_approximate_clock_rate(struct k230_clk_fracdiv *clk,
				       unsigned long rate,
				       unsigned long parent_rate, u32 *mul,
				       u32 *div)
{
	long abs_min, abs_current;
	long perfect_divide;
	u32 draft_mul, draft_div;
	const struct k230_fracdiv_table *tbl;

	if (clk->n_min == clk->n_max) {
		// Only change the numerator
		perfect_divide = (long)((parent_rate * 1000) / rate);
		abs_min =
			abs(perfect_divide - (long)(((long)clk->n_max * 1000) /
						    (long)clk->m_min));

		for (u32 i = clk->m_min + 1; i <= clk->m_max; ++i) {
			abs_current =
				abs(perfect_divide -
				    (long)((long)((long)clk->n_max * 1000) /
					   (long)i));
			if (abs_min > abs_current) {
				abs_min = abs_current;
				*mul = i;
			}
		}
		*div = clk->n_min;

		draft_div = clk->n_min;
		draft_mul = mult_frac(rate, clk->n_min, parent_rate);

		pr_debug(
			"Rates: %ld/%ld, Result: prev_frac=%d/%d, new_frac=%d/%d\n",
			rate, parent_rate, *mul, *div, draft_mul, draft_div);
	} else if (clk->m_min == clk->m_max) {
		// Constant multiplier - Only change the denominator
		perfect_divide = (long)((parent_rate * 1000) / rate);
		abs_min =
			abs(perfect_divide - (long)(((long)clk->n_min * 1000) /
						    (long)clk->m_max));
		*div = clk->n_min;

		for (u32 i = clk->n_min + 1; i <= clk->n_max; i++) {
			abs_current = abs(perfect_divide -
					  (long)((long)((long)i * 1000) /
						 (long)clk->m_max));
			if (abs_min > abs_current) {
				abs_min = abs_current;
				*div = i;
			}
		}
		*mul = clk->m_min;

		draft_mul = clk->m_min;
		draft_div = mult_frac(parent_rate, clk->m_min, rate);

		pr_debug(
			"Rates: %ld/%ld, Result: prev_frac=%d/%d, new_frac=%d/%d\n",
			rate, parent_rate, *mul, *div, draft_mul, draft_div);
	} else {
		if (unlikely(!clk->fracdiv_table)) {
			pr_err("Both numerator and denominator are variable, but the fracdiv_table was not provided!\n");
			return -EINVAL;
		}

		for (tbl = clk->fracdiv_table; tbl->div; ++tbl) {
			if (rate == tbl->rate) {
				*mul = tbl->mul;
				*div = tbl->div;
				break;
			}
		}
	}

	return 0;
}

static int k230_rate_clk_set_rate(struct clk_hw *hw, unsigned long rate,
				  unsigned long parent_rate)
{
	struct k230_clk_fracdiv *fracdiv_clk = to_k230_clk_fracdiv(hw);

	u32 mul, div, reg, reg_1;
	unsigned long flags;

	if (unlikely(!fracdiv_clk->reg))
		return -EINVAL;

	if (unlikely((rate > parent_rate) || (rate == 0) || (parent_rate == 0)))
		return -EINVAL;

	if (unlikely(k230_approximate_clock_rate(fracdiv_clk, rate, parent_rate,
						 &mul, &div))) {
		pr_err("Cannot approximate clock rate - maybe a precondition was not satisfied?\n");
		return -EINVAL;
	}

	if (fracdiv_clk->reg_1 == NULL) {
		spin_lock_irqsave(fracdiv_clk->lock, flags);

		reg = readl(fracdiv_clk->reg);

		reg &= ~(fracdiv_clk->n_mask << fracdiv_clk->n_shift);
		if (fracdiv_clk->n_min != fracdiv_clk->n_max &&
		    fracdiv_clk->m_min != fracdiv_clk->m_max)
			reg &= ~(fracdiv_clk->m_mask << fracdiv_clk->m_shift);

		reg |= BIT(fracdiv_clk->bit_idx);

		if (fracdiv_clk->n_min == fracdiv_clk->n_max) {
			// Only change the numerator
			reg |= ((mul - 1) & fracdiv_clk->n_mask)
			       << (fracdiv_clk->n_shift);
		} else if (fracdiv_clk->m_min == fracdiv_clk->m_max) {
			// Only change the denominator
			reg |= ((div - 1) & fracdiv_clk->n_mask)
			       << (fracdiv_clk->n_shift);
		} else {
			reg |= (mul & fracdiv_clk->m_mask)
			       << (fracdiv_clk->m_shift);
			reg |= (div & fracdiv_clk->n_mask)
			       << (fracdiv_clk->n_shift);
		}

		writel(reg, fracdiv_clk->reg);
		spin_unlock_irqrestore(fracdiv_clk->lock, flags);
	} else {
		spin_lock_irqsave(fracdiv_clk->lock, flags);
		reg = readl(fracdiv_clk->reg);
		reg_1 = readl(fracdiv_clk->reg_1);

		reg &= ~(fracdiv_clk->n_mask << fracdiv_clk->n_shift);
		reg_1 &= ~(fracdiv_clk->m_mask << fracdiv_clk->m_shift);

		reg_1 |= BIT(fracdiv_clk->bit_idx);
		reg_1 |= ((mul & fracdiv_clk->m_mask) << fracdiv_clk->m_shift);

		reg |= ((div & fracdiv_clk->n_mask) << fracdiv_clk->n_shift);

		writel(reg, fracdiv_clk->reg);
		writel(reg_1, fracdiv_clk->reg_1);
		spin_unlock_irqrestore(fracdiv_clk->lock, flags);
	}

	return 0;
}

static long k230_rate_clk_round_rate(struct clk_hw *hw, unsigned long rate,
				     unsigned long *parent_rate)
{
	return min(rate, LONG_MAX);
}

static unsigned long k230_rate_clk_recalc_rate(struct clk_hw *hw,
					       unsigned long parent_rate)
{
	struct k230_clk_fracdiv *fracdiv_clk = to_k230_clk_fracdiv(hw);

	if (unlikely(!fracdiv_clk->reg)) {
		return parent_rate;
	}

	u32 reg_val, reg_1_val, rate_mul, rate_div;
	unsigned long flags;

	if (fracdiv_clk->n_min == fracdiv_clk->n_max) {
		// Only change the numerator
		spin_lock_irqsave(fracdiv_clk->lock, flags);
		reg_val = readl(fracdiv_clk->reg);
		spin_unlock_irqrestore(fracdiv_clk->lock, flags);

		reg_val = (reg_val >> fracdiv_clk->n_shift) &
			  fracdiv_clk->n_mask;
		rate_mul = reg_val + 1;
		rate_div = fracdiv_clk->n_max;
	} else if (fracdiv_clk->m_min == fracdiv_clk->m_max) {
		// Only change the denominator
		spin_lock_irqsave(fracdiv_clk->lock, flags);
		reg_val = readl(fracdiv_clk->reg);
		spin_unlock_irqrestore(fracdiv_clk->lock, flags);

		reg_val = (reg_val >> fracdiv_clk->n_shift) &
			  fracdiv_clk->n_mask;
		rate_mul = fracdiv_clk->m_max;
		rate_div = reg_val + 1;
	} else {
		if (fracdiv_clk->reg_1 == NULL) {
			spin_lock_irqsave(fracdiv_clk->lock, flags);
			reg_val = readl(fracdiv_clk->reg);
			spin_unlock_irqrestore(fracdiv_clk->lock, flags);

			rate_mul = ((reg_val >> fracdiv_clk->m_shift) &
				    fracdiv_clk->m_mask);
			rate_div = ((reg_val >> fracdiv_clk->n_shift) &
				    fracdiv_clk->n_mask);
		} else {
			spin_lock_irqsave(fracdiv_clk->lock, flags);
			reg_val = readl(fracdiv_clk->reg);
			reg_1_val = readl(fracdiv_clk->reg_1);
			spin_unlock_irqrestore(fracdiv_clk->lock, flags);

			rate_div = ((reg_val >> fracdiv_clk->n_shift) &
				    fracdiv_clk->n_mask);
			rate_mul = ((reg_1_val >> fracdiv_clk->m_shift) &
				    fracdiv_clk->m_mask);
		}
	}

	return mult_frac(parent_rate, rate_mul, rate_div);
}

static const struct clk_ops k230_rate_clk_ops = {
	.set_rate = k230_rate_clk_set_rate,
	.round_rate = k230_rate_clk_round_rate,
	.recalc_rate = k230_rate_clk_recalc_rate
};

static int k230_gate_clk_enable(struct clk_hw *hw)
{
	struct clk_gate *gate = to_clk_gate(hw);
	u32 gate_val;
	unsigned long flags;

	spin_lock_irqsave(gate->lock, flags);
	gate_val = readl(gate->reg);
	if ((gate->flags & CLK_GATE_SET_TO_DISABLE) == 0) {
		gate_val |= BIT(gate->bit_idx);
	} else {
		gate_val &= ~BIT(gate->bit_idx);
	}

	writel(gate_val, gate->reg);
	spin_unlock_irqrestore(gate->lock, flags);

	return 0;
}

static void k230_gate_clk_disable(struct clk_hw *hw)
{
	struct clk_gate *gate = to_clk_gate(hw);
	u32 gate_val;
	unsigned long flags;

	spin_lock_irqsave(gate->lock, flags);
	gate_val = readl(gate->reg);
	if (gate->flags & CLK_GATE_SET_TO_DISABLE) {
		gate_val |= BIT(gate->bit_idx);
	} else {
		gate_val &= ~BIT(gate->bit_idx);
	}
	writel(gate_val, gate->reg);
	spin_unlock_irqrestore(gate->lock, flags);
}

static int k230_gate_clk_init(struct clk_hw *hw)
{
	if (clk_gate_is_enabled(hw))
		return clk_prepare_enable(hw->clk);

	return 0;
}

static const struct clk_ops k230_gate_clk_ops = {
	.is_enabled = clk_gate_is_enabled,
	.enable = k230_gate_clk_enable,
	.disable = k230_gate_clk_disable,
	.init = k230_gate_clk_init
};

static u8 k230_mux_clk_get_parent(struct clk_hw *hw)
{
	struct clk_mux *mux = to_clk_mux(hw);
	u32 mux_value;
	unsigned long flags;

	spin_lock_irqsave(mux->lock, flags);
	mux_value = readl(mux->reg);
	spin_unlock_irqrestore(mux->lock, flags);

	return ((mux_value >> mux->shift) & mux->mask);
}

static int k230_mux_clk_set_parent(struct clk_hw *hw, u8 index)
{
	struct clk_mux *mux = to_clk_mux(hw);
	unsigned long flags;

	u32 mux_value = ((index & mux->mask) << mux->shift);

	spin_lock_irqsave(mux->lock, flags);
	writel(mux_value, mux->reg);
	spin_unlock_irqrestore(mux->lock, flags);

	return 0;
}

static const struct clk_ops k230_mux_clk_ops = {
	.get_parent = k230_mux_clk_get_parent,
	.set_parent = k230_mux_clk_set_parent,
	.determine_rate = clk_hw_determine_rate_no_reparent
};

static int __init k230_register_child_cclk(struct device *dev,
					   void __iomem *sys_base,
					   unsigned int cclk_idx)
{
	struct clk_hw *rate_hw = NULL, *gate_hw = NULL, *mux_hw = NULL;
	struct k230_clk_fracdiv *fracdiv = NULL;
	struct clk_gate *gate = NULL;
	struct clk_mux *mux = NULL;

	struct k230_composite_clk_cfg clk_cfg = k230_cclk_cfgs[cclk_idx];
	unsigned int onecell_idx = clk_cfg.onecell_idx;

	const char *const *parent_names = clk_cfg.parent_names;
	int num_parents = clk_cfg.num_parents;

	int err;

	if (clk_cfg.is_fracdiv) {
		fracdiv = devm_kzalloc(dev, sizeof(*fracdiv), GFP_KERNEL);
		if (!fracdiv) {
			err = -ENOMEM;
			goto out_err;
		}

		fracdiv->reg = sys_base + clk_cfg.fracdiv_reg_offset;
		fracdiv->lock = &k230_cclk_lock;

		fracdiv->m_shift = clk_cfg.fracdiv_m_shift;
		fracdiv->m_mask = clk_cfg.fracdiv_m_mask;
		fracdiv->m_min = clk_cfg.fracdiv_m_min;
		fracdiv->m_max = clk_cfg.fracdiv_m_max;
		fracdiv->n_shift = clk_cfg.fracdiv_n_shift;
		fracdiv->n_mask = clk_cfg.fracdiv_n_mask;
		fracdiv->n_min = clk_cfg.fracdiv_n_min;
		fracdiv->n_max = clk_cfg.fracdiv_n_max;
		fracdiv->bit_idx = clk_cfg.fracdiv_write_enable_bit;

		if (clk_cfg.is_fracdiv_1)
			fracdiv->reg_1 =
				sys_base + clk_cfg.fracdiv_reg_1_offset;

		// There are 4 special clocks that requires their own divider table.
		switch (clk_cfg.fracdiv_reg_offset) {
		case 0x38:
		case 0x3C:
			fracdiv->fracdiv_table = codec_fracdiv_table;
			break;
		case 0x34:
		case 0x40:
			fracdiv->fracdiv_table = pdm_fracdiv_table;
			break;
		}

		rate_hw = &fracdiv->hw;
	}

	if (clk_cfg.is_gate) {
		gate = devm_kzalloc(dev, sizeof(*gate), GFP_KERNEL);
		if (!gate) {
			err = -ENOMEM;
			goto out_err_freerate;
		}

		gate->bit_idx = clk_cfg.gate_enable_bit;
		if (clk_cfg.gate_is_inverse) {
			gate->flags |= CLK_GATE_SET_TO_DISABLE;
		}

		gate->reg = sys_base + clk_cfg.gate_reg_offset;
		gate->lock = &k230_cclk_lock;

		gate_hw = &gate->hw;
	}

	if (clk_cfg.is_mux) {
		mux = devm_kzalloc(dev, sizeof(*mux), GFP_KERNEL);
		if (!mux) {
			err = -ENOMEM;
			goto out_err_freegate;
		}

		mux->reg = sys_base + clk_cfg.mux_reg_offset;
		mux->shift = clk_cfg.mux_shift;
		mux->mask = clk_cfg.mux_mask;
		mux->lock = &k230_cclk_lock;

		mux_hw = &mux->hw;
	}

	struct clk_hw *composite_hw = clk_hw_register_composite(
		dev, clk_cfg.name, parent_names, num_parents, mux_hw,
		&k230_mux_clk_ops, rate_hw, &k230_rate_clk_ops, gate_hw,
		&k230_gate_clk_ops, 0x20);

	if (IS_ERR(composite_hw)) {
		err = (int)PTR_ERR(composite_hw);
		dev_err(dev,
			"Can't register the composite clock with name %s (id=%d, err=%d)",
			clk_cfg.name, onecell_idx, err);
		goto out_err_freemux;
	}

	all_clks->hws[onecell_idx] = composite_hw;

	return 0;

out_err_freemux:
	devm_kfree(dev, mux);
out_err_freegate:
	devm_kfree(dev, gate);
out_err_freerate:
	devm_kfree(dev, fracdiv);
out_err:
	return err;
}

static int __init k230_register_pll_clk(struct device *dev,
					void __iomem *pll_base,
					unsigned int pll_idx)
{
	struct k230_pll_clk_cfg clk_cfg = k230_pll_cfgs[pll_idx];
	unsigned int onecell_idx = clk_cfg.onecell_idx;
	const struct clk_parent_data pll_parent = { .fw_name = "osc24m" };
	int err;

	struct k230_pll_clk *pll_clk =
		devm_kzalloc(dev, sizeof(*pll_clk), GFP_KERNEL);
	if (!pll_clk) {
		dev_err(dev, "Can't allocate memory for k230_pll_clk (id=%d)\n",
			clk_cfg.onecell_idx);
		err = -ENOMEM;
		goto out_err;
	}

	struct clk_init_data pll_init = { .name = clk_cfg.name,
					  .parent_data = &pll_parent,
					  .num_parents = 1,
					  .ops = &k230_pll_clk_ops,
					  .flags = 0x20 };

	pll_clk->reg = pll_base + clk_cfg.pll_reg_offset;
	pll_clk->pll_lock = &k230_pll_lock;
	pll_clk->hw.init = &pll_init;

	err = devm_clk_hw_register(dev, &pll_clk->hw);
	if (err) {
		dev_err(dev,
			"An error has occurred while registering a PLL clock (id=%d, err=%d)\n",
			onecell_idx, err);
		goto out_free_clk;
	}

	all_clks->hws[onecell_idx] = &pll_clk->hw;

	return 0;

out_free_clk:
	devm_kfree(dev, pll_clk);
out_err:
	return err;
}

static int __init k230_register_all_internal_osc_clks(struct device *dev)
{
	for (int i = 0; i < ARRAY_SIZE(k230_osc_cfgs); ++i) {
		struct k230_internal_osc_cfg cfg = k230_osc_cfgs[i];
		struct clk_hw *hw = devm_clk_hw_register_fixed_rate(
			dev, cfg.name, NULL, 0, cfg.freq);

		if (IS_ERR(hw)) {
			int err = PTR_ERR(hw);
			dev_err(dev,
				"An error has occurred while registering internal oscillator clk %s (err=%d)\n",
				k230_osc_cfgs[i].name, err);
			return err;
		}
	}

	return 0;
}

static int __init k230_register_all_pll_clk(struct device *dev,
					    void __iomem *pll_base)
{
	for (int i = 0; i < ARRAY_SIZE(k230_pll_cfgs); ++i) {
		int err = k230_register_pll_clk(dev, pll_base, i);

		if (err) {
			dev_err(dev,
				"An error has occurred while registering pll clk %s (err=%d)\n",
				k230_pll_cfgs[i].name, err);
			return err;
		}
	}

	return 0;
}

static int __init k230_register_all_fixedfactor_clks(struct device *dev)
{
	for (int i = 0; i < ARRAY_SIZE(k230_pll_child_cfgs); ++i) {
		struct k230_fixedfactor_clk_cfg clk_cfg =
			k230_pll_child_cfgs[i];
		struct clk_hw *hw = devm_clk_hw_register_fixed_factor(
			dev, clk_cfg.name, clk_cfg.parent_name, 0, 1,
			clk_cfg.clk_div);

		if (IS_ERR(hw)) {
			int err = PTR_ERR(hw);
			dev_err(dev,
				"An error has occurred while registering fixed-factor clk %s (err=%d)\n",
				k230_pll_child_cfgs[i].name, err);
			return err;
		}

		all_clks->hws[clk_cfg.onecell_idx] = hw;
	}

	return 0;
}

static void __init k230_register_all_child_cclks(struct device *dev,
						 void __iomem *sys_base)
{
	for (int i = 0; i < ARRAY_SIZE(k230_cclk_cfgs); ++i) {
		int result = k230_register_child_cclk(dev, sys_base, i);

		if (result) {
			dev_warn(
				dev,
				"An error has occurred while registering cclk %s (err=%d)\n",
				k230_cclk_cfgs[i].name, result);
		}
	}
}

static int __init k230_clk_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	const size_t clk_count =
		ARRAY_SIZE(k230_osc_cfgs) + ARRAY_SIZE(k230_pll_cfgs) +
		ARRAY_SIZE(k230_pll_child_cfgs) + ARRAY_SIZE(k230_cclk_cfgs);
	const size_t clks_data_size = struct_size(all_clks, hws, clk_count);
	void __iomem *sys_base, *pll_base;
	int err;

	sys_base = devm_platform_ioremap_resource_byname(pdev, "sys");
	if (IS_ERR(sys_base))
		return dev_err_probe(
			dev, PTR_ERR(sys_base),
			"Can't map the sys_base address\n");

	pll_base = devm_platform_ioremap_resource_byname(pdev, "pll");
	if (IS_ERR(pll_base))
		return dev_err_probe(
			dev, PTR_ERR(pll_base),
			"Can't map the pll_base address");

	all_clks = devm_kmalloc(dev, clks_data_size, GFP_KERNEL);
	if (!all_clks) {
		return dev_err_probe(dev, -ENOMEM,
			"Cannot allocate clock onecell data - not enough memory\n");
	}

	all_clks->num = clk_count;
	for (int i = 0; i < clk_count; ++i) {
		all_clks->hws[i] = ERR_PTR(-ENOENT);
	}

	err = k230_register_all_internal_osc_clks(dev);
	if (err)
		return err;

	err = k230_register_all_pll_clk(dev, pll_base);
	if (err)
		return err;

	err = k230_register_all_fixedfactor_clks(dev);
	if (err)
		return err;

	k230_register_all_child_cclks(dev, sys_base);

	err = devm_of_clk_add_hw_provider(dev, of_clk_hw_onecell_get, all_clks);

	if (err) {
		return dev_err_probe(dev, err, "Can't register the clk hw provider (err=%d)\n",
			err);
	}

	dev_dbg(dev, "k230_clk setup completed\n");
	return 0;
}

static const struct of_device_id k230_clk_of_match[] = {
	{ .compatible = "canaan,k230-clk" },
	{}
};

static struct platform_driver k230_composite_clk_driver = {
	.driver = { .name = "k230_clk", .of_match_table = k230_clk_of_match }
};

builtin_platform_driver_probe(k230_composite_clk_driver, k230_clk_probe);

MODULE_AUTHOR("Hoang Trung Hieu <hoangtrunghieu.gch17214@gmail.com>");
MODULE_DESCRIPTION("Clock Management Unit (CMU) driver for Canaan K230 SoC");
MODULE_LICENSE("GPL");
