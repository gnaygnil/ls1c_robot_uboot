/*
 * Copyright (C) 2013 Tang Haifeng <tanghaifeng-gz@loongson.cn>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

/*
 * Driver for Loongson1 GPIO controller
 */

#include <common.h>
#include <asm/ls1x.h>
#include <asm/io.h>
#include <errno.h>

int gpio_set_value(unsigned gpio, int value)
{
	int offset;
	u32 reg_out, val, mask;

	switch (gpio/32) {
	case 0:
		offset = gpio;
		reg_out = LS1X_GPIO_OUT0;
		break;
	case 1:
		offset = gpio - 32;
		reg_out = LS1X_GPIO_OUT1;
		break;
#if defined(LS1ASOC) || defined(LS1CSOC)
	case 2:
		offset = gpio - 64;
		reg_out = LS1X_GPIO_OUT2;
		break;
#endif
#if defined(LS1CSOC)
	case 3:
		offset = gpio - 96;
		reg_out = LS1X_GPIO_OUT3;
		break;
#endif
	default:
		return 0;
	}

	mask = 1 << offset;
	val = __raw_readl(reg_out);
	if (value)
		val |= mask;
	else
		val &= ~mask;
	__raw_writel(val, reg_out);

	return 0;
}

int gpio_get_value(unsigned gpio)
{
	int offset;
	u32 reg_in;

	switch (gpio/32) {
	case 0:
		offset = gpio;
		reg_in = LS1X_GPIO_IN0;
		break;
	case 1:
		offset = gpio - 32;
		reg_in = LS1X_GPIO_IN1;
		break;
#if defined(LS1ASOC) || defined(LS1CSOC)
	case 2:
		offset = gpio - 64;
		reg_in = LS1X_GPIO_IN2;
		break;
#endif
#if defined(LS1CSOC)
	case 3:
		offset = gpio - 96;
		reg_in = LS1X_GPIO_IN3;
		break;
#endif
	default:
		return -1;
	}

	return (__raw_readl(reg_in) >> offset) & 1;
}

int gpio_request(unsigned gpio, const char *label)
{
	if (gpio >= LS1X_GPIO_COUNT)
		return -EINVAL;

	return 0;
}

int gpio_free(unsigned gpio)
{
	int offset;
	u32 reg_cfg, temp;

	switch (gpio/32) {
	case 0:
		offset = gpio;
		reg_cfg = LS1X_GPIO_CFG0;
		break;
	case 1:
		offset = gpio - 32;
		reg_cfg = LS1X_GPIO_CFG1;
		break;
#if defined(LS1ASOC) || defined(LS1CSOC)
	case 2:
		offset = gpio - 64;
		reg_cfg = LS1X_GPIO_CFG2;
		break;
#endif
#if defined(LS1CSOC)
	case 3:
		offset = gpio - 96;
		reg_cfg = LS1X_GPIO_CFG3;
		break;
#endif
	default:
		return 0;
	}

	temp = __raw_readl(reg_cfg);
	temp &= ~(1 << offset);
	__raw_writel(temp, reg_cfg);

	return 0;
}

int gpio_direction_input(unsigned gpio)
{
	int offset;
	u32 reg_cfg, reg_oe;
	u32 temp, mask;

	switch (gpio/32) {
	case 0:
		offset = gpio;
		reg_cfg = LS1X_GPIO_CFG0;
		reg_oe = LS1X_GPIO_OE0;
		break;
	case 1:
		offset = gpio - 32;
		reg_cfg = LS1X_GPIO_CFG1;
		reg_oe = LS1X_GPIO_OE1;
		break;
#if defined(LS1ASOC) || defined(LS1CSOC)
	case 2:
		offset = gpio - 64;
		reg_cfg = LS1X_GPIO_CFG2;
		reg_oe = LS1X_GPIO_OE2;
		break;
#endif
#if defined(LS1CSOC)
	case 3:
		offset = gpio - 96;
		reg_cfg = LS1X_GPIO_CFG3;
		reg_oe = LS1X_GPIO_OE3;
		break;
#endif
	default:
		return 0;
	}

	mask = 1 << offset;
	temp = __raw_readl(reg_cfg);
	temp |= mask;
	__raw_writel(temp, reg_cfg);
	temp = __raw_readl(reg_oe);
	temp |= mask;
	__raw_writel(temp, reg_oe);

	return 0;
}

int gpio_direction_output(unsigned gpio, int value)
{
	int offset;
	u32 reg_cfg, reg_oe;
	u32 temp, mask;

	gpio_set_value(gpio, value);

	switch (gpio/32) {
	case 0:
		offset = gpio;
		reg_cfg = LS1X_GPIO_CFG0;
		reg_oe = LS1X_GPIO_OE0;
		break;
	case 1:
		offset = gpio - 32;
		reg_cfg = LS1X_GPIO_CFG1;
		reg_oe = LS1X_GPIO_OE1;
		break;
#if defined(LS1ASOC) || defined(LS1CSOC)
	case 2:
		offset = gpio - 64;
		reg_cfg = LS1X_GPIO_CFG2;
		reg_oe = LS1X_GPIO_OE2;
		break;
#endif
#if defined(LS1CSOC)
	case 3:
		offset = gpio - 96;
		reg_cfg = LS1X_GPIO_CFG3;
		reg_oe = LS1X_GPIO_OE3;
		break;
#endif
	default:
		return 0;
	}

	mask = 1 << offset;
	temp = __raw_readl(reg_cfg);
	temp |= mask;
	__raw_writel(temp, reg_cfg);
	temp = __raw_readl(reg_oe);
	temp &= (~mask);
	__raw_writel(temp, reg_oe);

	return 0;
}

