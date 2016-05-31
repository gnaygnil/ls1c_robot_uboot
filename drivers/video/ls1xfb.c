/*
 * Porting to u-boot:
 *
 * (C) Copyright 2014
 * TangHaifeng <tanghaifeng-gz@loongson.cn>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/errno.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/fb.h>
#include <linux/compat.h>
#include <asm/io.h>
#include <malloc.h>
#include <video_fb.h>
#include <div64.h>

#include <asm/ls1x.h>
#include <asm/arch/ls1xfb.h>
#include <asm/arch/regs-clk.h>
#include <asm/arch/serial_reg.h>

#include "videomodes.h"
#include "ls1xfb.h"

/* graphics setup */
static GraphicDevice panel;
static struct fb_videomode const *gmode;
static uint8_t gdisp;
static uint32_t gpixfmt;
static struct ls1xfb_mach_info *mi;

DECLARE_GLOBAL_DATA_PTR;

#define PICOS2KHZ(a) (1000000000UL/(a))

/* 如果已配置好LCD控制器，这里可以不用再次初始化，可以避免屏幕切换时的花屏 */
#define INIT_AGAIN
#ifdef INIT_AGAIN
#define writel_reg(val, addr)	writel(val, addr);
#else
#define writel_reg(val, addr)
#endif

#define DEFAULT_REFRESH		75	/* Hz */
#define DEFAULT_XRES	800
#define DEFAULT_YRES	600
#define DEFAULT_BPP	16
//static u32 default_refresh = DEFAULT_REFRESH;
static u32 default_xres = DEFAULT_XRES;
static u32 default_yres = DEFAULT_YRES;
static u32 default_bpp = DEFAULT_BPP;
static int vga_mode = 0;

/**
 * fb_var_to_videomode - convert fb_var_screeninfo to fb_videomode
 * @mode: pointer to struct fb_videomode
 * @var: pointer to struct fb_var_screeninfo
 */
void fb_var_to_videomode(struct fb_videomode *mode,
			 const struct fb_var_screeninfo *var)
{
	u32 pixclock, hfreq, htotal, vtotal;

	mode->name = NULL;
	mode->xres = var->xres;
	mode->yres = var->yres;
	mode->pixclock = var->pixclock;
	mode->hsync_len = var->hsync_len;
	mode->vsync_len = var->vsync_len;
	mode->left_margin = var->left_margin;
	mode->right_margin = var->right_margin;
	mode->upper_margin = var->upper_margin;
	mode->lower_margin = var->lower_margin;
	mode->sync = var->sync;
	mode->vmode = var->vmode & FB_VMODE_MASK;
	mode->flag = FB_MODE_IS_FROM_VAR;
	mode->refresh = 0;

	if (!var->pixclock)
		return;

	pixclock = PICOS2KHZ(var->pixclock) * 1000;

	htotal = var->xres + var->right_margin + var->hsync_len +
		var->left_margin;
	vtotal = var->yres + var->lower_margin + var->vsync_len +
		var->upper_margin;

	if (var->vmode & FB_VMODE_INTERLACED)
		vtotal /= 2;
	if (var->vmode & FB_VMODE_DOUBLE)
		vtotal *= 2;

	hfreq = pixclock/htotal;
	mode->refresh = hfreq/vtotal;
}

void fb_videomode_to_var(struct fb_var_screeninfo *var,
			 const struct fb_videomode *mode)
{
	var->xres = mode->xres;
	var->yres = mode->yres;
	var->xres_virtual = mode->xres;
	var->yres_virtual = mode->yres;
	var->xoffset = 0;
	var->yoffset = 0;
	var->pixclock = mode->pixclock;
	var->left_margin = mode->left_margin;
	var->right_margin = mode->right_margin;
	var->upper_margin = mode->upper_margin;
	var->lower_margin = mode->lower_margin;
	var->hsync_len = mode->hsync_len;
	var->vsync_len = mode->vsync_len;
	var->sync = mode->sync;
	var->vmode = mode->vmode & FB_VMODE_MASK;
}

static int determine_best_pix_fmt(struct fb_var_screeninfo *var)
{
	switch (var->bits_per_pixel) {
		case 1:
		case 2:
		case 4:
		case 8:
		case 12:
			return PIX_FMT_RGB444;
			break;
		case 15:
			return PIX_FMT_RGB1555;
			break;
		case 16:
			return PIX_FMT_RGB565;
			break;
		case 24:
		case 32:
			return PIX_FMT_RGBA888;
			break;
		default:
			return PIX_FMT_RGB565;
			break;
	}
	return -EINVAL;
}

static void set_pix_fmt(struct fb_var_screeninfo *var, int pix_fmt)
{
	switch (pix_fmt) {
	case PIX_FMT_RGB444:
		var->bits_per_pixel = 16;
		var->red.offset = 8;    var->red.length = 4;
		var->green.offset = 4;   var->green.length = 4;
		var->blue.offset = 0;    var->blue.length = 4;
		var->transp.offset = 0;  var->transp.length = 0;
		break;
	case PIX_FMT_RGB1555:
		var->bits_per_pixel = 16;
		var->red.offset = 10;    var->red.length = 5;
		var->green.offset = 5;   var->green.length = 5;
		var->blue.offset = 0;    var->blue.length = 5;
		var->transp.offset = 0; var->transp.length = 0;
		break;
	case PIX_FMT_RGB565:
		var->bits_per_pixel = 16;
		var->red.offset = 11;    var->red.length = 5;
		var->green.offset = 5;   var->green.length = 6;
		var->blue.offset = 0;    var->blue.length = 5;
		var->transp.offset = 0;  var->transp.length = 0;
		break;
	case PIX_FMT_RGB888PACK:
		var->bits_per_pixel = 24;
		var->red.offset = 16;    var->red.length = 8;
		var->green.offset = 8;   var->green.length = 8;
		var->blue.offset = 0;    var->blue.length = 8;
		var->transp.offset = 0;  var->transp.length = 0;
		break;
	case PIX_FMT_RGB888UNPACK:
		var->bits_per_pixel = 32;
		var->red.offset = 16;    var->red.length = 8;
		var->green.offset = 8;   var->green.length = 8;
		var->blue.offset = 0;    var->blue.length = 8;
		var->transp.offset = 24; var->transp.length = 8;
		break;
	case PIX_FMT_RGBA888:
		var->bits_per_pixel = 32;
		var->red.offset = 16;    var->red.length = 8;
		var->green.offset = 8;   var->green.length = 8;
		var->blue.offset = 0;    var->blue.length = 8;
		var->transp.offset = 24; var->transp.length = 8;
		break;
	}
}

static int ls1xfb_check_var(struct fb_var_screeninfo *var,
			      struct fb_info *info)
{
	struct ls1xfb_info *fbi = info->par;
	int pix_fmt, mode_valid = 0;

	/*
	 * Determine which pixel format we're going to use.
	 */
	pix_fmt = determine_best_pix_fmt(var);
	if (pix_fmt < 0)
		return pix_fmt;
	set_pix_fmt(var, pix_fmt);
	fbi->pix_fmt = pix_fmt;

	if (!mode_valid) {
		const struct fb_videomode *mode;

		mode = mi->modes;
		if (mode) {
//			ls1x_update_var(var, mode);
			fb_videomode_to_var(var, mode);
			mode_valid = 1;
		}
	}

	if (!mode_valid && info->monspecs.modedb_len)
		return -EINVAL;

	/*
	 * Basic geometry sanity checks.
	 */
	if (var->xoffset + var->xres > var->xres_virtual) {
		return -EINVAL;
	}
	if (var->yoffset + var->yres > var->yres_virtual) {
		return -EINVAL;
	}
	if (var->xres + var->right_margin +
	    var->hsync_len + var->left_margin > 3000)
		return -EINVAL;
	if (var->yres + var->lower_margin +
	    var->vsync_len + var->upper_margin > 2000)
		return -EINVAL;

	/*
	 * Check size of framebuffer.
	 */
	if (var->xres_virtual * var->yres_virtual *
	    (var->bits_per_pixel >> 3) > info->fix.smem_len)
		return -EINVAL;
	return 0;
}

#ifdef CONFIG_CPU_LOONGSON1B
/* only for 1B vga */
static void set_uart_clock_divider(void)
{
	u32 pll, ctrl, rate, divisor;
	u32 x;
	int i;
	int ls1b_uart_base[] = {
		LS1X_UART0_BASE, LS1X_UART1_BASE, LS1X_UART2_BASE,
		LS1X_UART3_BASE, LS1X_UART4_BASE, LS1X_UART5_BASE,
		LS1X_UART6_BASE, LS1X_UART7_BASE, LS1X_UART8_BASE,
		LS1X_UART9_BASE, LS1X_UART10_BASE, LS1X_UART11_BASE
	};
	#define UART_PORT(id, offset)	(u8 *)(KSEG1ADDR(ls1b_uart_base[id] + offset))

	pll = readl(LS1X_CLK_PLL_FREQ);
	ctrl = readl(LS1X_CLK_PLL_DIV) & DIV_DDR;
	rate = (12 + (pll & 0x3f)) * OSC_CLK / 2
			+ ((pll >> 8) & 0x3ff) * OSC_CLK / 1024 / 2;
	rate = rate / (ctrl >> DIV_DDR_SHIFT);
	divisor = rate / 2 / (16*115200);
	
	for (i=0; i<12; i++) {
		x = readb(UART_PORT(i, UART_LCR));
		writeb(x | UART_LCR_DLAB, UART_PORT(i, UART_LCR));
	
		writeb(divisor & 0xff, UART_PORT(i, UART_DLL));
		writeb((divisor>>8) & 0xff, UART_PORT(i, UART_DLM));
	
		writeb(x & ~UART_LCR_DLAB, UART_PORT(i, UART_LCR));
	}
}

static void set_clock_divider_forls1bvga(struct ls1xfb_info *fbi,
			      const struct fb_videomode *m)
{
	struct ls1b_vga *input_vga;
	extern struct ls1b_vga ls1b_vga_modes[];

	for (input_vga=ls1b_vga_modes; input_vga->ls1b_pll_freq !=0; ++input_vga) {
//		if((input_vga->xres == m->xres) && (input_vga->yres == m->yres) && 
//			(input_vga->refresh == m->refresh)) {
		if ((input_vga->xres == m->xres) && (input_vga->yres == m->yres)) {
			break;
		}
	}
	if (input_vga->ls1b_pll_freq) {
		writel(input_vga->ls1b_pll_freq, LS1X_CLK_PLL_FREQ);
		writel(input_vga->ls1b_pll_div, LS1X_CLK_PLL_DIV);
		set_uart_clock_divider();
	}
}
#endif	//#ifdef CONFIG_CPU_LOONGSON1B

#ifdef CONFIG_CPU_LOONGSON1A
int caclulatefreq(unsigned int sys_clk, unsigned int pclk)
{
/* N值和OD值选择不正确会造成系统死机，莫名其妙。OD=2^PIX12 */
	unsigned int N = 4, PIX12 = 2, OD = 4;
	unsigned int M = 0, FRAC = 0;
	unsigned long tmp1, tmp2;

	while (1) {
		tmp2 = pclk * N * OD;
		M = tmp2 / sys_clk;
		if (M <= 1) {
			N++;
		} else {
			tmp1 = sys_clk * M;
			if (tmp2 < tmp1) {
				unsigned int tmp3;
				tmp3 = tmp1; tmp1 = tmp2; tmp2 = tmp3;
			}
			if ((tmp2 - tmp1) > 16384) {
				if (N < 15 ) {
					N++;
				} else {
					N = 15; PIX12++; OD *= 2;
					if (PIX12 > 3) {
						printk(KERN_WARNING "Warning: \
								clock source is out of range.\n");
						break;
					}
				}
			}
			else {
				FRAC = ((tmp2 - tmp1) * 262144) / sys_clk;
				break;
			}
		}
	}
	return ((FRAC<<14) + (PIX12<<12) + (N<<8) + M);
}
#endif	//#ifdef CONFIG_CPU_LOONGSON1A

static void set_clock_divider(struct ls1xfb_info *fbi,
			      const struct fb_videomode *m)
{
	int divider_int;
	int needed_pixclk;
	u64 div_result;

	/*
	 * Notice: The field pixclock is used by linux fb
	 * is in pixel second. E.g. struct fb_videomode &
	 * struct fb_var_screeninfo
	 */

	/*
	 * Check input values.
	 */
	if (!m || !m->pixclock || !m->refresh) {
		printf("Input refresh or pixclock is wrong.\n");
		return;
	}

	/*
	 * Calc divider according to refresh rate.
	 */
	div_result = 1000000000000ll;
	do_div(div_result, m->pixclock);
	needed_pixclk = (u32)div_result;

#if defined(CONFIG_CPU_LOONGSON1A)
	#define PLL_CTRL(x)		(x)
	/* 设置gpu时钟频率为200MHz */
	divider_int = caclulatefreq(OSC_CLK/1000, 200000);
	writel(divider_int, PLL_CTRL(LS1X_GPU_PLL_CTRL));
	/* 像素时钟 */
	divider_int = caclulatefreq(OSC_CLK/1000, needed_pixclk/1000);
	writel(divider_int, PLL_CTRL(LS1X_PIX1_PLL_CTRL));
	writel(divider_int, PLL_CTRL(LS1X_PIX2_PLL_CTRL));
#elif defined(CONFIG_CPU_LOONGSON1B)
	divider_int = gd->arch.pll_clk / needed_pixclk / 4;
	/* check whether divisor is too small. */
	if (divider_int < 1) {
		printf("Warning: clock source is too slow."
				"Try smaller resolution\n");
		divider_int = 1;
	}
	else if(divider_int > 15) {
		printf("Warning: clock source is too fast."
				"Try smaller resolution\n");
		divider_int = 15;
	}

	/*
	 * Set setting to reg.
	 */
	{
		u32 regval = 0;
		regval = readl(LS1X_CLK_PLL_DIV);
		regval |= 0x00003000;	//dc_bypass 置1
		regval &= ~0x00000030;	//dc_rst 置0
		regval &= ~(0x1f<<26);	//dc_div 清零
		regval |= divider_int << 26;
		writel(regval, LS1X_CLK_PLL_DIV);
		regval &= ~0x00001000;	//dc_bypass 置0
		writel(regval, LS1X_CLK_PLL_DIV);
	}
#elif defined(CONFIG_CPU_LOONGSON1C)
	divider_int = gd->arch.pll_clk / needed_pixclk;
	/* check whether divisor is too small. */
	if (divider_int < 2) {
		printf("Warning: clock source is too slow."
				"Try smaller resolution\n");
		divider_int = 1;
	}
	else if(divider_int > 127) {
		printf("Warning: clock source is too fast."
				"Try smaller resolution\n");
		divider_int = 0x7f;
	}

	/*
	 * Set setting to reg.
	 */
	{
		u32 regval = 0;
		regval = readl(LS1X_CLK_PLL_DIV);
		/* 注意：首先需要把分频使能位清零 */
		writel(regval & ~DIV_DC_EN, LS1X_CLK_PLL_DIV);
		regval |= DIV_DC_EN | DIV_DC_SEL_EN | DIV_DC_SEL;
		regval &= ~DIV_DC;
		regval |= divider_int << DIV_DC_SHIFT;
		writel(regval, LS1X_CLK_PLL_DIV);
	}
#endif
}

static void set_graphics_start(struct fb_info *info, int xoffset, int yoffset)
{
	struct ls1xfb_info *fbi = info->par;
	struct fb_var_screeninfo *var = &info->var;
	int pixel_offset;
	unsigned long addr;

	pixel_offset = (yoffset * var->xres_virtual) + xoffset;

	addr = fbi->fb_start_dma + (pixel_offset * (var->bits_per_pixel >> 3));
	writel(addr, fbi->reg_base + LS1X_FB_ADDR0);
	writel(addr, fbi->reg_base + LS1X_FB_ADDR1);
}

static void set_dumb_panel_control(struct fb_info *info)
{
	struct ls1xfb_info *fbi = info->par;
	u32 x;

	/*
	 * Preserve enable flag.
	 */
	x = readl(fbi->reg_base + LS1X_FB_PANEL_CONF) & 0x80001110;

	if (unlikely(vga_mode)) {
		/* have to set 0x80001310 */
		writel_reg(x | 0x80001310, fbi->reg_base + LS1X_FB_PANEL_CONF);
	} else {
		x |= mi->invert_pixde ? LS1X_FB_PANEL_CONF_DE_POL : 0;
		x |= mi->invert_pixclock ? LS1X_FB_PANEL_CONF_CLK_POL : 0;
		x |= mi->de_mode ? LS1X_FB_PANEL_CONF_DE : 0;
		writel_reg(x | 0x80001110, fbi->reg_base + LS1X_FB_PANEL_CONF);
	}

	if (!mi->de_mode) {
		x = readl(fbi->reg_base + LS1X_FB_HSYNC) & ~LS1X_FB_HSYNC_POL;
		x |= (info->var.sync & FB_SYNC_HOR_HIGH_ACT) ? LS1X_FB_HSYNC_POL : 0;
		writel_reg(x, fbi->reg_base + LS1X_FB_HSYNC);

		x = readl(fbi->reg_base + LS1X_FB_VSYNC) & ~LS1X_FB_VSYNC_POL;
		x |= (info->var.sync & FB_SYNC_VERT_HIGH_ACT) ? LS1X_FB_VSYNC_POL : 0;
		writel_reg(x, fbi->reg_base + LS1X_FB_VSYNC);
	} else {
		writel_reg(0x0, fbi->reg_base + LS1X_FB_HSYNC);
		writel_reg(0x0, fbi->reg_base + LS1X_FB_VSYNC);
	}
}

static void set_dumb_screen_dimensions(struct fb_info *info)
{
	struct ls1xfb_info *fbi = info->par;
	struct fb_var_screeninfo *v = &info->var;
	int x;
	int y;

	x = v->xres + v->right_margin + v->hsync_len + v->left_margin;
	y = v->yres + v->lower_margin + v->vsync_len + v->upper_margin;

	writel_reg((readl(fbi->reg_base + LS1X_FB_HDISPLAY) & ~LS1X_FB_HDISPLAY_TOTAL) | (x << 16), 
		fbi->reg_base + LS1X_FB_HDISPLAY);
	writel_reg((readl(fbi->reg_base + LS1X_FB_VDISPLAY) & ~LS1X_FB_HDISPLAY_TOTAL) | (y << 16), 
		fbi->reg_base + LS1X_FB_VDISPLAY);
}

static int ls1xfb_set_par(struct fb_info *info)
{
	struct ls1xfb_info *fbi = info->par;
	struct fb_var_screeninfo *var = &info->var;
	struct fb_videomode mode;
	u32 x;

	/*
	 * Set additional mode info.
	 */
	info->fix.visual = FB_VISUAL_TRUECOLOR;
	info->fix.line_length = var->xres_virtual * var->bits_per_pixel / 8;
//	info->fix.ypanstep = var->yres;

	/*
	 * Disable panel output while we setup the display.
	 */
	x = readl(fbi->reg_base + LS1X_FB_PANEL_CONF);
	writel_reg(x & ~LS1X_FB_PANEL_CONF_DE, fbi->reg_base + LS1X_FB_PANEL_CONF);

	/*
	 * convet var to video mode
	 */
	fb_var_to_videomode(&mode, &info->var);

	/* Calculate clock divisor. */
#ifdef CONFIG_CPU_LOONGSON1B
	if (unlikely(vga_mode)) {
		set_clock_divider_forls1bvga(fbi, &mode);
	}
	else 
#endif
	{
		set_clock_divider(fbi, &mode);
	}

	/*
	 * Configure dumb panel ctrl regs & timings.
	 */
	set_dumb_panel_control(info);
	set_dumb_screen_dimensions(info);

	writel_reg((readl(fbi->reg_base + LS1X_FB_HDISPLAY) & ~LS1X_FB_HDISPLAY_END) | (var->xres),
		fbi->reg_base + LS1X_FB_HDISPLAY);	/* 显示屏一行中显示区的像素数 */
	writel_reg((readl(fbi->reg_base + LS1X_FB_VDISPLAY) & ~LS1X_FB_VDISPLAY_END) | (var->yres),
		fbi->reg_base + LS1X_FB_VDISPLAY);	/* 显示屏中显示区的行数 */

	if (mi->de_mode) {
		writel_reg(0x00000000, fbi->reg_base + LS1X_FB_HSYNC);
		writel_reg(0x00000000, fbi->reg_base + LS1X_FB_VSYNC);
	} else {
		writel_reg((readl(fbi->reg_base + LS1X_FB_HSYNC) & 0xc0000000) | 0x40000000 | 
				((var->right_margin + var->xres + var->hsync_len) << 16) | 
				(var->right_margin + var->xres),
				fbi->reg_base + LS1X_FB_HSYNC);
		writel_reg((readl(fbi->reg_base + LS1X_FB_VSYNC) & 0xc0000000) | 0x40000000 | 
				((var->lower_margin + var->yres + var->vsync_len) << 16) | 
				(var->lower_margin + var->yres),
				fbi->reg_base + LS1X_FB_VSYNC);
	}

	/*
	 * Configure global panel parameters.
	 */
	x = readl(fbi->reg_base + LS1X_FB_CONF) & ~LS1X_FB_CONF_FORMAT;
	if (fbi->pix_fmt > 3) {
		writel_reg(x | LS1X_FB_CONF_RESET | 4, fbi->reg_base + LS1X_FB_CONF);
	}
	else {
		writel_reg(x | LS1X_FB_CONF_RESET | fbi->pix_fmt, fbi->reg_base + LS1X_FB_CONF);
	}

#if defined(CONFIG_CPU_LOONGSON1C)
	writel_reg((info->fix.line_length + 0x7f) & ~0x7f, fbi->reg_base + LS1X_FB_STRIDE);
#else
	writel_reg((info->fix.line_length + 0xff) & ~0xff, fbi->reg_base + LS1X_FB_STRIDE);
#endif
	writel_reg(0, fbi->reg_base + LS1X_FB_ORIGIN);

	/*
	 * Re-enable panel output.
	 */
	x = readl(fbi->reg_base + LS1X_FB_PANEL_CONF);
	writel_reg(x | LS1X_FB_PANEL_CONF_DE, fbi->reg_base + LS1X_FB_PANEL_CONF);

	return 0;
}

static int ls1xfb_init_mode(struct fb_info *info,
			      struct ls1xfb_mach_info *mi)
{
	struct fb_var_screeninfo *var = &info->var;
	int ret = 0;
	u32 total_w, total_h, refresh;
	u64 div_result;

	/*
	 * Set default value
	 */
	refresh = mi->modes->refresh;
	if (default_bpp)
		var->bits_per_pixel = default_bpp;

	#if defined(CONFIG_FB_LS1X_I2C)
	ls1x_update_var(&info->var, mi->modes);
	#else
	fb_videomode_to_var(&info->var, mi->modes);
	#endif

	printf("%s:%dx%d-%d@%d\n", mi->modes->name, 
		var->xres, var->yres, var->bits_per_pixel, refresh);

	/* Init settings. */
	/* No need for virtual resolution support */
	var->xres_virtual = var->xres;
	var->yres_virtual = info->fix.smem_len /
		(var->xres_virtual * (var->bits_per_pixel >> 3));
	printf("ls1xfb: find best mode: res = %dx%d\n",
				var->xres, var->yres);

	/* correct pixclock. */
	total_w = var->xres + var->left_margin + var->right_margin +
		  var->hsync_len;
	total_h = var->yres + var->upper_margin + var->lower_margin +
		  var->vsync_len;

	div_result = 1000000000000ll;
	do_div(div_result, total_w * total_h * refresh);
	var->pixclock = (u32)div_result;

	return ret;
}

/*
 * Initializes the framebuffer information pointer. After allocating
 * sufficient memory for the framebuffer structure, the fields are
 * filled with custom information passed in from the configurable
 * structures.  This includes information such as bits per pixel,
 * color maps, screen width/height and RGBA offsets.
 *
 * @return      Framebuffer structure initialized with our information
 */
static struct fb_info *ls1xfb_init_fbinfo(void)
{
#define BYTES_PER_LONG 4
#define PADDING (BYTES_PER_LONG - (sizeof(struct fb_info) % BYTES_PER_LONG))
	struct fb_info *info;
	struct ls1xfb_info *ls1xfbi;
	char *p;
	int size = sizeof(struct ls1xfb_info) + PADDING +
		sizeof(struct fb_info);

	debug("%s: %d %d %d %d\n",
		__func__,
		PADDING,
		size,
		sizeof(struct ls1xfb_info),
		sizeof(struct fb_info));
	/*
	 * Allocate sufficient memory for the fb structure
	 */

	p = malloc(size);
	if (!p)
		return NULL;

	memset(p, 0, size);

	info = (struct fb_info *)p;
	info->par = p + sizeof(struct fb_info) + PADDING;

	ls1xfbi = (struct ls1xfb_info *)info->par;
	debug("Framebuffer structures at: info=0x%x ls1xfbi=0x%x\n",
		(unsigned int)info, (unsigned int)ls1xfbi);

	return info;
}

static int ls1xfb_probe(u32 interface_pix_fmt, uint8_t disp,
			struct fb_videomode const *mode)
{
	struct fb_info *info;
	struct ls1xfb_info *ls1xfbi;
	int ret = 0;

	/*
	 * Initialize FB structures
	 */
	info = ls1xfb_init_fbinfo();
	if (!info) {
		ret = -ENOMEM;
		goto err0;
	}

	/* Initialize private data */
	ls1xfbi = (struct ls1xfb_info *)info->par;
	ls1xfbi->info = info;
	ls1xfbi->de_mode = mi->de_mode;

	/*
	 * Initialise static fb parameters.
	 */
	info->flags = FBINFO_FLAG_DEFAULT;
	info->node = -1;
	strlcpy(info->fix.id, mi->id, 16);
	info->fix.type = FB_TYPE_PACKED_PIXELS;
	info->fix.type_aux = 0;
	info->fix.xpanstep = 0;
	info->fix.ypanstep = 0;
	info->fix.ywrapstep = 0;
	info->fix.accel = FB_ACCEL_NONE;
	info->pseudo_palette = ls1xfbi->pseudo_palette;

	/*
	 * Map LCD controller registers.
	 */
	ls1xfbi->reg_base = (unsigned long *)mi->base;

	/*
	 * Allocate framebuffer memory.
	 */
	if (unlikely(vga_mode)) {
		info->fix.smem_len = ALIGN(1920 * 1080 * 4, PAGE_SIZE); /* 分配足够的显存，用于切换分辨率 */
	} else {
		info->fix.smem_len = ALIGN(default_xres * default_yres * 4, PAGE_SIZE);
	}
	info->screen_size = info->fix.smem_len;

	info->screen_base = (char *)(((unsigned int)malloc(info->fix.smem_len) & 0x0fffffff) | 0xa0000000);
	if (info->screen_base == NULL) {
		ret = -ENOMEM;
		goto failed_free_info;
	}
	ls1xfbi->fb_start_dma = virt_to_phys(info->screen_base);

	info->fix.smem_start = (unsigned long)ls1xfbi->fb_start_dma;
	set_graphics_start(info, 0, 0);

	mi->modes = (struct fb_videomode *)mode;
	mi->num_modes = 1;

	/*
	 * init video mode data.
	 */
	ls1xfb_init_mode(info, mi);

	/*
	 * Fill in sane defaults.
	 */
	ret = ls1xfb_check_var(&info->var, info);
	if (ret)
		goto failed_free_fbmem;

	/* init ls1x lcd controller */
	{
		u32 x;
		int timeout = 204800;
		x = readl(ls1xfbi->reg_base + LS1X_FB_CONF);
		writel_reg(x & ~LS1X_FB_CONF_RESET, ls1xfbi->reg_base + LS1X_FB_CONF);
//		writel_reg(x & ~LS1X_FB_CONF_OUT_EN, ls1xfbi->reg_base + LS1X_FB_CONF);
		x = readl(ls1xfbi->reg_base + LS1X_FB_CONF);	/* 不知道为什么这里需要读一次 */
		writel_reg(x & ~LS1X_FB_CONF_RESET, ls1xfbi->reg_base + LS1X_FB_CONF);
		x = readl(ls1xfbi->reg_base + LS1X_FB_CONF);

		ls1xfb_set_par(info);

		/* 不知道为什么要设置多次才有效,且对LS1X_FB_CONF寄存器的设置不能放在ls1xfb_set_par函数里，
		因为在设置format字段后再设置LS1X_FB_CONF_OUT_EN位会使format字段设置失效 */
		x = readl(ls1xfbi->reg_base + LS1X_FB_CONF);
		writel_reg(x | LS1X_FB_CONF_OUT_EN, ls1xfbi->reg_base + LS1X_FB_CONF);
		do {
			x = readl(ls1xfbi->reg_base + LS1X_FB_CONF);
			writel_reg(x | LS1X_FB_CONF_OUT_EN, ls1xfbi->reg_base + LS1X_FB_CONF);
		} while (((x & LS1X_FB_CONF_OUT_EN) == 0)
				&& (timeout-- > 0));
	}

	panel.winSizeX = mode->xres;
	panel.winSizeY = mode->yres;
	panel.plnSizeX = mode->xres;
	panel.plnSizeY = mode->yres;

	panel.frameAdrs = (u32)info->screen_base;
	panel.memSize = info->screen_size;

	panel.gdfBytesPP = 2;
	panel.gdfIndex = GDF_16BIT_565RGB;

	return 0;

failed_free_fbmem:
#ifdef CONFIG_FB_LS1X_I2C
	ls1xfb_delete_i2c_busses(info);
#endif
	free(info->screen_base);
failed_free_info:
	free(info);
err0:
	return ret;
}

void ls1x_fb_shutdown(void)
{
}

void *video_hw_init(void)
{
	int ret;

	ret = ls1xfb_probe(gpixfmt, gdisp, gmode);
	debug("Framebuffer at 0x%x %d\n", (unsigned int)panel.frameAdrs, ret);

	return (void *)&panel;
}

void video_set_lut(unsigned int index, /* color number */
			unsigned char r,    /* red */
			unsigned char g,    /* green */
			unsigned char b     /* blue */
			)
{
	return;
}

int ls1x_fb_init(struct fb_videomode const *mode,
		  uint8_t disp,
		  uint32_t pixfmt,
		  struct ls1xfb_mach_info *ls1x_mi)
{
	gmode = mode;
	gdisp = disp;
	gpixfmt = pixfmt;
	mi = ls1x_mi;

	return 0;
}
