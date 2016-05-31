/*
 * 
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <linux/list.h>
#include <linux/fb.h>
#include <ipu_pixfmt.h>

#include <pca953x.h>

#include <asm/gpio.h>
#include <asm/ls1x.h>
#include <asm/arch/ls1xfb.h>

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

static struct fb_videomode const at070tn93 = {
	.name	= "HX8264",	// AT070TN93
	.pixclock	= 34209,
	.refresh	= 60,
	.xres		= 800,
	.yres		= 480,
	.hsync_len	= 48,		// 888-840
	.left_margin	= 40,	// 928-888
	.right_margin	= 40,	// 840-800
	.vsync_len	= 3,		// 496-493
	.upper_margin	= 29,	// 525-496
	.lower_margin	= 13,	// 493-480
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

static void backlight(int on)
{
	if (on) {
	#ifdef CONFIG_PCA953X
		pca953x_set_dir(CONFIG_SYS_I2C_PCA953X_ADDR, 
		  1 << CONFIG_BACKLIGHT_GPIO, PCA953X_DIR_OUT << CONFIG_BACKLIGHT_GPIO);
		pca953x_set_val(CONFIG_SYS_I2C_PCA953X_ADDR, 
		  1 << CONFIG_BACKLIGHT_GPIO, 1 << CONFIG_BACKLIGHT_GPIO);
	#endif
	} else {
	#ifdef CONFIG_PCA953X
		pca953x_set_dir(CONFIG_SYS_I2C_PCA953X_ADDR, 
		  1 << CONFIG_BACKLIGHT_GPIO, PCA953X_DIR_OUT << CONFIG_BACKLIGHT_GPIO);
		pca953x_set_val(CONFIG_SYS_I2C_PCA953X_ADDR, 
		  1 << CONFIG_BACKLIGHT_GPIO, 0 << CONFIG_BACKLIGHT_GPIO);
	#endif
	}
}

int board_video_skip(void)
{
	int ret;
	char const *e = getenv("panel");

	if (e) {
		if (strcmp(e, "at043tn13") == 0) {
			ls1x_lcd0_info.modes = (struct fb_videomode *)&at043tn13;
			ret = ls1x_fb_init(&at043tn13, 0, IPU_PIX_FMT_RGB565, &ls1x_lcd0_info);
		}
		else if (strcmp(e, "at070tn93") == 0) {
			ls1x_lcd0_info.modes = (struct fb_videomode *)&at070tn93;
			ret = ls1x_fb_init(&at070tn93, 0, IPU_PIX_FMT_RGB565, &ls1x_lcd0_info);
		}
		if (ret)
			printf("Panel cannot be configured: %d\n", ret);
	} else {
		/*
		 * 'panel' env variable not found or has different value than 'at043tn13'
		 *  Defaulting to at043tn13 lcd.
		 */
		ret = ls1x_fb_init(&at043tn13, 0, IPU_PIX_FMT_RGB565, &ls1x_lcd0_info);
		if (ret)
			printf("at043tn13 cannot be configured: %d\n", ret);
	}

	backlight(1);

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
