/*
 * Copyright (c) 2014 TangHaifeng <tanghaifeng-gz@loongson.cn>
 *
 * Loongson1 Clock Register Definitions.
 *
 */

#ifndef __ASM_MACH_LOONGSON1_REGS_CLK_H
#define __ASM_MACH_LOONGSON1_REGS_CLK_H

//#define LS1X_CLK_REG(x)		(LS1X_CLK_BASE + (x))
//#define LS1X_CLK_PLL_FREQ		LS1X_CLK_REG(0x0)
//#define LS1X_CLK_PLL_DIV		LS1X_CLK_REG(0x4)

/* Clock PLL Divisor Register Bits */
#if defined(CONFIG_CPU_LOONGSON1C)
#define DIV_DC_EN			(0x1 << 31)
#define DIV_DC				(0x7f << 24)
#define DIV_CAM_EN			(0x1 << 23)
#define DIV_CAM			(0x7f << 16)
#define DIV_CPU_EN			(0x1 << 15)
#define DIV_CPU				(0x7f << 8)
#define DIV_DC_SEL_EN			(0x1 << 5)
#define DIV_DC_SEL				(0x1 << 4)
#define DIV_CAM_SEL_EN			(0x1 << 3)
#define DIV_CAM_SEL				(0x1 << 2)
#define DIV_CPU_SEL_EN			(0x1 << 1)
#define DIV_CPU_SEL				(0x1 << 0)

#define DIV_DC_SHIFT			24
#define DIV_CAM_SHIFT			16
#define DIV_CPU_SHIFT			8

#else
#define DIV_DC_EN			(0x1 << 31)
#define DIV_DC				(0x1f << 26)
#define DIV_CPU_EN			(0x1 << 25)
#define DIV_CPU				(0x1f << 20)
#define DIV_DDR_EN			(0x1 << 19)
#define DIV_DDR				(0x1f << 14)

#define DIV_DC_SHIFT			26
#define DIV_CPU_SHIFT			20
#define DIV_DDR_SHIFT			14
#endif

#endif /* __ASM_MACH_LOONGSON1_REGS_CLK_H */
