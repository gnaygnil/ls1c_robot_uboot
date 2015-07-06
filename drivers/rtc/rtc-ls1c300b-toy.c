/*
 * Loongson1 RTC Driver
 *
 * Copyright (C) 2015 Tang Haifeng <tanghaifeng-gz@loongson.cn>
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

#define LS1X_SEC_OFFSET		(4)
#define LS1X_MIN_OFFSET		(10)
#define LS1X_HOUR_OFFSET	(16)
#define LS1X_DAY_OFFSET		(21)
#define LS1X_MONTH_OFFSET	(26)


#define LS1X_SEC_MASK		(0x3f)
#define LS1X_MIN_MASK		(0x3f)
#define LS1X_HOUR_MASK		(0x1f)
#define LS1X_DAY_MASK		(0x1f)
#define LS1X_MONTH_MASK		(0x3f)
#define LS1X_YEAR_MASK		(0xff)

#define ls1x_get_sec(t)		(((t) >> LS1X_SEC_OFFSET) & LS1X_SEC_MASK)
#define ls1x_get_min(t)		(((t) >> LS1X_MIN_OFFSET) & LS1X_MIN_MASK)
#define ls1x_get_hour(t)	(((t) >> LS1X_HOUR_OFFSET) & LS1X_HOUR_MASK)
#define ls1x_get_day(t)		(((t) >> LS1X_DAY_OFFSET) & LS1X_DAY_MASK)
#define ls1x_get_month(t)	(((t) >> LS1X_MONTH_OFFSET) & LS1X_MONTH_MASK)

#define RTC_CNTR_OK (SYS_CNTRL_E0 | SYS_CNTRL_32S)

int rtc_get(struct rtc_time *rtm)
{
	unsigned long v, t;

	v = readl(SYS_TOYREAD0);
	t = readl(SYS_TOYREAD1);

	rtm->tm_sec	= ls1x_get_sec(v);
	rtm->tm_min	= ls1x_get_min(v);
	rtm->tm_hour	= ls1x_get_hour(v);
//	rtm->tm_wday	= ls1x_get_day(v);
	rtm->tm_mday	= ls1x_get_day(v);
	rtm->tm_mon	= ls1x_get_month(v);
	rtm->tm_year	= t & LS1X_YEAR_MASK;

	if (rtm->tm_year >= 70)
		rtm->tm_year += 1900;
	else
		rtm->tm_year += 2000;

	debug("%s: tm is secs=%d, mins=%d, hours=%d, "
		"mday=%d, mon=%d, year=%d, wday=%d\n",
		__func__,
		rtm->tm_sec, rtm->tm_min, rtm->tm_hour,
		rtm->tm_mday, rtm->tm_mon + 1, rtm->tm_year, rtm->tm_wday);

	return 0;
}

int rtc_set(struct  rtc_time *rtm)
{
	unsigned long v, t;

	if (rtm->tm_year < 1970 || rtm->tm_year > 2069)
		printf("WARNING: year should be between 1970 and 2069!\n");

	v = (rtm->tm_mon  << LS1X_MONTH_OFFSET)
		| (rtm->tm_mday << LS1X_DAY_OFFSET)
		| (rtm->tm_hour << LS1X_HOUR_OFFSET)
		| (rtm->tm_min  << LS1X_MIN_OFFSET)
		| (rtm->tm_sec  << LS1X_SEC_OFFSET);
	writel(v, SYS_TOYWRITE0);

	t = rtm->tm_year % 100;
	writel(t, SYS_TOYWRITE1);

	return 0;
}

void rtc_reset(void)
{
	return;
}

