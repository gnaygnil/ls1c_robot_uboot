#
# Copyright (C) 2013 Tang Haifeng <tanghaifeng-gz@loongson.cn>
#
# SPDX-License-Identifier:	GPL-2.0+
#

PLATFORM_CPPFLAGS += -march=mips32
PLATFORM_CPPFLAGS += -mabi=32 -DCONFIG_32BIT
ifdef CONFIG_SYS_BIG_ENDIAN
PLATFORM_LDFLAGS  += -m elf32btsmip
else
PLATFORM_LDFLAGS  += -m elf32ltsmip
endif

CONFIG_STANDALONE_LOAD_ADDR ?= 0x80200000 -T mips.lds
