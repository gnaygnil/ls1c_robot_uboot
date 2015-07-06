/*
 * Loongson1 RTC Driver
 *
 * Copyright (C) 2014 Tang Haifeng <tanghaifeng-gz@loongson.cn>
 * on behalf of DENX Software Engineering GmbH
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <rtc.h>
#include <asm/io.h>

#include <asm/ls1x.h>

#define LS1X_RTC_REG_OFFSET	(LS1X_RTC_BASE + 0x20)
#define LS1X_RTC_REGS(x)	(LS1X_RTC_REG_OFFSET + (x))

/*RTC programmable counters 0 and 1*/
#define SYS_COUNTER_CNTRL		(LS1X_RTC_REGS(0x20))
#define SYS_CNTRL_ERS			(1 << 23)
#define SYS_CNTRL_RTS			(1 << 20)
#define SYS_CNTRL_RM2			(1 << 19)
#define SYS_CNTRL_RM1			(1 << 18)
#define SYS_CNTRL_RM0			(1 << 17)
#define SYS_CNTRL_RS			(1 << 16)
#define SYS_CNTRL_BP			(1 << 14)
#define SYS_CNTRL_REN			(1 << 13)
#define SYS_CNTRL_BRT			(1 << 12)
#define SYS_CNTRL_TEN			(1 << 11)
#define SYS_CNTRL_BTT			(1 << 10)
#define SYS_CNTRL_E0			(1 << 8)
#define SYS_CNTRL_ETS			(1 << 7)
#define SYS_CNTRL_32S			(1 << 5)
#define SYS_CNTRL_TTS			(1 << 4)
#define SYS_CNTRL_TM2			(1 << 3)
#define SYS_CNTRL_TM1			(1 << 2)
#define SYS_CNTRL_TM0			(1 << 1)
#define SYS_CNTRL_TS			(1 << 0)

/* Programmable Counter 0 Registers */
#define SYS_TOYTRIM		(LS1X_RTC_REGS(0))
#define SYS_TOYWRITE0		(LS1X_RTC_REGS(4))
#define SYS_TOYWRITE1		(LS1X_RTC_REGS(8))
#define SYS_TOYREAD0		(LS1X_RTC_REGS(0xC))
#define SYS_TOYREAD1		(LS1X_RTC_REGS(0x10))
#define SYS_TOYMATCH0		(LS1X_RTC_REGS(0x14))
#define SYS_TOYMATCH1		(LS1X_RTC_REGS(0x18))
#define SYS_TOYMATCH2		(LS1X_RTC_REGS(0x1C))

/* Programmable Counter 1 Registers */
#define SYS_RTCTRIM		(LS1X_RTC_REGS(0x40))
#define SYS_RTCWRITE0		(LS1X_RTC_REGS(0x44))
#define SYS_RTCREAD0		(LS1X_RTC_REGS(0x48))
#define SYS_RTCMATCH0		(LS1X_RTC_REGS(0x4C))
#define SYS_RTCMATCH1		(LS1X_RTC_REGS(0x50))
#define SYS_RTCMATCH2		(LS1X_RTC_REGS(0x54))

#define RTC_CNTR_OK (SYS_CNTRL_E0 | SYS_CNTRL_32S)

int rtc_get(struct rtc_time *time)
{
	uint32_t secs;

	secs = readl(SYS_RTCREAD0);
	to_tm(secs, time);

	return 0;
}

int rtc_set(struct rtc_time *time)
{
	uint32_t secs;
	uint32_t c;
	int ret = -1;

	secs = mktime(time->tm_year, time->tm_mon, time->tm_mday,
		time->tm_hour, time->tm_min, time->tm_sec);
	secs += 4;	/* 需要补上4秒？ */

	writel(secs, SYS_RTCWRITE0);
	c = 0x10000;
	/* add timeout check counter, for more safe */
	while ((readl(SYS_COUNTER_CNTRL) & SYS_CNTRL_RS) && --c)
		udelay(1000);

	if (!c) {
		debug("set time timeout!\n");
		goto err;
	}

	return 0;
err:
	return ret;
}

void rtc_reset(void)
{
	unsigned long v;
	int ret;

	writel(0x2100, SYS_COUNTER_CNTRL);
	writel(0x0, SYS_RTCMATCH0);
	while (readl(SYS_COUNTER_CNTRL) & SYS_CNTRL_RM0)
		udelay(1);
	writel(0x0, SYS_RTCMATCH1);
	while (readl(SYS_COUNTER_CNTRL) & SYS_CNTRL_RM1)
		udelay(1);
	writel(0x0, SYS_RTCMATCH2);
	while (readl(SYS_COUNTER_CNTRL) & SYS_CNTRL_RM2)
		udelay(1);

	v = readl(SYS_COUNTER_CNTRL);
	if (!(v & RTC_CNTR_OK)) {
		debug("rtc counters not working\n");
		ret = -1;
		goto err;
	}
	ret = -1;
	/* set to 1 HZ if needed */
	if (readl(SYS_RTCTRIM) != 32767) {
		v = 0x100000;
		while ((readl(SYS_COUNTER_CNTRL) & SYS_CNTRL_RTS) && --v)
			udelay(1);

		if (!v) {
			debug("time out\n");
			goto err;
		}
		writel(32767, SYS_RTCTRIM);
	}
	/* this loop coundn't be endless */
	while (readl(SYS_COUNTER_CNTRL) & SYS_CNTRL_RTS)
		udelay(1);

	return;
err:
	return;
}
