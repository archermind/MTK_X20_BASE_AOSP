#ifndef _LOADER_H_
#define _LOADER_H_

#define MPU_PROTECT     1
#define WDT_SUPPORT     0
#define TCM_FORCE_LOCK  0

#define LOADER_BASE     0x00000000
#define LOADER_SIZE     0x9     /* 2KB */
#define MPU_BASE        0xE000ED80
#define MPU_SIZE        0x5     /* 64 bytes */
#define SRAM_BASE       0x00000000
#define SRAM_SIZE       0x12    /* 512KB */
#define MEM_BASE        0x20000000
#define MEM_SPACE       0x1F    /* 4GB */
#define DRAM_REMAP_ADR  0x20000000
#define DRAM_REMAP_MASK 0x00080000
#define UNUSED_REGION   0x1C    /* 512MB (0x8000~0x2000_0000) */

#define REMAP_CFG0      0x400A0014
#define REMAP_CFG1      0x400A0018
#define REMAP_VAL0      0x21987654
#define REMAP_VAL1      0x112200A3
#define TCM_LOCK_CFG    0x400A0010
#define TCM_LOCK_SHIFT  8
#define TCM_LOCK_UNIT   12      /* 4KB */
#define TCM_EN_LOCK_CFG 0x400A00DC
#define TCM_EN_CFG      0x400A00E0
#define TCM_EN_BIT      (1 << 20)

#define WDT_REG_BASE    0x400A0084
#define WDT_EN          (0 << 31)
#define WDT_INTV        (10 * 32768)
#define WDT_INTV_MASK   0x000FFFFF
#define WDT_TICK        0x00000001
#define MPU_TYPE        0xE000ED90
#define MPU_CTRL        0xE000ED94
#define MPU_RBAR        0xE000ED9C
#define MPU_RASR        0xE000EDA0
#define VTOR            0xE000ED08
#define CPACR           0xE000ED88
#define CPACR_VAL       (0xF << 20)

#define OFF_DRAM_START  0
#define OFF_DRAM_LEN    4

#endif
