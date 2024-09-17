/* SPDX-License-Identifier: (GPL-2.0-only OR BSD-2-Clause) */

/*
 * Copyright (C) 2024, Hoang Trung Hieu <hoangtrunghieu.gch17214@gmail.com>
 */

#ifndef CLOCK_K230_CLK_H
#define CLOCK_K230_CLK_H

// Internal oscillators
#define K230_TIMERX_PULSE_IN	0
#define K230_SYSCTL_PCLK	1

// PLL clocks
#define K230_PLL0_CLK		2
#define K230_PLL1_CLK		3
#define K230_PLL2_CLK		4
#define K230_PLL3_CLK		5

// Fixed-factor children of PLL clocks
#define K230_PLL0_DIV2_CLK      6
#define K230_PLL0_DIV3_CLK	7
#define K230_PLL0_DIV4_CLK	8
#define K230_PLL0_DIV16_CLK	9
#define K230_PLL1_DIV2_CLK	10
#define K230_PLL1_DIV3_CLK	11
#define K230_PLL1_DIV4_CLK	12
#define K230_PLL2_DIV2_CLK	13
#define K230_PLL2_DIV3_CLK	14
#define K230_PLL2_DIV4_CLK	15
#define K230_PLL3_DIV2_CLK	16
#define K230_PLL3_DIV3_CLK	17
#define K230_PLL3_DIV4_CLK	18

// Composite clocks
#define K230_SD_SRC_CCLK	19
#define K230_SD0_GATE_CCLK	20
#define K230_SSI0_CCLK		21
#define K230_SSI1_CCLK          22
#define K230_SSI2_CCLK          23
#define K230_I2C0_CCLK          24
#define K230_I2C1_CCLK          25
#define K230_I2C2_CCLK          26
#define K230_I2C3_CCLK          27
#define K230_I2C4_CCLK          28
#define K230_WDT0_CCLK          29
#define K230_SHRM_AXIM_CCLK     30
#define K230_PDMA_APB_CCLK      31
#define K230_ADC_CLK            32
#define K230_AUDIO_DEV_CLK      33
#define K230_CODEC_ADC_MCLK     34
#define K230_CODEC_DAC_MCLK     35
#define K230_LOWSYS_APB_CCLK    36
#define K230_PWM_CCLK           37

#define K230_CPU0_SRC		38
#define K230_CPU0_PLIC		39
#define K230_CPU0_ACLK		40
#define K230_CPU0_DDRCP4	41
#define K230_CPU0_PCLK		42
#define K230_PMU_PCLK		43
#define K230_HS_HCLK_HIGH_SRC   44
#define K230_HS_HCLK_HIGH	45
#define K230_HS_HCLK_SRC   	46
#define K230_SD0_HCLK_GATE   	47
#define K230_SD1_HCLK_GATE   	48
#define K230_USB0_HCLK_GATE   	49
#define K230_USB1_HCLK_GATE   	50
#define K230_SSI1_HCLK_GATE   	51
#define K230_SSI2_HCLK_GATE   	52
#define K230_QSPI_ACLK_SRC	53
#define K230_SSI1_ACLK_GATE   	54
#define K230_SSI2_ACLK_GATE   	55
#define K230_SD_ACLK   	        56
#define K230_SD0_ACLK_GATE   	57
#define K230_SD1_ACLK_GATE   	58
#define K230_SD0_BCLK_GATE   	59
#define K230_SD1_BCLK_GATE   	60

#define K230_USB_REF_50M_CLK    61
#define K230_USB0_REF_CLK       62
#define K230_USB1_REF_CLK       63
#define K230_SD_TMCLK_SRC       64
#define K230_SD0_TMCLK_GATE     65
#define K230_SD1_TMCLK_GATE     66

#define K230_UART0_PCLK_GATE    67
#define K230_UART1_PCLK_GATE    68
#define K230_UART2_PCLK_GATE    69
#define K230_UART3_PCLK_GATE    70
#define K230_UART4_PCLK_GATE    71
#define K230_I2C0_PCLK_GATE     72
#define K230_I2C1_PCLK_GATE     73
#define K230_I2C2_PCLK_GATE     74
#define K230_I2C3_PCLK_GATE     75
#define K230_I2C4_PCLK_GATE     76
#define K230_GPIO_PCLK_GATE     77
#define K230_JAMLINK0_PCLK_GATE 78
#define K230_JAMLINK1_PCLK_GATE 79
#define K230_JAMLINK2_PCLK_GATE 80
#define K230_JAMLINK3_PCLK_GATE 81
#define K230_AUDIO_PCLK_GATE    82
#define K230_ADC_PCLK_GATE      83
#define K230_CODEC_PCLK_GATE    84

#define K230_UART0_CLK          85
#define K230_UART1_CLK          86
#define K230_UART2_CLK          87
#define K230_UART3_CLK          88
#define K230_UART4_CLK          89
#define K230_JAMLINKCO_DIV      90
#define K230_JAMLINK0CO_GATE    91
#define K230_JAMLINK1CO_GATE    92
#define K230_JAMLINK2CO_GATE    93
#define K230_JAMLINK3CO_GATE    94

#define K230_PDM_CLK            95

#define K230_GPIO_DBCLK         96

#define K230_WDT0_PCLK_GATE     97
#define K230_WDT1_PCLK_GATE     98
#define K230_TIMER_PCLK_GATE    99
#define K230_IOMUX_PCLK_GATE    100
#define K230_MAILBOX_PCLK_GATE  101

#define K230_HDI_CLK            102
#define K230_STC_CLK            103
#define K230_TS_CLK             104
#define K230_WDT1_CCLK          105

#define K230_TIMER0_CLK_SRC	106
#define K230_TIMER0_CLK		107
#define K230_TIMER1_CLK_SRC	108
#define K230_TIMER1_CLK		109
#define K230_TIMER2_CLK_SRC	110
#define K230_TIMER2_CLK		111
#define K230_TIMER3_CLK_SRC	112
#define K230_TIMER3_CLK		113
#define K230_TIMER4_CLK_SRC	114
#define K230_TIMER4_CLK		115
#define K230_TIMER5_CLK_SRC	116
#define K230_TIMER5_CLK		117

#define K230_SHRM_SRC		118

#define K230_SHRM_PCLK		119
#define K230_GSDMA_ACLK_GATE	120
#define K230_NONAI2D_ACLK_GATE	121

#define K230_DISP_HCLK		122
#define K230_DISP_ACLK_GATE	123
#define K230_DISP_CLK_EXT	124
#define K230_DISP_GPU		125
#define K230_DPIPCLK		126
#define K230_DISP_CFGCLK	127
#define K230_DISP_REFCLK_GATE	128

#define K230_DDRC_CORE_CLK	129
#define K230_DDRC_BYPASS_GATE	130
#define K230_DDRC_PCLK		131

#define K230_VPU_SRC		132
#define K230_VPU_ACLK_SRC	133
#define K230_VPU_ACLK		134
#define K230_VPU_DDRCP2		135
#define K230_VPU_CFG		136

#define K230_SEC_PCLK		137
#define K230_SEC_FIXCLK		138
#define K230_SEC_ACLK_GATE	139

#define K230_USB_CLK480		140
#define K230_USB_CLK100		141

#define K230_DPHY_TEST_CLK	142
#define K230_SPI2AXI_ACLK	143

#define K230_SHRM_DIV2		144
#define K230_SHRM_AXIS_CLK_GATE 145
#define K230_DECOMPRESS_CLK_GATE 146

#endif //CLOCK_K230_CLK_H
