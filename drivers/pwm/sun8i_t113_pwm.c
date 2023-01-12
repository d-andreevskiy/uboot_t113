// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2017-2018 Vasily Khoruzhick <anarsoul@gmail.com>
 */

#include <common.h>
#include <div64.h>
#include <dm.h>
#include <log.h>
#include <pwm.h>
#include <regmap.h>
#include <syscon.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <asm/arch/pwm.h>
#include <power/regulator.h>
#include "sun8i_t113_pwm.h"
#include <clk.h>
#include <reset.h>

DECLARE_GLOBAL_DATA_PTR;

#define OSC_24MHZ 24000000

struct sunxi_pwm_priv {
	u32 reg_base;
	bool invert;
	u32 prescaler;
};

// static const u32 prescaler_table[] = {
// 	120,	/* 0000 */
// 	180,	/* 0001 */
// 	240,	/* 0010 */
// 	360,	/* 0011 */
// 	480,	/* 0100 */
// 	0,	/* 0101 */
// 	0,	/* 0110 */
// 	0,	/* 0111 */
// 	12000,	/* 1000 */
// 	24000,	/* 1001 */
// 	36000,	/* 1010 */
// 	48000,	/* 1011 */
// 	72000,	/* 1100 */
// 	0,	/* 1101 */
// 	0,	/* 1110 */
// 	1,	/* 1111 */
// };

static int sunxi_pwm_set_invert(struct udevice *dev, uint channel,
				bool polarity)
{
	struct sunxi_pwm_priv *priv = dev_get_priv(dev);

	debug("%s: polarity=%u\n", __func__, polarity);
	priv->invert = polarity;

	return 0;
}

/* 
 * div1 ： pre_scal  value: 0 - 8    div_vlaue = (1 << div1)
 * div2 ： value： 0 - 255   div_value = div2 + 1
 * entire_cycle: 0 - 65535  value = entire_cycle + 1
 * act_cycle: 0-65535
*/
static void sunxi_pwm_set_clock(struct udevice *dev, uint channel, 
                        uint div1, uint div2, 
                        uint entire_cycle, uint act_cycle)
{
	struct sunxi_pwm_priv *priv = dev_get_priv(dev);
	u32 reg_base = priv->reg_base;
    uint v;

    /* clock set */
	v = readl(reg_base + PWM_PPCR(channel));
    v &= ~0x0f;
    v |= div1;
    writel(v, reg_base + PWM_PPCR(channel));	

	v = readl(reg_base + PWM_PCR(channel));
    v &= ~0xff;
    v |= div2;
    writel(v, reg_base + PWM_PCR(channel));	

    /* set duty */
    v = ((entire_cycle) << PWM_ACT_CYCLES_WIDTH) | (act_cycle << PWM_ACT_CYCLES_SHIFT);
    writel(v, reg_base + PWM_PPR(channel));	

}

static int sunxi_pwm_set_config(struct udevice *dev, uint channel,
				uint period_ns, uint duty_ns)
{
	uint entire_cycles = 0;
	uint active_cycles = 0;
    uint pre_scal_id = 0;
    uint entire_cycles_max = 65536;
    uint freq = 0;

	uint pre_scal[][2] = {
		{0, 1},
		{1, 2},
		{2, 4},
		{3, 8},
		{4, 16},
		{5, 32},
		{6, 64},
		{7, 128},
		{8, 256},
	};

	debug("%s: period_ns=%u, duty_ns=%u\n", __func__, period_ns, duty_ns);

    if (period_ns < 42) 
    {
        /* clock greater than 24MHz, then direct output 24M clock */
        pre_scal_id = 0;
        entire_cycles = 1;
        active_cycles = 1;
    }
    else
    {
        if (period_ns < 10667)
            freq = 93747;
        else if (period_ns > 1000000000)
            freq = 1;
        else
            freq = 1000000000 / period_ns;   

        entire_cycles = 24000000 / freq / pre_scal[pre_scal_id][1];   

        while (entire_cycles > entire_cycles_max) {
            pre_scal_id++;

            if (pre_scal_id > (ARRAY_SIZE(pre_scal) - 1))
                break;

            entire_cycles = 24000000 / freq / pre_scal[pre_scal_id][1];
        }

        if (period_ns < 5 * 100 * 1000)
            active_cycles = (duty_ns * entire_cycles + (period_ns / 2)) / period_ns;
        else if (period_ns >= 5 * 100 * 1000 && period_ns < 6553500)
            active_cycles = ((duty_ns / 100) * entire_cycles + (period_ns / 2 / 100)) / (period_ns / 100);
        else
            active_cycles = ((duty_ns / 10000) * entire_cycles + (period_ns / 2 / 10000)) / (period_ns / 10000);
    }

    debug("pre_scal_id = %d, entire_cycles = %d, active_cycles = %d\n", pre_scal_id, entire_cycles, active_cycles);

    sunxi_pwm_set_clock(dev, channel, pre_scal_id, 0, (entire_cycles -1), active_cycles);

	return 0;
}

static int sunxi_pwm_set_enable(struct udevice *dev, uint channel, bool enable)
{
	struct sunxi_pwm_priv *priv = dev_get_priv(dev);
	u32 reg_base = priv->reg_base;
	u32 v;
    
	debug("%s: Enable '%s'\n", __func__, dev->name);

    if(channel >= PWM_MAX_CHANNEL)
    {
        debug("error: max chnnel is %d\n", PWM_MAX_CHANNEL - 1);
        return -EINVAL;
    }

    if(priv->invert)
    {
        /* pwm active low */
        v = readl(reg_base + PWM_PCR(channel));
        v &= ~(1 << PWM_MODE_ACTS_SHIFT);
        writel(v, reg_base + PWM_PCR(channel));	
    }
    else
    {
        /* pwm active high */
        v = readl(reg_base + PWM_PCR(channel));
        v |= (1 << PWM_MODE_ACTS_SHIFT);
        writel(v, reg_base + PWM_PCR(channel));	
    }

    if(enable)
    {
        /* enable clock gating */
        v = readl(reg_base + PWM_PCGR);
        v |= (1 << channel);
        writel(v, reg_base + PWM_PCGR);	        

        /* enable pwm controller */
        v = readl(reg_base + PWM_PER);
        v |= (1 << channel);
        writel(v, reg_base + PWM_PER);
    }
    else
    {
        v = readl(reg_base + PWM_PER);
        v &= ~(1 << channel);
        writel(v, reg_base + PWM_PER);     

        v = readl(reg_base + PWM_PCGR);
        v &= ~(1 << channel);
        writel(v, reg_base + PWM_PCGR);	    
    }

	return 0;
}

static int sunxi_pwm_of_to_plat(struct udevice *dev)
{
	struct sunxi_pwm_priv *priv = dev_get_priv(dev);

	priv->reg_base = dev_read_addr(dev);

    debug("addr = 0x%x\n", priv->reg_base);

	return 0;
}

static int sunxi_pwm_probe(struct udevice *dev)
{
    int ret = 0;
    struct clk clk;
    struct reset_ctl reset;

	ret = reset_get_by_index(dev, 0, &reset);
	if (!ret)
		reset_deassert(&reset);

	ret = clk_get_by_index(dev, 0, &clk);
	if (!ret)
		clk_enable(&clk);

	return ret;
}

static const struct pwm_ops sunxi_pwm_ops = {
	.set_invert	= sunxi_pwm_set_invert,
	.set_config	= sunxi_pwm_set_config,
	.set_enable	= sunxi_pwm_set_enable,
};

static const struct udevice_id sunxi_pwm_ids[] = {
	{ .compatible = "allwinner,sun8i-t113-pwm" },
	{ }
};

U_BOOT_DRIVER(sunxi_pwm) = {
	.name	= "sun8i_t113_pwm",
	.id	= UCLASS_PWM,
	.of_match = sunxi_pwm_ids,
	.ops	= &sunxi_pwm_ops,
	.of_to_plat	= sunxi_pwm_of_to_plat,
	.probe		= sunxi_pwm_probe,
	.priv_auto	= sizeof(struct sunxi_pwm_priv),
};
