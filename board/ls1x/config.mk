#
# Copyright (c) 2013 Tang, Haifeng <tanghaifeng-gz@loongson.cn>
#
# SPDX-License-Identifier:	GPL-2.0+
#

#
# Loongson1 development board ls1b, MIPS32 core
#
ifdef CONFIG_NAND_BOOT_EN
# RAM version
CONFIG_SYS_TEXT_BASE = 0x80100000
else
# ROM version
CONFIG_SYS_TEXT_BASE = 0xbfc00000
endif
