#
# Copyright (c) 2013 Tang, Haifeng <tanghaifeng-gz@loongson.cn>
#
# SPDX-License-Identifier:	GPL-2.0+
#

include $(TOPDIR)/config.mk

LIB	= $(obj)lib$(BOARD).o

COBJS-y	+= $(BOARD).o
SOBJS	= lowlevel_init.o
COBJS-$(CONFIG_CPU_LOONGSON1A)	+= ddr2_config_1a_CL3.o
COBJS-$(CONFIG_CPU_LOONGSON1B)	+= ddr2_config_1b_CL3.o
COBJS-$(CONFIG_VIDEO)	+= ls1x_video.o

COBJS	:= $(COBJS-y)
SRCS	:= $(SOBJS:.o=.S) $(COBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS))
SOBJS	:= $(addprefix $(obj),$(SOBJS))

$(LIB):	$(obj).depend $(OBJS) $(SOBJS)
	$(call cmd_link_o_target, $(OBJS) $(SOBJS))

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
