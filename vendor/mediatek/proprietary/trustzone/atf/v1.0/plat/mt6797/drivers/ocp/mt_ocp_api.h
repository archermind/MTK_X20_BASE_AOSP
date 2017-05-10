/**
 * @file    mt_ocp_api.h
 * @brief   Driver header for Over Current Protect
 *
 */

#include <mmio.h> 

/*
 * BIT Operation
 */
#undef  BIT
#define BIT(_bit_)                    (unsigned)(1 << (_bit_))
#define BITS(_bits_, _val_)           ((((unsigned) -1 >> (31 - ((1) ? _bits_))) & ~((1U << ((0) ? _bits_)) - 1)) & ((_val_)<<((0) ? _bits_)))
#define BITMASK(_bits_)               (((unsigned) -1 >> (31 - ((1) ? _bits_))) & ~((1U << ((0) ? _bits_)) - 1))
#define GET_BITS_VAL(_bits_, _val_)   (((_val_) & (BITMASK(_bits_))) >> ((0) ? _bits_))



//#define ocp_read(addr)          (*(volatile unsigned int *)(addr))
//#define ocp_write(addr, val)    (*(volatile unsigned int *)(addr) = (unsigned int)(val))
/**
 * Read/Write a field of a register.
 * @addr:       Address of the register
 * @range:      The field bit range in the form of MSB:LSB
 * @val:        The value to be written to the field
 */

#define ocp_read(addr)	                     mmio_read_32(addr)
#define ocp_read_field(addr, range)	         GET_BITS_VAL(range, ocp_read(addr))
#define ocp_write(addr, val)                 mmio_write_32(addr, val)
#define ocp_write_field(addr, range, val)    ocp_write(addr, (ocp_read(addr) & ~(BITMASK(range))) | BITS(range, val))

#define ATF_LOG_ON 1
#define HW_API_DEBUG_ON 0
#define HW_API_RET_DEBUG_ON 0

/**
 * OCP control register
 */
#define OCP_BASE_ADDR      (0x10220000) //ocp_base


//eFuse for BIG
#define EFUSE_LkgMonTRIM      (0x0)
#define EFUSE_Cpu0LkgTrim     (0x0)
#define EFUSE_Cpu1LkgTrim     (0x0)
#define EFUSE_PowerCal        (0x200)

#define PTP3_OD0              (0x10206660)
#define PTP3_OD1              (0x10206664)
#define PTP3_OD2              (0x10206668)
#define PTP3_OD3              (0x1020666C)

//eFuse for LITTLE
#define EFUSE_LLkgMonTRIM      (0x0)
#define EFUSE_LCpu0LkgTrim     (0x0)
#define EFUSE_LCpu1LkgTrim     (0x0)
#define EFUSE_LCpu2LkgTrim     (0x0)
#define EFUSE_LCpu3LkgTrim     (0x0)
#define EFUSE_LPowerCal        (0x200)

//BIG
#define OCPAPBSTATUS00         (OCP_BASE_ADDR + 0x2500)
#define OCPAPBSTATUS01         (OCP_BASE_ADDR + 0x2504)
#define OCPAPBSTATUS02         (OCP_BASE_ADDR + 0x2508)
#define OCPAPBSTATUS03         (OCP_BASE_ADDR + 0x250C)
#define OCPAPBSTATUS04         (OCP_BASE_ADDR + 0x2510)
#define OCPAPBSTATUS05         (OCP_BASE_ADDR + 0x2514)
#define OCPAPBSTATUS06         (OCP_BASE_ADDR + 0x2518)
#define OCPAPBSTATUS07         (OCP_BASE_ADDR + 0x251C)

#define OCPAPBCFG00            (OCP_BASE_ADDR + 0x2520)
#define OCPAPBCFG01            (OCP_BASE_ADDR + 0x2524)
#define OCPAPBCFG02            (OCP_BASE_ADDR + 0x2528)
#define OCPAPBCFG03            (OCP_BASE_ADDR + 0x252C)
#define OCPAPBCFG04            (OCP_BASE_ADDR + 0x2530)
#define OCPAPBCFG05            (OCP_BASE_ADDR + 0x2534)
#define OCPAPBCFG06            (OCP_BASE_ADDR + 0x2538)
#define OCPAPBCFG07            (OCP_BASE_ADDR + 0x253C)
#define OCPAPBCFG08            (OCP_BASE_ADDR + 0x2540)
#define OCPAPBCFG09            (OCP_BASE_ADDR + 0x2544)
#define OCPAPBCFG10            (OCP_BASE_ADDR + 0x2548)
#define OCPAPBCFG11            (OCP_BASE_ADDR + 0x254C)
#define OCPAPBCFG12            (OCP_BASE_ADDR + 0x2550)
#define OCPAPBCFG13            (OCP_BASE_ADDR + 0x2554)
#define OCPAPBCFG14            (OCP_BASE_ADDR + 0x2558)
#define OCPAPBCFG15            (OCP_BASE_ADDR + 0x255C)
#define OCPAPBCFG16            (OCP_BASE_ADDR + 0x2560)
#define OCPAPBCFG17            (OCP_BASE_ADDR + 0x2564)
#define OCPAPBCFG18            (OCP_BASE_ADDR + 0x2568)
#define OCPAPBCFG19            (OCP_BASE_ADDR + 0x256C)
#define OCPAPBCFG20            (OCP_BASE_ADDR + 0x2570)
#define OCPAPBCFG21            (OCP_BASE_ADDR + 0x2574)
#define OCPAPBCFG22            (OCP_BASE_ADDR + 0x2578)
#define OCPAPBCFG23            (OCP_BASE_ADDR + 0x257C)
#define OCPAPBCFG24            (OCP_BASE_ADDR + 0x2580)
#define OCPAPBCFG25            (OCP_BASE_ADDR + 0x2584)
#define OCPAPBCFG26            (OCP_BASE_ADDR + 0x2588)
#define OCPAPBCFG27            (OCP_BASE_ADDR + 0x258C)
#define OCPAPBCFG28            (OCP_BASE_ADDR + 0x2590)


// LITTLE1
#define MP0_OCP_ENABLE         (OCP_BASE_ADDR + 0x1040)      //mp0_ocpenable     ocp enable register
#define MP0_MAF_CTRL           (OCP_BASE_ADDR + 0x1044)      //mp0_maf_ctrl      maf control register
#define MP0_OCP_LKG_CTRL       (OCP_BASE_ADDR + 0x1058)      //mp0_ocplkg_ctrl   lkgmonitor control
#define MP0_OCP_OC_CTRL        (OCP_BASE_ADDR + 0x105C)      //mp0_oc_ctrl       OC control  -->  OCWASel,fpiwasel,OCMin
#define MP0_OCP_SCAL           (OCP_BASE_ADDR + 0x115C)      //mp0_ocpscale_ctrl scale register
#define MP0_OCP_NCPU_LKGMON    (OCP_BASE_ADDR + 0x1160)      //mp0_coptoplkgmon_ctrl Bit 0 : TopLkgInit
#define MP0_OCP_CPU_LKGMON     (OCP_BASE_ADDR + 0x1164)      //mp0_coptoplkgmon_ctrl Bit 0~3 : Cpu0~3LkgInit

#define MP0_OCP_CPU_EVSEL      (OCP_BASE_ADDR + 0x116C)      //mp0_ocpcpuevsel   core event sel control register
#define MP0_OCP_NCPU_EVSEL     (OCP_BASE_ADDR + 0x1170)      //mp0_ocpncpuevsel  non-core event sel control register

#define MP0_OCP_CPU_EVSHIFT0   (OCP_BASE_ADDR + 0x1174)      //mp0_ocpcpuevshift  core event shift control register
#define MP0_OCP_CPU_EVSHIFT1   (OCP_BASE_ADDR + 0x1178)      //mp0_ocpcpuevshift1 core event shift control register
#define MP0_OCP_CPU_EVSHIFT2   (OCP_BASE_ADDR + 0x117C)      //mp0_ocpcpuevshift2 core event shift control register

#define MP0_OCP_NCPU_EVSHIFT0  (OCP_BASE_ADDR + 0x1180)      //mp0_ocpncpuevshift  non-core event shift register
#define MP0_OCP_NCPU_EVSHIFT1  (OCP_BASE_ADDR + 0x1184)      //mp0_ocpncpuevshift1 non-core event shift register

#define MP0_OCP_CPU_R0         (OCP_BASE_ADDR + 0x10A0)      //mp0_ocpcpur0   OCP Core coeff for power index 0
#define MP0_OCP_CPU_R1         (OCP_BASE_ADDR + 0x10A4)      //mp0_ocpcpur1   OCP Core coeff for power index 1
#define MP0_OCP_CPU_R2         (OCP_BASE_ADDR + 0x10A8)      //mp0_ocpcpur2   OCP Core coeff for power index 2
#define MP0_OCP_CPU_R3         (OCP_BASE_ADDR + 0x10AC)      //mp0_ocpcpur3   OCP Core coeff for power index 3
#define MP0_OCP_CPU_R4         (OCP_BASE_ADDR + 0x10B0)      //mp0_ocpcpur4   OCP Core coeff for power index 4
#define MP0_OCP_CPU_R5         (OCP_BASE_ADDR + 0x10B4)      //mp0_ocpcpur5   OCP Core coeff for power index 5
#define MP0_OCP_CPU_R6         (OCP_BASE_ADDR + 0x10B8)      //mp0_ocpcpur6   OCP Core coeff for power index 6
#define MP0_OCP_CPU_R7         (OCP_BASE_ADDR + 0x10BC)      //mp0_ocpcpur7   OCP Core coeff for power index 7
#define MP0_OCP_CPU_R8         (OCP_BASE_ADDR + 0x10C0)      //mp0_ocpcpur8   OCP Core coeff for power index 8
#define MP0_OCP_CPU_R9         (OCP_BASE_ADDR + 0x10C4)      //mp0_ocpcpur9   OCP Core coeff for power index 9
#define MP0_OCP_CPU_R10        (OCP_BASE_ADDR + 0x10C8)      //mp0_ocpcpur10  OCP Core coeff for power index 10
#define MP0_OCP_CPU_R11        (OCP_BASE_ADDR + 0x10CC)      //mp0_ocpcpur11  OCP Core coeff for power index 11
#define MP0_OCP_CPU_R12        (OCP_BASE_ADDR + 0x10D0)      //mp0_ocpcpur12  OCP Core coeff for power index 12
#define MP0_OCP_CPU_R13        (OCP_BASE_ADDR + 0x10D4)      //mp0_ocpcpur13  OCP Core coeff for power index 13
#define MP0_OCP_CPU_R14        (OCP_BASE_ADDR + 0x10D8)      //mp0_ocpcpur14  OCP Core coeff for power index 14
#define MP0_OCP_CPU_R15        (OCP_BASE_ADDR + 0x10DC)      //mp0_ocpcpur15  OCP Core coeff for power index 15
#define MP0_OCP_CPU_R16        (OCP_BASE_ADDR + 0x10E0)      //mp0_ocpcpur16  OCP Core coeff for power index 16

#define MP0_OCP_NCPU_R0        (OCP_BASE_ADDR + 0x1100)      //mp0_ocpncpu0   OCP TOP coeff for power index 0
#define MP0_OCP_NCPU_R1        (OCP_BASE_ADDR + 0x1104)      //mp0_ocpncpu1   OCP TOP coeff for power index 1
#define MP0_OCP_NCPU_R2        (OCP_BASE_ADDR + 0x1108)      //mp0_ocpncpu2   OCP TOP coeff for power index 2
#define MP0_OCP_NCPU_R3        (OCP_BASE_ADDR + 0x110C)      //mp0_ocpncpu3   OCP TOP coeff for power index 3
#define MP0_OCP_NCPU_R4        (OCP_BASE_ADDR + 0x1110)      //mp0_ocpncpu4   OCP TOP coeff for power index 4
#define MP0_OCP_NCPU_R5        (OCP_BASE_ADDR + 0x1114)      //mp0_ocpncpu5   OCP TOP coeff for power index 5
#define MP0_OCP_NCPU_R6        (OCP_BASE_ADDR + 0x1118)      //mp0_ocpncpu6   OCP TOP coeff for power index 6
#define MP0_OCP_NCPU_R7        (OCP_BASE_ADDR + 0x111C)      //mp0_ocpncpu7   OCP TOP coeff for power index 7
#define MP0_OCP_NCPU_R8        (OCP_BASE_ADDR + 0x1120)      //mp0_ocpncpu8   OCP TOP coeff for power index 8
#define MP0_OCP_CPU_ROFF       (OCP_BASE_ADDR + 0x108C)      //mp0_ocpcpuroff Roff coefficient
#define MP0_OCP_NCPU_LKG       (OCP_BASE_ADDR + 0x1140)      //mp0_ocpncpulkg non-core leakage
#define MP0_OCP_CPU_LKG        (OCP_BASE_ADDR + 0x1144)      //mp0_ocpcpurlkg cpu leakage

#define MP0_OCP_FPIDTKI        (OCP_BASE_ADDR + 0x1148)      //mp0_ocpfpidtki fpi control
#define MP0_OCP_FREQ_CTRL      (OCP_BASE_ADDR + 0x114C)      //mp0_ocpfreq_   ctrl freq control register
#define MP0_OCP_FPIKP          (OCP_BASE_ADDR + 0x1150)      //mp0_ocpfpikp   fpi control

#define MP0_OCP_IDVFS_VACT     (OCP_BASE_ADDR + 0x118C)      //mp0_idvfs_vact IDVFS_VACT
#define MP0_OCP_IDVFS_VPRED    (OCP_BASE_ADDR + 0x1190)      //mp0_idvfs_vpred IDVFS_VPRED
#define MP0_OCP_IDVFS_FACT     (OCP_BASE_ADDR + 0x1194)      //mp0_idvfs_fact IDVFS_FACT
#define MP0_OCP_IDVFS_VREQMAX  (OCP_BASE_ADDR + 0x1198)      
#define MP0_OCP_IDVFS_VACT_WR  (OCP_BASE_ADDR + 0x119C)      //mp0_idvfs_vact_wr idvfs_vact_wr
#define MP0_OCP_IDVFS_VPRED_WR (OCP_BASE_ADDR + 0x11A0)
#define MP0_OCP_IDVFS_FACT_WR  (OCP_BASE_ADDR + 0x11A4)      //mp0_idvfs_fact_wr idvfs_fact_wr
#define MP0_OCP_IDVFS_FREQMAX  (OCP_BASE_ADDR + 0x11A8)

#define MP0_OCP_CPU_POSTCTRL0  (OCP_BASE_ADDR + 0x1700)      //mp0_ocpcpupost_ctrl0 event0~9 post control
#define MP0_OCP_CPU_POSTCTRL1  (OCP_BASE_ADDR + 0x1704)      //mp0_ocpcpupost_ctrl1 event10~19 post control
#define MP0_OCP_CPU_POSTCTRL2  (OCP_BASE_ADDR + 0x1708)      //mp0_ocpcpupost_ctrl2 event20~29 post control
#define MP0_OCP_CPU_POSTCTRL3  (OCP_BASE_ADDR + 0x170C)      //mp0_ocpcpupost_ctrl3 event30~39 post control
#define MP0_OCP_CPU_POSTCTRL4  (OCP_BASE_ADDR + 0x1710)      //mp0_ocpcpupost_ctrl4 event40~49 post control
#define MP0_OCP_CPU_POSTCTRL5  (OCP_BASE_ADDR + 0x1714)      //mp0_ocpcpupost_ctrl5 event50~59 post control
#define MP0_OCP_CPU_POSTCTRL6  (OCP_BASE_ADDR + 0x1718)      //mp0_ocpcpupost_ctrl6 event60~69 post control
#define MP0_OCP_CPU_POSTCTRL7  (OCP_BASE_ADDR + 0x171C)      //mp0_ocpcpupost_ctrl7 event70~79 post control

#define MP0_OCP_NCPU_POSTCTRL0 (OCP_BASE_ADDR + 0x17A0)      //mp0_ocppncpupost_ctrl ncpu-event0~9 post control

#define MP0_OCP_OCWATGT        (OCP_BASE_ADDR + 0x1060)      //mp0_ocpocwatgt ocwatgt (Q8.12)
#define MP0_OCP_VOLT_CTRL      (OCP_BASE_ADDR + 0x1070)      //mp0_ocp_voltage_ctrl voltage control register
#define MP0_OCP_OCKP           (OCP_BASE_ADDR + 0x1154)      //mp0_ocpockp oc kp control
#define MP0_OCP_OCDTKI         (OCP_BASE_ADDR + 0x1158)      //mp0_ocpocdtki oc dtki control
#define MP0_OCP_GENERAL_CTRL   (OCP_BASE_ADDR + 0x17FC)      //mp0_ocp_general_ctrl

#define MP0_OCP_DBG_IFCTRL     (OCP_BASE_ADDR + 0x1200)
#define MP0_OCP_DBG_IFCTRL1    (OCP_BASE_ADDR + 0x1204)
#define MP0_OCP_DBG_STAT       (OCP_BASE_ADDR + 0x1208)
#define MP0_OCP_DBG_ACT_L      (OCP_BASE_ADDR + 0x1250)
#define MP0_OCP_DBG_ACT_H      (OCP_BASE_ADDR + 0x1254)
#define MP0_OCP_DBG_LKG_L      (OCP_BASE_ADDR + 0x1260)
#define MP0_OCP_DBG_LKG_H      (OCP_BASE_ADDR + 0x1264)

#define MP0_OCP_LKGMAX         (OCP_BASE_ADDR + 0x1084)
#define MP0_OCP_OCCGMIN        (OCP_BASE_ADDR + 0x1088)
#define MP0_OCP_WAMAXACT       (OCP_BASE_ADDR + 0x1074)
#define MP0_OCP_WAMINACT       (OCP_BASE_ADDR + 0x1078)

#define MP0_OCP_IRQSTATE       (OCP_BASE_ADDR + 0x1000)
#define MP0_OCP_CAP_STATUS00   (OCP_BASE_ADDR + 0x1004)
#define MP0_OCP_CAP_STATUS01   (OCP_BASE_ADDR + 0x1008)
#define MP0_OCP_CAP_STATUS02   (OCP_BASE_ADDR + 0x100C)
#define MP0_OCP_CAP_STATUS03   (OCP_BASE_ADDR + 0x1010)
#define MP0_OCP_CAP_STATUS04   (OCP_BASE_ADDR + 0x1014)
#define MP0_OCP_CAP_STATUS05   (OCP_BASE_ADDR + 0x1018)
#define MP0_OCP_CAP_STATUS06   (OCP_BASE_ADDR + 0x101C)
#define MP0_OCP_CAP_STATUS07   (OCP_BASE_ADDR + 0x1020)
#define MP0_OCP_IRQCLR         (OCP_BASE_ADDR + 0x1048)
#define MP0_OCP_IRQEN          (OCP_BASE_ADDR + 0x104C)

#define MP0_OCP_CAP_HOLDOFF    (OCP_BASE_ADDR + 0x1050)
#define MP0_OCP_CAP_CTRL       (OCP_BASE_ADDR + 0x1054)

// LITTLE2
#define MP1_OCP_ENABLE         (OCP_BASE_ADDR + 0x3040)
#define MP1_MAF_CTRL           (OCP_BASE_ADDR + 0x3044)
#define MP1_OCP_LKG_CTRL       (OCP_BASE_ADDR + 0x3058)
#define MP1_OCP_OC_CTRL        (OCP_BASE_ADDR + 0x305C)
#define MP1_OCP_SCAL           (OCP_BASE_ADDR + 0x315C)
#define MP1_OCP_NCPU_LKGMON    (OCP_BASE_ADDR + 0x3160)      //mp1_coptoplkgmon_ctrl Bit 0 : TopLkgInit
#define MP1_OCP_CPU_LKGMON     (OCP_BASE_ADDR + 0x3164)      //mp1_coptoplkgmon_ctrl Bit 0~3 : Cpu0~3LkgInit
#define MP1_OCP_CPU_EVSEL      (OCP_BASE_ADDR + 0x316C)
#define MP1_OCP_NCPU_EVSEL     (OCP_BASE_ADDR + 0x3170)
#define MP1_OCP_CPU_EVSHIFT0   (OCP_BASE_ADDR + 0x3174)
#define MP1_OCP_CPU_EVSHIFT1   (OCP_BASE_ADDR + 0x3178)
#define MP1_OCP_CPU_EVSHIFT2   (OCP_BASE_ADDR + 0x317C)
#define MP1_OCP_NCPU_EVSHIFT0  (OCP_BASE_ADDR + 0x3180)
#define MP1_OCP_NCPU_EVSHIFT1  (OCP_BASE_ADDR + 0x3184)

#define MP1_OCP_CPU_R0         (OCP_BASE_ADDR + 0x30A0)
#define MP1_OCP_CPU_R1         (OCP_BASE_ADDR + 0x30A4)
#define MP1_OCP_CPU_R2         (OCP_BASE_ADDR + 0x30A8)
#define MP1_OCP_CPU_R3         (OCP_BASE_ADDR + 0x30AC)
#define MP1_OCP_CPU_R4         (OCP_BASE_ADDR + 0x30B0)
#define MP1_OCP_CPU_R5         (OCP_BASE_ADDR + 0x30B4)
#define MP1_OCP_CPU_R6         (OCP_BASE_ADDR + 0x30B8)
#define MP1_OCP_CPU_R7         (OCP_BASE_ADDR + 0x30BC)
#define MP1_OCP_CPU_R8         (OCP_BASE_ADDR + 0x30C0)
#define MP1_OCP_CPU_R9         (OCP_BASE_ADDR + 0x30C4)
#define MP1_OCP_CPU_R10        (OCP_BASE_ADDR + 0x30C8)
#define MP1_OCP_CPU_R11        (OCP_BASE_ADDR + 0x30CC)
#define MP1_OCP_CPU_R12        (OCP_BASE_ADDR + 0x30D0)
#define MP1_OCP_CPU_R13        (OCP_BASE_ADDR + 0x30D4)
#define MP1_OCP_CPU_R14        (OCP_BASE_ADDR + 0x30D8)
#define MP1_OCP_CPU_R15        (OCP_BASE_ADDR + 0x30DC)
#define MP1_OCP_CPU_R16        (OCP_BASE_ADDR + 0x30E0)

#define MP1_OCP_NCPU_R0        (OCP_BASE_ADDR + 0x3100)
#define MP1_OCP_NCPU_R1        (OCP_BASE_ADDR + 0x3104)
#define MP1_OCP_NCPU_R2        (OCP_BASE_ADDR + 0x3108)
#define MP1_OCP_NCPU_R3        (OCP_BASE_ADDR + 0x310C)
#define MP1_OCP_NCPU_R4        (OCP_BASE_ADDR + 0x3110)
#define MP1_OCP_NCPU_R5        (OCP_BASE_ADDR + 0x3114)
#define MP1_OCP_NCPU_R6        (OCP_BASE_ADDR + 0x3118)
#define MP1_OCP_NCPU_R7        (OCP_BASE_ADDR + 0x311C)
#define MP1_OCP_NCPU_R8        (OCP_BASE_ADDR + 0x3120)
#define MP1_OCP_CPU_ROFF       (OCP_BASE_ADDR + 0x308C)
#define MP1_OCP_NCPU_LKG       (OCP_BASE_ADDR + 0x3140)      //mp1_ocpncpulkg non-core leakage
#define MP1_OCP_CPU_LKG        (OCP_BASE_ADDR + 0x3144)      //mp1_ocpcpurlkg cpu leakage
 
#define MP1_OCP_FPIDTKI        (OCP_BASE_ADDR + 0x3148)
#define MP1_OCP_FREQ_CTRL      (OCP_BASE_ADDR + 0x314C)
#define MP1_OCP_FPIKP          (OCP_BASE_ADDR + 0x3150)

#define MP1_OCP_IDVFS_VACT     (OCP_BASE_ADDR + 0x318C)
#define MP1_OCP_IDVFS_VPRED    (OCP_BASE_ADDR + 0x3190)
#define MP1_OCP_IDVFS_FACT     (OCP_BASE_ADDR + 0x3194)
#define MP1_OCP_IDVFS_VREQMAX  (OCP_BASE_ADDR + 0x3198)
#define MP1_OCP_IDVFS_VACT_WR  (OCP_BASE_ADDR + 0x319C)
#define MP1_OCP_IDVFS_VPRED_WR (OCP_BASE_ADDR + 0x31A0)
#define MP1_OCP_IDVFS_FACT_WR  (OCP_BASE_ADDR + 0x31A4)
#define MP1_OCP_IDVFS_FREQMAX  (OCP_BASE_ADDR + 0x31A8)

#define MP1_OCP_CPU_POSTCTRL0  (OCP_BASE_ADDR + 0x3700)
#define MP1_OCP_CPU_POSTCTRL1  (OCP_BASE_ADDR + 0x3704)
#define MP1_OCP_CPU_POSTCTRL2  (OCP_BASE_ADDR + 0x3708)
#define MP1_OCP_CPU_POSTCTRL3  (OCP_BASE_ADDR + 0x370C)
#define MP1_OCP_CPU_POSTCTRL4  (OCP_BASE_ADDR + 0x3710)
#define MP1_OCP_CPU_POSTCTRL5  (OCP_BASE_ADDR + 0x3714)
#define MP1_OCP_CPU_POSTCTRL6  (OCP_BASE_ADDR + 0x3718)
#define MP1_OCP_CPU_POSTCTRL7  (OCP_BASE_ADDR + 0x371C)

#define MP1_OCP_NCPU_POSTCTRL0 (OCP_BASE_ADDR + 0x37A0)

#define MP1_OCP_OCWATGT        (OCP_BASE_ADDR + 0x3060)
#define MP1_OCP_VOLT_CTRL      (OCP_BASE_ADDR + 0x3070)
#define MP1_OCP_OCKP           (OCP_BASE_ADDR + 0x3154)
#define MP1_OCP_OCDTKI         (OCP_BASE_ADDR + 0x3158)
#define MP1_OCP_GENERAL_CTRL   (OCP_BASE_ADDR + 0x37FC)

#define MP1_OCP_DBG_IFCTRL     (OCP_BASE_ADDR + 0x3200)
#define MP1_OCP_DBG_IFCTRL1    (OCP_BASE_ADDR + 0x3204)
#define MP1_OCP_DBG_STAT       (OCP_BASE_ADDR + 0x3208)
#define MP1_OCP_DBG_ACT_L      (OCP_BASE_ADDR + 0x3250)
#define MP1_OCP_DBG_ACT_H      (OCP_BASE_ADDR + 0x3254)
#define MP1_OCP_DBG_LKG_L      (OCP_BASE_ADDR + 0x3260)
#define MP1_OCP_DBG_LKG_H      (OCP_BASE_ADDR + 0x3264)

#define MP1_OCP_LKGMAX         (OCP_BASE_ADDR + 0x3084)
#define MP1_OCP_OCCGMIN        (OCP_BASE_ADDR + 0x3088)
#define MP1_OCP_WAMAXACT       (OCP_BASE_ADDR + 0x3074)
#define MP1_OCP_WAMINACT       (OCP_BASE_ADDR + 0x3078)

#define MP1_OCP_IRQSTATE       (OCP_BASE_ADDR + 0x3000)
#define MP1_OCP_CAP_STATUS00   (OCP_BASE_ADDR + 0x3004)
#define MP1_OCP_CAP_STATUS01   (OCP_BASE_ADDR + 0x3008)
#define MP1_OCP_CAP_STATUS02   (OCP_BASE_ADDR + 0x300C)
#define MP1_OCP_CAP_STATUS03   (OCP_BASE_ADDR + 0x3010)
#define MP1_OCP_CAP_STATUS04   (OCP_BASE_ADDR + 0x3014)
#define MP1_OCP_CAP_STATUS05   (OCP_BASE_ADDR + 0x3018)
#define MP1_OCP_CAP_STATUS06   (OCP_BASE_ADDR + 0x301C)
#define MP1_OCP_CAP_STATUS07   (OCP_BASE_ADDR + 0x3020)
#define MP1_OCP_IRQCLR         (OCP_BASE_ADDR + 0x3048)
#define MP1_OCP_IRQEN          (OCP_BASE_ADDR + 0x304C)

#define MP1_OCP_CAP_HOLDOFF    (OCP_BASE_ADDR + 0x3050)
#define MP1_OCP_CAP_CTRL       (OCP_BASE_ADDR + 0x3054)

//for ALL
#define OCP_MA    (0x0)
#define OCP_MW    (0x1)

#define OCP_OCPI  (0x1)
#define OCP_FPI   (0x2)
#define OCP_ALL   (0x3)
  
#define  OCP_LL    (0)
#define  OCP_L     (1)

#define OCP_DISABLE    (0)
#define OCP_ENABLE     (1)
#define OCP_RISING     (1)
#define OCP_FALLING    (0)
#define OCP_CAP_IMM    (15)
#define OCP_CAP_IRQ0   (1)
#define OCP_CAP_IRQ1   (2)
#define OCP_CAP_EO     (3)
#define OCP_CAP_EI     (4)

#define MP0_MAX_FREQ_MHZ  (1400)
#define MP1_MAX_FREQ_MHZ  (1950)

//DREQ
#define BIG_SRAMLDO      (0x102222b0)
#define BIG_SRAMDREQ     (0x102222B8)
#define LITTLE_SRAMDREQ  (0x10001000)

/*******************************/
// OCP
extern int BigOCPConfig(int VOffInmV, int VStepInuV);
extern int BigOCPSetTarget(int OCPMode, int Target);
extern int BigOCPEnable(int OCPMode, int Units, int ClkPctMin, int FreqPctMin);
extern void BigOCPDisable(void);
extern int BigOCPIntLimit(int Select, int Limit);
extern int BigOCPIntEnDis(int Value1, int Value0);
extern int BigOCPIntClr(int Value1, int Value0);
extern int BigOCPCapture(int EnDis, int Edge, int Count, int Trig);
extern unsigned int BigOCPAvgPwrGet(unsigned int Count);
extern int BigOCPClkAvg(int EnDis, int CGAvgSel);
extern int LittleOCPConfig(int Cluster, int VOffInmV, int VStepInuV);
extern int LittleOCPSetTarget(int Cluster, int Target);
extern int LittleOCPEnable(int Cluster, int Units, int ClkPctMin);
extern int LittleOCPDisable(int Cluster);
extern int LittleOCPDVFSSet(int Cluster, int FreqMHz, int VoltInmV);
extern int LittleOCPIntLimit(int Cluster, int Select, int Limit);
extern int LittleOCPIntEnDis(int Cluster, int Value1, int Value0);
extern int LittleOCPIntClr(int Cluster, int Value1, int Value0);
extern int LittleOCPCapture(int Cluster, int EnDis, int Edge, int Count, int Trig);
extern int LittleOCPAvgPwr(int Cluster, int EnDis, int Count);
extern unsigned int LittleOCPAvgPwrGet(int Cluster);
//DREQ + SRAMLDO
extern int BigSRAMLDOEnable(int mVolts);
extern int BigDREQHWEn(int VthHi, int VthLo);
extern int BigDREQSWEn(int Value);
extern int BigDREQGet(void);
extern int LittleDREQSWEn(int EnDis);
extern int LittleDREQGet(void);

/************************************/

