#ifndef __MT_GIC_V3_H
#define __MT_GIC_V3_H

/* distributor registers & their field definitions, in secure world */

#define GICD_V3_CTLR                       0x0000
#define GICD_V3_TYPER                      0x0004
#define GICD_V3_IIDR                       0x0008
#define GICD_V3_STATUSR                    0x0010
#define GICD_V3_SETSPI_NSR                 0x0040
#define GICD_V3_CLRSPI_NSR                 0x0048
#define GICD_V3_SETSPI_SR                  0x0050
#define GICD_V3_CLRSPI_SR                  0x0058
#define GICD_V3_SEIR                       0x0068
#define GICD_V3_ISENABLER                  0x0100
#define GICD_V3_ICENABLER                  0x0180
#define GICD_V3_ISPENDR                    0x0200
#define GICD_V3_ICPENDR                    0x0280
#define GICD_V3_ISACTIVER                  0x0300
#define GICD_V3_ICACTIVER                  0x0380
#define GICD_V3_IPRIORITYR                 0x0400
#define GICD_V3_ICFGR                      0x0C00
#define GICD_V3_IROUTER                    0x6000
#define GICD_V3_PIDR2                      0xFFE8

#define GICD_V3_CTLR_RWP                   (1U << 31)
#define GICD_V3_CTLR_E1NWF			(1U << 7)
#define GICD_V3_CTLR_DS			(1U << 6)
#define GICD_V3_CTLR_ARE_NS                (1U << 5)
#define GICD_V3_CTLR_ARE_S                 (1U << 4)
#define GICD_V3_CTLR_ENABLE_G1S            (1U << 2)
#define GICD_V3_CTLR_ENABLE_G1NS           (1U << 1)
#define GICD_V3_CTLR_ENABLE_G0		(1U << 0)

#define GICD_V3_TYPER_ID_BITS(typer)       ((((typer) >> 19) & 0x1f) + 1)
#define GICD_V3_TYPER_IRQS(typer)          ((((typer) & 0x1f) + 1) * 32)
#define GICD_V3_TYPER_LPIS                 (1U << 17)

#define GICD_V3_IROUTER_SPI_MODE_ONE       (0U << 31)
#define GICD_V3_IROUTER_SPI_MODE_ANY       (1U << 31)

#define GIC_V3_PIDR2_ARCH_MASK             0xf0
#define GIC_V3_PIDR2_ARCH_GICv3            0x30
#define GIC_V3_PIDR2_ARCH_GICv4            0x40

/*
 * Re-Distributor registers, offsets from RD_base
 */
#define GICR_V3_CTLR                       GICD_V3_CTLR
#define GICR_V3_IIDR                       0x0004
#define GICR_V3_TYPER                      0x0008
#define GICR_V3_STATUSR                    GICD_V3_STATUSR
#define GICR_V3_WAKER                      0x0014
#define GICR_V3_SETLPIR                    0x0040
#define GICR_V3_CLRLPIR                    0x0048
#define GICR_V3_SEIR                       GICD_V3_SEIR
#define GICR_V3_PROPBASER                  0x0070
#define GICR_V3_PENDBASER                  0x0078
#define GICE_V3_IGROUP0			   0x0080
#define GICR_V3_INVLPIR                    0x00A0
#define GICR_V3_INVALLR                    0x00B0
#define GICR_V3_SYNCR                      0x00C0
#define GICR_V3_MOVLPIR                    0x0100
#define GICR_V3_MOVALLR                    0x0110
#define GICE_V3_IGRPMOD0		   0x0d00
#define GICR_V3_PIDR2                      GICD_V3_PIDR2

#define GICR_V3_CTLR_ENABLE_LPIS           (1UL << 0)

#define GICR_V3_TYPER_CPU_NUMBER(r)        (((r) >> 8) & 0xffff)

#define GICR_V3_WAKER_ProcessorSleep       (1U << 1)
#define GICR_V3_WAKER_ChildrenAsleep       (1U << 2)

#define GICR_V3_PROPBASER_NonShareable     (0U << 10)
#define GICR_V3_PROPBASER_InnerShareable   (1U << 10)
#define GICR_V3_PROPBASER_OuterShareable   (2U << 10)
#define GICR_V3_PROPBASER_SHAREABILITY_MASK (3UL << 10)
#define GICR_V3_PROPBASER_nCnB             (0U << 7)
#define GICR_V3_PROPBASER_nC               (1U << 7)
#define GICR_V3_PROPBASER_RaWt             (2U << 7)
#define GICR_V3_PROPBASER_RaWb             (3U << 7)
#define GICR_V3_PROPBASER_WaWt             (4U << 7)
#define GICR_V3_PROPBASER_WaWb             (5U << 7)
#define GICR_V3_PROPBASER_RaWaWt           (6U << 7)
#define GICR_V3_PROPBASER_RaWaWb           (7U << 7)
#define GICR_V3_PROPBASER_IDBITS_MASK      (0x1f)

/*
 * Re-Distributor registers, offsets from SGI_base
 */
#define GICR_V3_ISENABLER0                 GICD_V3_ISENABLER
#define GICR_V3_ICENABLER0                 GICD_V3_ICENABLER
#define GICR_V3_ISPENDR0                   GICD_V3_ISPENDR
#define GICR_V3_ICPENDR0                   GICD_V3_ICPENDR
#define GICR_V3_ISACTIVER0                 GICD_V3_ISACTIVER
#define GICR_V3_ICACTIVER0                 GICD_V3_ICACTIVER
#define GICR_V3_IPRIORITYR0                GICD_V3_IPRIORITYR
#define GICR_V3_ICFGR0                     GICD_V3_ICFGR

#define GICR_V3_TYPER_PLPIS                (1U << 0)
#define GICR_V3_TYPER_VLPIS                (1U << 1)
#define GICR_V3_TYPER_LAST                 (1U << 4)

/*
 * CPU interface registers
 */
#define ICC_V3_CTLR_EL1_EOImode_drop_dir   (0U << 1)
#define ICC_V3_CTLR_EL1_EOImode_drop       (1U << 1)
#define ICC_V3_SRE_EL1_SRE                 (1U << 0)

/*
 * Hypervisor interface registers (SRE only)
 */
#define ICH_V3_LR_VIRTUAL_ID_MASK          ((1UL << 32) - 1)

#define ICH_V3_LR_EOI                      (1UL << 41)
#define ICH_V3_LR_GROUP                    (1UL << 60)
#define ICH_V3_LR_STATE                    (3UL << 62)
#define ICH_V3_LR_PENDING_BIT              (1UL << 62)
#define ICH_V3_LR_ACTIVE_BIT               (1UL << 63)

#define ICH_V3_MISR_EOI                    (1 << 0)
#define ICH_V3_MISR_U                      (1 << 1)

#define ICH_V3_HCR_EN                      (1 << 0)
#define ICH_V3_HCR_UIE                     (1 << 1)

#define ICH_V3_VMCR_CTLR_SHIFT             0
#define ICH_V3_VMCR_CTLR_MASK              (0x21f << ICH_VMCR_CTLR_SHIFT)
#define ICH_V3_VMCR_BPR1_SHIFT             18
#define ICH_V3_VMCR_BPR1_MASK              (7 << ICH_VMCR_BPR1_SHIFT)
#define ICH_V3_VMCR_BPR0_SHIFT             21
#define ICH_V3_VMCR_BPR0_MASK              (7 << ICH_VMCR_BPR0_SHIFT)
#define ICH_V3_VMCR_PMR_SHIFT              24
#define ICH_V3_VMCR_PMR_MASK               (0xffUL << ICH_VMCR_PMR_SHIFT)

/* TODO: replace sys_reg macro */
#define ICC_V3_EOIR1_EL1                   sys_reg(3, 0, 12, 12, 1)
#define ICC_V3_IAR1_EL1                    sys_reg(3, 0, 12, 12, 0)
#define ICC_V3_SGI1R_EL1                   sys_reg(3, 0, 12, 11, 5)
#define ICC_V3_PMR_EL1                     sys_reg(3, 0, 4, 6, 0)
#define ICC_V3_CTLR_EL1                    sys_reg(3, 0, 12, 12, 4)
#define ICC_V3_SRE_EL1                     sys_reg(3, 0, 12, 12, 5)
#define ICC_V3_GRPEN1_EL1                  sys_reg(3, 0, 12, 12, 7)

#define ICC_V3_IAR1_EL1_SPURIOUS           0x3ff

#define ICC_V3_SRE_EL2                     sys_reg(3, 4, 12, 9, 5)

#define ICC_V3_SRE_EL2_SRE                 (1 << 0)
#define ICC_V3_SRE_EL2_ENABLE              (1 << 3)

static inline unsigned int gicd_v3_read_ctlr(unsigned int base)
{
        return mmio_read_32(base + GICD_V3_CTLR);
}

static inline void gicd_v3_write_ctlr(unsigned int base, unsigned int val)
{
        mmio_write_32(base + GICD_V3_CTLR, val);
}

static inline unsigned int gicd_v3_read_pidr2(unsigned int base)
{
	return mmio_read_32(base + GICD_V3_PIDR2);
}

#if 0
static unsigned int gicd_v3_read_grpmodr(unsigned int base, unsigned int id)
{
	unsigned n = id >> IGROUPR_SHIFT;
	return mmio_read_32(base + GICE_V3_IGRPMOD0 + (n << 2));
}

static void gicd_write_grpmodr(unsigned int base, unsigned int id, unsigned int val)
{
	unsigned n = id >> IGROUPR_SHIFT;
	mmio_write_32(base + GICE_V3_IGRPMOD0 + (n << 2), val);
}

static void gicd_v3_clr_grpmodr(unsigned int base, unsigned int id)
{
	unsigned bit_num = id & ((1 << IGROUPR_SHIFT) - 1);
	unsigned int reg_val = gicd_v3_read_grpmodr(base, id);

	gicd_write_igroupr(base, id, reg_val & ~(1 << bit_num));
}
#endif
static void gicd_v3_set_irouter(unsigned int base, unsigned int id, uint64_t aff)
{
	unsigned int reg = base + GICD_V3_IROUTER + (id*8);

	mmio_write_64(reg, aff);
}
#endif /* end of __MT_GIC_V3_H */
