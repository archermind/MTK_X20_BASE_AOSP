#ifndef _EMI_DRV_H_
#define _EMI_DRV_H_

/* EMI Memory Protect Unit */
#define EMI_MPUA		(EMI_MPU_BASE+0x0160)
#define EMI_MPUB		(EMI_MPU_BASE+0x0168)
#define EMI_MPUC		(EMI_MPU_BASE+0x0170)
#define EMI_MPUD		(EMI_MPU_BASE+0x0178)
#define EMI_MPUE		(EMI_MPU_BASE+0x0180)
#define EMI_MPUF		(EMI_MPU_BASE+0x0188)
#define EMI_MPUG		(EMI_MPU_BASE+0x0190)
#define EMI_MPUH		(EMI_MPU_BASE+0x0198)
#define EMI_MPUA2		(EMI_MPU_BASE+0x0260)
#define EMI_MPUB2		(EMI_MPU_BASE+0x0268)
#define EMI_MPUC2		(EMI_MPU_BASE+0x0270)
#define EMI_MPUD2		(EMI_MPU_BASE+0x0278)
#define EMI_MPUE2		(EMI_MPU_BASE+0x0280)
#define EMI_MPUF2		(EMI_MPU_BASE+0x0288)
#define EMI_MPUG2		(EMI_MPU_BASE+0x0290)
#define EMI_MPUH2		(EMI_MPU_BASE+0x0298)
#define EMI_MPUA3		(EMI_MPU_BASE+0x0360)
#define EMI_MPUB3		(EMI_MPU_BASE+0x0368)
#define EMI_MPUC3		(EMI_MPU_BASE+0x0370)
#define EMI_MPUD3		(EMI_MPU_BASE+0x0378)
#define EMI_MPUE3		(EMI_MPU_BASE+0x0380)
#define EMI_MPUF3		(EMI_MPU_BASE+0x0388)
#define EMI_MPUG3		(EMI_MPU_BASE+0x0390)
#define EMI_MPUH3		(EMI_MPU_BASE+0x0398)

#define EMI_MPUI		(EMI_MPU_BASE+0x01A0)
#define EMI_MPUI_2ND	(EMI_MPU_BASE+0x01A4)
#define EMI_MPUJ		(EMI_MPU_BASE+0x01A8)
#define EMI_MPUJ_2ND	(EMI_MPU_BASE+0x01AC)
#define EMI_MPUK		(EMI_MPU_BASE+0x01B0)
#define EMI_MPUK_2ND	(EMI_MPU_BASE+0x01B4)
#define EMI_MPUL		(EMI_MPU_BASE+0x01B8)
#define EMI_MPUL_2ND	(EMI_MPU_BASE+0x01BC)
#define EMI_MPUI2	   	(EMI_MPU_BASE+0x02A0)
#define EMI_MPUI2_2ND	(EMI_MPU_BASE+0x02A4)
#define EMI_MPUJ2		(EMI_MPU_BASE+0x02A8)
#define EMI_MPUJ2_2ND	(EMI_MPU_BASE+0x02AC)
#define EMI_MPUK2		(EMI_MPU_BASE+0x02B0)
#define EMI_MPUK2_2ND	(EMI_MPU_BASE+0x02B4)
#define EMI_MPUL2		(EMI_MPU_BASE+0x02B8)
#define EMI_MPUL2_2ND	(EMI_MPU_BASE+0x02BC)
#define EMI_MPUI3		(EMI_MPU_BASE+0x03A0)
#define EMI_MPUI3_2ND	(EMI_MPU_BASE+0x03A4)
#define EMI_MPUJ3		(EMI_MPU_BASE+0x03A8)
#define EMI_MPUJ3_2ND	(EMI_MPU_BASE+0x03AC)
#define EMI_MPUK3		(EMI_MPU_BASE+0x03B0)
#define EMI_MPUK3_2ND	(EMI_MPU_BASE+0x03B4)
#define EMI_MPUL3		(EMI_MPU_BASE+0x03B8)
#define EMI_MPUL3_2ND	(EMI_MPU_BASE+0x03BC)

#define EMI_MPUM		(EMI_MPU_BASE+0x01C0)
#define EMI_MPUN		(EMI_MPU_BASE+0x01C8)
#define EMI_MPUO 		(EMI_MPU_BASE+0x01D0)
#define EMI_MPUU		(EMI_MPU_BASE+0x0200)
#define EMI_MPUM2		(EMI_MPU_BASE+0x02C0)
#define EMI_MPUN2		(EMI_MPU_BASE+0x02C8)
#define EMI_MPUO2		(EMI_MPU_BASE+0x02D0)
#define EMI_MPUU2		(EMI_MPU_BASE+0x0300)

/* EMI memory protection align 64K */
#define EMI_MPU_ALIGNMENT   0x10000

#define NO_PROTECTION       0
#define SEC_RW              1
#define SEC_RW_NSEC_R       2
#define SEC_RW_NSEC_W       3
#define SEC_R_NSEC_R        4
#define FORBIDDEN           5

#define LOCK                1
#define UNLOCK              0
#define EMIMPU_DX_SEC_BIT   30

#define EMIMPU_DOMAIN_NUM  8
#define EMIMPU_REGION_NUM  24

extern uint32_t sip_emimpu_set_region_protection(unsigned long long start, unsigned long long end, unsigned int region_permission);
extern uint64_t sip_emimpu_write(unsigned int offset, unsigned int reg_value);
extern uint32_t sip_emimpu_read(unsigned int offset);
extern void emimpu_setup(void);

#endif
