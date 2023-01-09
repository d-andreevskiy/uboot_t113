# SPDX-License-Identifier: GPL-2.0+
#
# (C) Copyright 2000-2007
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.

obj-$(CONFIG_VIDEO_SUNXI) += sunxi_display.o simplefb_common.o lcdc.o tve_common.o ../videomodes.o

ifdef CONFIG_MACH_SUN8I_T113
obj-$(CONFIG_VIDEO_DE2) += sunxi_de2.o simplefb_common.o lcdc.o sunxi_lcd.o
else 
obj-$(CONFIG_VIDEO_DE2) += sunxi_de2.o sunxi_dw_hdmi.o simplefb_common.o lcdc.o sunxi_lcd.o
endif
