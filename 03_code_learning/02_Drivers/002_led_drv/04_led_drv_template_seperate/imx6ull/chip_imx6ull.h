#ifndef CHIP_IMX6ULL_H
#define CHIP_IMX6ULL_H

#include <linux/types.h>

// 位设置与清除
#define BIT_CLR(val, bit) ((val) &= ~(1 << (bit)))
#define BIT_SET(val, bit) ((val) |= (1 << (bit)))

// CCM地址范围(0x020C4000~0x020C7FFF)
#define CCM_BASE_ADDR 0x020C4000
// GPIO5地址范围(0x020AC000~0x020ACFFF)
#define GPIO5_BASE_ADDR 0x020AC000
// IOMUXC_SNVS地址范围(0x02290000~0x02293FFF)
#define IOMUXC_SNVS_BASE_ADDR 0x02290000

#define CCGR1_GPIO5_CLK_ENABLE_BIT 30

#define IOMUXC_SION_BIT 4
#define IOMUXC_MODE_BIT 0

/*
 * CCM(Clock Control Module)时钟模块
 */
typedef struct {
  volatile uint32_t CCR;          // 0x00
  volatile uint32_t CCDR;         // 0x04
  volatile uint32_t CSR;          // 0x08
  volatile uint32_t CCSR;         // 0x0C
  volatile uint32_t CACRR;        // 0x10
  volatile uint32_t CBCDR;        // 0x14
  volatile uint32_t CBCMR;        // 0x18
  volatile uint32_t CSCMR1;       // 0x1C
  volatile uint32_t CSCMR2;       // 0x20
  volatile uint32_t CSCDR1;       // 0x24
  volatile uint32_t CS1CDR;       // 0x28
  volatile uint32_t CS2CDR;       // 0x2C
  volatile uint32_t CDCDR;        // 0x30
  volatile uint32_t CHSCCDR;      // 0x34
  volatile uint32_t CSCDR2;       // 0x38
  volatile uint32_t CSCDR3;       // 0x3C
  volatile uint32_t RESERVED0[2]; // 0x40&0x44
  volatile uint32_t CDHIPR;       // 0x48
  volatile uint32_t RESERVED1[2]; // 0x4C&0x50
  volatile uint32_t CLPCR;        // 0x54
  volatile uint32_t CISR;         // 0x58
  volatile uint32_t CIMR;         // 0x5C
  volatile uint32_t CCOSR;        // 0x60
  volatile uint32_t CGPR;         // 0x64
  volatile uint32_t CCGR0;        // 0x68
  volatile uint32_t CCGR1;        // 0x6C
  volatile uint32_t CCGR2;        // 0x70
  volatile uint32_t CCGR3;        // 0x74
  volatile uint32_t CCGR4;        // 0x78
  volatile uint32_t CCGR5;        // 0x7C
  volatile uint32_t CCGR6;        // 0x80
  volatile uint32_t RESERVED2[1]; // 0x84
  volatile uint32_t CMEOR;        // 0x88
} CCM_TypeDef;

typedef struct {
  volatile uint32_t IOMUXC_SNVS_SW_MUX_CTL_PAD_BOOT_MODE0;   // 0x00
  volatile uint32_t IOMUXC_SNVS_SW_MUX_CTL_PAD_BOOT_MODE1;   // 0x04
  volatile uint32_t IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER0; // 0x08
  volatile uint32_t IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER1; // 0x0C
  volatile uint32_t IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER2; // 0x10
  volatile uint32_t IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3; // 0x14
  volatile uint32_t IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER4; // 0x18
  volatile uint32_t IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER5; // 0x1C
  volatile uint32_t IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER6; // 0x20
  volatile uint32_t IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER7; // 0x24
  volatile uint32_t IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER8; // 0x28
  volatile uint32_t IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER9; // 0x2C
} IOMUXC_SNVS_TypeDef;

typedef struct {
  volatile uint32_t DR;       // 0x00
  volatile uint32_t GDIR;     // 0x04
  volatile uint32_t PSR;      // 0x08
  volatile uint32_t ICR1;     // 0x0C
  volatile uint32_t ICR2;     // 0x10
  volatile uint32_t IMR;      // 0x14
  volatile uint32_t ISR;      // 0x18
  volatile uint32_t EDGE_SEL; // 0x1C
} GPIOx_TypeDef;

#endif // !CHIP_IMX6ULL_H