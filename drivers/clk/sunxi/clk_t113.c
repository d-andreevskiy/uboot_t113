// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (C) 2018 Amarula Solutions.
 * Author: Jagan Teki <jagan@amarulasolutions.com>
 */

#include <common.h>
#include <clk-uclass.h>
#include <dm.h>
#include <errno.h>
#include <clk/sunxi.h>
#include <dt-bindings/clock/sun8i-t113-ccu.h>
#include <dt-bindings/reset/sun8i-t113-ccu.h>
#include <linux/bitops.h>

static struct ccu_clk_gate t113_gates[] = {
	[CLK_BUS_MMC0]		= GATE(0x84C, BIT(0)),
	[CLK_BUS_SPI0]		= GATE(0x96c, BIT(0)),
	[CLK_BUS_UART0]		= GATE(0x90C, BIT(0)),
    [CLK_BUS_OTG]		= GATE(0xa8c, BIT(8)),

	[CLK_USB_OHCI0]		= GATE(0xa70, BIT(31)),
	[CLK_USB_OHCI1]		= GATE(0xa74, BIT(31)),
	[CLK_BUS_OHCI0]		= GATE(0xa8c, BIT(0)),
	[CLK_BUS_OHCI1]		= GATE(0xa8c, BIT(1)),
	[CLK_BUS_EHCI0]		= GATE(0xa8c, BIT(4)),
	[CLK_BUS_EHCI1]		= GATE(0xa8c, BIT(5)),

    [CLK_BUS_PWM]		= GATE(0x7ac, BIT(0)),
    
    [CLK_SPI0]		    = GATE(0x940, BIT(31)),
};

static struct ccu_reset t113_resets[] = {
	[RST_BUS_MMC0]		= RESET(0x84C, BIT(16)),
	[RST_BUS_SPI0]		= RESET(0x96c, BIT(16)),
	[RST_BUS_UART0]		= RESET(0x90C, BIT(16)),
    [RST_BUS_OTG]		= RESET(0xa8c, BIT(24)),
    [RST_USB_PHY0]		= RESET(0xa70, BIT(30)),
    [RST_USB_PHY1]		= RESET(0xa74, BIT(30)),

	[RST_BUS_OHCI0]		= RESET(0xa8c, BIT(16)),
	[RST_BUS_OHCI1]		= RESET(0xa8c, BIT(17)),
	[RST_BUS_EHCI0]		= RESET(0xa8c, BIT(20)),
	[RST_BUS_EHCI1]		= RESET(0xa8c, BIT(21)),

    [RST_BUS_PWM]		= RESET(0x7ac, BIT(16)),
};

const struct ccu_desc t113_ccu_desc = {
	.gates = t113_gates,
	.resets = t113_resets,
	.num_gates = ARRAY_SIZE(t113_gates),
	.num_resets = ARRAY_SIZE(t113_resets),
};
