/*
 * 
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <linux/list.h>
#include <linux/fb.h>
#include <ipu_pixfmt.h>

#include <asm/gpio.h>
#include <asm/ls1x.h>
#include <asm/arch/ls1xfb.h>

#define MX53LOCO_LCD_POWER		IMX_GPIO_NR(3, 24)

static struct fb_videomode const at043tn13 = {
	.name	= "AT043TN13",
	.pixclock	= 111000,
	.refresh	= 60,
	.xres		= 480,
	.yres		= 272,
	.hsync_len	= 41,		// 523-482
	.left_margin	= 2,	// 525-523
	.right_margin	= 2,	// 482-480
	.vsync_len	= 10,		// 284-274
	.upper_margin	= 4,	// 288-284
	.lower_margin	= 2,	// 274-272
	.sync		= FB_SYNC_VERT_HIGH_ACT | FB_SYNC_HOR_HIGH_ACT,
};

struct ls1xfb_mach_info ls1x_lcd0_info = {
	.base		= LS1X_DC0_BASE,
	.id			= "Graphic lcd",
	.modes			= (struct fb_videomode *)&at043tn13,
//	.num_modes		= ARRAY_SIZE(video_modes),
	.pix_fmt		= PIX_FMT_RGB565,
	.de_mode		= 1,	/* 注意：lcd是否使用DE模式 */
	/* 根据lcd屏修改invert_pixclock和invert_pixde参数(0或1)，部分lcd可能显示不正常 */
	.invert_pixclock	= 0,
	.invert_pixde	= 0,
};

int board_video_skip(void)
{
	int ret;
	char const *e = getenv("panel");

	if (e) {
		if (strcmp(e, "seiko") == 0) {
			ret = ls1x_fb_init(&at043tn13, 0, IPU_PIX_FMT_RGB565, &ls1x_lcd0_info);
			if (ret)
				printf("Seiko cannot be configured: %d\n", ret);
			return ret;
		}
	}

	/*
	 * 'panel' env variable not found or has different value than 'seiko'
	 *  Defaulting to claa lcd.
	 */
	ret = ls1x_fb_init(&at043tn13, 0, IPU_PIX_FMT_RGB565, &ls1x_lcd0_info);
	if (ret)
		printf("CLAA cannot be configured: %d\n", ret);
	return ret;
}

/*
 * Do not overwrite the console
 * Use always serial for U-Boot console
 */
int overwrite_console(void)
{
	return 1;
}
