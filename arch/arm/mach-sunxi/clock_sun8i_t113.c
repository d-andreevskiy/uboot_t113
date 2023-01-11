// SPDX-License-Identifier: GPL-2.0+
/*
 * A83 specific clock code
 *
 * (C) Copyright 2007-2012
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Tom Cubie <tangliang@allwinnertech.com>
 *
 * (C) Copyright 2015 Vishnu Patekar <vishnupatekar0510@gmail.com>
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/prcm.h>
#include <asm/arch/sys_proto.h>
#include <linux/delay.h>

#ifdef CONFIG_SPL_BUILD

static inline uint32_t read32(uint32_t addr)
{
	return( *((volatile uint32_t *)(addr)) );
}

static inline void write32(uint32_t addr, uint32_t value)
{
	*((volatile uint32_t *)(addr)) = value;
}

static void set_pll_cpux_axi(void)
{
	uint32_t val;

	/* Select cpux clock src to osc24m, axi divide ratio is 3, system apb clk ratio is 4 */
	write32(T113_CCU_BASE + CCU_CPU_AXI_CFG_REG, (0 << 24) | (3 << 8) | (1 << 0));
	sdelay(1);

	/* Disable pll gating */
	val = read32(T113_CCU_BASE + CCU_PLL_CPU_CTRL_REG);
	val &= ~(1 << 27);
	write32(T113_CCU_BASE + CCU_PLL_CPU_CTRL_REG, val);

	/* Enable pll ldo */
	val = read32(T113_CCU_BASE + CCU_PLL_CPU_CTRL_REG);
	val |= (1 << 30);
	write32(T113_CCU_BASE + CCU_PLL_CPU_CTRL_REG, val);
	sdelay(5);

	/* Set default clk to 1008mhz */
	val = read32(T113_CCU_BASE + CCU_PLL_CPU_CTRL_REG);
	val &= ~((0x3 << 16) | (0xff << 8) | (0x3 << 0));
	val |= (41 << 8);
	write32(T113_CCU_BASE + CCU_PLL_CPU_CTRL_REG, val);

	/* Lock enable */
	val = read32(T113_CCU_BASE + CCU_PLL_CPU_CTRL_REG);
	val |= (1 << 29);
	write32(T113_CCU_BASE + CCU_PLL_CPU_CTRL_REG, val);

	/* Enable pll */
	val = read32(T113_CCU_BASE + CCU_PLL_CPU_CTRL_REG);
	val |= (1 << 31);
	write32(T113_CCU_BASE + CCU_PLL_CPU_CTRL_REG, val);

	/* Wait pll stable */
	while(!(read32(T113_CCU_BASE + CCU_PLL_CPU_CTRL_REG) & (0x1 << 28)));
	sdelay(20);

	/* Enable pll gating */
	val = read32(T113_CCU_BASE + CCU_PLL_CPU_CTRL_REG);
	val |= (1 << 27);
	write32(T113_CCU_BASE + CCU_PLL_CPU_CTRL_REG, val);

	/* Lock disable */
	val = read32(T113_CCU_BASE + CCU_PLL_CPU_CTRL_REG);
	val &= ~(1 << 29);
	write32(T113_CCU_BASE + CCU_PLL_CPU_CTRL_REG, val);
	sdelay(1);

	/* set and change cpu clk src to PLL_CPUX, PLL_CPUX:AXI0 = 1008M:504M */
	val = read32(T113_CCU_BASE + CCU_CPU_AXI_CFG_REG);
	val &= ~(0x07 << 24 | 0x3 << 16 | 0x3 << 8 | 0xf << 0);
	val |= (0x03 << 24 | 0x0 << 16 | 0x3 << 8 | 0x1 << 0);
	write32(T113_CCU_BASE + CCU_CPU_AXI_CFG_REG, val);
	sdelay(1);
}

static void set_pll_periph0(void)
{
	uint32_t val;

	/* Periph0 has been enabled */
	if(read32(T113_CCU_BASE + CCU_PLL_PERI0_CTRL_REG) & (1 << 31))
		return;

	/* Change psi src to osc24m */
	val = read32(T113_CCU_BASE + CCU_PSI_CLK_REG);
	val &= (~(0x3 << 24));
	write32(val, T113_CCU_BASE + CCU_PSI_CLK_REG);

	/* Set default val */
	write32(T113_CCU_BASE + CCU_PLL_PERI0_CTRL_REG, 0x63 << 8);

	/* Lock enable */
	val = read32(T113_CCU_BASE + CCU_PLL_PERI0_CTRL_REG);
	val |= (1 << 29);
	write32(T113_CCU_BASE + CCU_PLL_PERI0_CTRL_REG, val);

	/* Enabe pll 600m(1x) 1200m(2x) */
	val = read32(T113_CCU_BASE + CCU_PLL_PERI0_CTRL_REG);
	val |= (1 << 31);
	write32(T113_CCU_BASE + CCU_PLL_PERI0_CTRL_REG, val);

	/* Wait pll stable */
	while(!(read32(T113_CCU_BASE + CCU_PLL_PERI0_CTRL_REG) & (0x1 << 28)));
	sdelay(20);

	/* Lock disable */
	val = read32(T113_CCU_BASE + CCU_PLL_PERI0_CTRL_REG);
	val &= ~(1 << 29);
	write32(T113_CCU_BASE + CCU_PLL_PERI0_CTRL_REG, val);
}

static void set_ahb(void)
{
	write32(T113_CCU_BASE + CCU_PSI_CLK_REG, (2 << 0) | (0 << 8));
	write32(T113_CCU_BASE + CCU_PSI_CLK_REG, read32(T113_CCU_BASE + CCU_PSI_CLK_REG) | (0x03 << 24));
	sdelay(1);
}

static void set_apb(void)
{
	write32(T113_CCU_BASE + CCU_APB0_CLK_REG, (2 << 0) | (1 << 8));
	write32(T113_CCU_BASE + CCU_APB0_CLK_REG, (0x03 << 24) | read32(T113_CCU_BASE + CCU_APB0_CLK_REG));
	sdelay(1);
}

static void set_dma(void)
{
	/* Dma reset */
	write32(T113_CCU_BASE + CCU_DMA_BGR_REG, read32(T113_CCU_BASE + CCU_DMA_BGR_REG) | (1 << 16));
	sdelay(20);
	/* Enable gating clock for dma */
	write32(T113_CCU_BASE + CCU_DMA_BGR_REG, read32(T113_CCU_BASE + CCU_DMA_BGR_REG) | (1 << 0));
}

static void set_mbus(void)
{
	uint32_t val;

	/* Reset mbus domain */
	val = read32(T113_CCU_BASE + CCU_MBUS_CLK_REG);
	val |= (0x1 << 30);
	write32(T113_CCU_BASE + CCU_MBUS_CLK_REG, val);
	sdelay(1);
	/* Enable mbus master clock gating */
	write32(T113_CCU_BASE + CCU_MBUS_MAT_CLK_GATING_REG, 0x00000d87);
}

static void set_module(uint32_t addr)
{
	uint32_t val;

	if(!(read32(addr) & (1 << 31)))
	{
		val = read32(addr);
		write32(addr, val | (1 << 31) | (1 << 30));

		/* Lock enable */
		val = read32(addr);
		val |= (1 << 29);
		write32(addr, val);

		/* Wait pll stable */
		while(!(read32(addr) & (0x1 << 28)));
		sdelay(20);

		/* Lock disable */
		val = read32(addr);
		val &= ~(1 << 29);
		write32(addr, val);
	}
}

void sunxi_clk_init(void)
{
	set_pll_cpux_axi();
	set_pll_periph0();
	set_ahb();
	set_apb();
	set_dma();
	set_mbus();
	set_module(T113_CCU_BASE + CCU_PLL_PERI0_CTRL_REG);
	set_module(T113_CCU_BASE + CCU_PLL_VIDEO0_CTRL_REG);
	set_module(T113_CCU_BASE + CCU_PLL_VIDEO1_CTRL_REG);
	set_module(T113_CCU_BASE + CCU_PLL_VE_CTRL);
	set_module(T113_CCU_BASE + CCU_PLL_AUDIO0_CTRL_REG);
	set_module(T113_CCU_BASE + CCU_PLL_AUDIO1_CTRL_REG);
}


void clock_init_safe(void)
{
    sunxi_clk_init();
    
    // writel(0x01FFFFFF, 0x02000098);
    // writel(0x00000000, 0x020000A0);
}
#endif

void clock_init_uart(void)
{
	uint32_t addr;
	uint32_t val;

	/* Open the clock gate for usart */
	addr = T113_CCU_BASE + CCU_USART_BGR_REG;
	val	 = readl(addr);
	val |= 1 << 0;
	writel(val, addr);

	/* Deassert USART reset */
	addr = T113_CCU_BASE + CCU_USART_BGR_REG;
	val	 = readl(addr);
	val |= 1 << 16;
	writel(val, addr);

}

#ifdef CONFIG_SPL_BUILD
void clock_set_pll1(unsigned int clk)
{

}
#endif

unsigned int clock_get_pll6(void)
{
    return 0;
}

unsigned int clock_get_peri1x_rate(void)
{
	uint32_t reg32;
	uint8_t	 plln, pllm, p0;

	/* PLL PERIx */
	reg32 = readl(T113_CCU_BASE + CCU_PLL_PERI0_CTRL_REG);
	if (reg32 & (1 << 31)) {
		plln = ((reg32 >> 8) & 0xff) + 1;
		pllm = (reg32 & 0x01) + 1;
		p0	 = ((reg32 >> 16) & 0x03) + 1;

		return ((((24 * plln) / (pllm * p0)) >> 1) * 1000 * 1000);
	}

	return 0;
}

unsigned int clock_set_video(void)
{
    struct sunxi_ccm_reg * const ccm =
        (struct sunxi_ccm_reg *)SUNXI_CCM_BASE;

    /* PLL_VIDEO0(4X) / 3 = 1188MHz / 3 = 396MHz */
	setbits_le32(&ccm->de_clk_cfg, 0x81000002);
    setbits_le32(&ccm->tcon_lcd0_clk_cfg, 0x81000002);

    return 0;
}

unsigned int clock_get_video(void)
{
    return 396000000;
}
