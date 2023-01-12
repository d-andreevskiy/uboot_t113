
#ifndef __PWM_SUN8I_T113__H__
#define __PWM_SUN8I_T113__H__

#define PWM_PIER	(0x0000)
#define PWM_PISR	(0x0004)
#define	PWM_CIER	(0x0010)
#define	PWM_CISR	(0x0014)

#define PWM_PPCR(x)     (0x0020 + (x / 2) * 4)

#define PWM_PCGR    (0x0040)

#define PWM_PDZCR01	(0x0060)
#define PWM_PDZCR23	(0x0064)
#define	PWM_PDZCR45	(0x0068)
#define	PWM_PDZCR67	(0x006c)

#define PWM_PER     (0x0080)

#define PGR0        (0x0090)
#define PGR1        (0x0094)
#define PGR2        (0x0098)
#define PGR3        (0x009C)

#define PWM_CER		(0x00c0)

#define PWM_PCR(x)      (0x0100 + (x) * 0x20)
#define PWM_PPR(x)      (0x0104 + (x) * 0x20)
#define PWM_PCNTR(x)    (0x0108 + (x) * 0x20)
#define PWM_PPCNTR(x)   (0x010C + (x) * 0x20)
#define PWM_CCR(x)	    (0x0110 + (x) * 0x20)
#define	PWM_CRLR(x)	    (0x0114 + (x) * 0x20)
#define PWM_CFLR(x)	    (0x0118 + (x) * 0x20)

#define PWMG_CS_SHIFT           0
#define PWMG_CS_WIDTH           16
#define PWMG_EN_SHIFT           16
#define PWMG_START_SHIFT        17
#define PWM_COUNTER_START_SHIFT 16
#define PWM_COUNTER_START_WIDTH 16
#define PWM_PUL_START_SHIFT     10
#define PWM_PUL_START_WIDTH     1
#define PWM_PUL_NUM_SHIFT       16
#define PWM_PUL_NUM_WIDTH       16
#define PWM_MODE_ACTS_SHIFT		8
#define PWM_MODE_ACTS_WIDTH     2
#define PWM_ACT_STA_SHIFT	0x8
#define PWM_ACT_STA_WIDTH	0x1
#define PWM_CLK_SRC_SHIFT	0x7
#define PWM_CLK_SRC_WIDTH	0x2
#define PWM_DIV_M_SHIFT		0x0
#define PWM_DIV_M_WIDTH		0x4
#define PWM_PRESCAL_SHIFT	0x0
#define PWM_PRESCAL_WIDTH	0x8
#define PWM_ACT_CYCLES_SHIFT	0x0
#define PWM_ACT_CYCLES_WIDTH	0x10
#define PWM_PERIOD_CYCLES_SHIFT	0x10
#define PWM_PERIOD_CYCLES_WIDTH	0x10
#define PWM_DZ_EN_SHIFT		0x0
#define PWM_DZ_EN_WIDTH		0x1
#define PWM_PDZINTV_SHIFT	0x8
#define PWM_PDZINTV_WIDTH	0x8
#define PWM_BYPASS_WIDTH	0x1
#define PWM_CLK_GATING_SHIFT	0x4
#define PWM_CLK_GATING_WIDTH	0x1

#define PWM_CLK_BYPASS_SHIFT	16

#define PWM_MAX_CHANNEL         8

#endif

