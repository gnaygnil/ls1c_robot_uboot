#
# Copyright 2009-2011 Freescale Semiconductor, Inc.
#
# SPDX-License-Identifier:	GPL-2.0+
#

include $(TOPDIR)/config.mk

LIB	:= $(obj)libgmac.o

COBJS-$(CONFIG_LS1X_GMAC) += synopGMAC_Dev.o synopGMAC_plat.o synopGMAC_network_interface.o

COBJS	:= $(COBJS-y)
SRCS	:= $(COBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS))

all:	$(LIB)

$(LIB):	$(obj).depend $(OBJS)
	$(call cmd_link_o_target, $(OBJS))

#########################################################################

include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
