/**
 * @file    mt_ocp_api.c
 * @brief   Driver for Over Current Protect
 *
 */
#include <debug.h>
#include "mt_ocp_api.h"
#include "mt_cpuxgpt.h"



typedef struct OCP_setting
{
	unsigned int addr;
	unsigned int value;
}OCP_SETTING_T;

static const OCP_SETTING_T BigOCP_setting_table[] =
{
	{OCPAPBCFG00, 0x00220100},
	{OCPAPBCFG01, 0x00000000},
	{OCPAPBCFG02, 0x00000000},
	{OCPAPBCFG03, 0x00000000},
	{OCPAPBCFG04, 0x00000777},
	{OCPAPBCFG05, 0x7FFFF000},
	{OCPAPBCFG06, 0x00000000},
	{OCPAPBCFG07, 0x0007FFFF},
	{OCPAPBCFG08, 0x00000000},
	{OCPAPBCFG09, 0x0028D333},
	{OCPAPBCFG10, 0x00000000},
	{OCPAPBCFG11, 0x00000000},
	{OCPAPBCFG12, 0x00000000},
	{OCPAPBCFG13, 0x00000000},
	{OCPAPBCFG14, 0x00090000}, /* TopRoff:CpuROff */
	{OCPAPBCFG15, 0x43B90000}, /* CpuR1:CpuR0 */
	{OCPAPBCFG16, 0x00001B17}, /* CpuR3:CpuR2 */
	{OCPAPBCFG17, 0x00000000},
	{OCPAPBCFG18, 0x00000000},
	{OCPAPBCFG19, 0x00000000},
	{OCPAPBCFG20, 0x065205B1},
	{OCPAPBCFG21, 0x04000051},
	{OCPAPBCFG22, 0x04000000},
	{OCPAPBCFG23, 0x07C00000},
	{OCPAPBCFG24, 0x0CB00270},
	{OCPAPBCFG25, 0x00000000},
	{OCPAPBCFG26, 0x00000160},
	{OCPAPBCFG27, 0x00000000},
	{OCPAPBCFG28, 0x00000000},

};

static const OCP_SETTING_T Cluster0_OCP_setting_table[] =
{
	{MP0_OCP_GENERAL_CTRL, 0x0},
	{MP0_OCP_CPU_R0 , 0x106E}, //1
	{MP0_OCP_CPU_R1 , 0x5639},
	{MP0_OCP_CPU_R2 , 0x040F},
	{MP0_OCP_CPU_R3 , 0x0C3F},
	{MP0_OCP_CPU_R4 , 0x080E},
	{MP0_OCP_CPU_R5 , 0x5DDB},
	{MP0_OCP_CPU_R6 , 0x0A53},
	{MP0_OCP_CPU_R7 , 0x0AF8},
	{MP0_OCP_CPU_R8 , 0x1488},
	{MP0_OCP_CPU_R9 , 0x38A4},
	{MP0_OCP_CPU_R10, 0x4AD6},
	{MP0_OCP_CPU_R11, 0x11C9},
	{MP0_OCP_CPU_R12, 0x23CF},
	{MP0_OCP_CPU_R13, 0x0BCD},
	{MP0_OCP_CPU_R14, 0x0A4B},
	{MP0_OCP_CPU_R15, 0x365B},
	{MP0_OCP_CPU_R16, 0x3AB0},
	{MP0_OCP_NCPU_R0, 0x4C8C},
	{MP0_OCP_NCPU_R1, 0x217D},
	{MP0_OCP_NCPU_R2, 0x3058},
	{MP0_OCP_NCPU_R3, 0x1820},
	{MP0_OCP_NCPU_R4, 0x1799},
	{MP0_OCP_NCPU_R5, 0x22B6},
	{MP0_OCP_NCPU_R6, 0x11F9},
	{MP0_OCP_NCPU_R7, 0x2373},
	{MP0_OCP_NCPU_R8, 0x158B}, //26
	{MP0_OCP_CPU_ROFF, 0x022E0000},
	{MP0_OCP_NCPU_LKG, 0x7009},
	{MP0_OCP_CPU_LKG, 0x36A1},
	{MP0_OCP_CPU_EVSHIFT0, 0x00000010},
	{MP0_OCP_CPU_EVSHIFT1, 0x00000100},
	{MP0_OCP_CPU_EVSHIFT2, 0x0},
	{MP0_OCP_CPU_POSTCTRL0, 0x65184},
	{MP0_OCP_CPU_POSTCTRL1, 0x10140413},
	{MP0_OCP_CPU_POSTCTRL2, 0x80086D2},
	{MP0_OCP_CPU_POSTCTRL3, 0x686440},
	{MP0_OCP_CPU_POSTCTRL4, 0x11040801},
	{MP0_OCP_CPU_POSTCTRL5, 0xAA8229},
	{MP0_OCP_CPU_POSTCTRL6, 0XA110},
	{MP0_OCP_CPU_POSTCTRL7, 0x8},
	{MP0_OCP_NCPU_POSTCTRL0, 0x32CB249},
	{MP0_OCP_NCPU_EVSHIFT0, 0x0},
	{MP0_OCP_NCPU_EVSHIFT1, 0x0},
	{MP0_OCP_CPU_EVSEL , 0x1FFFF},
	{MP0_OCP_NCPU_EVSEL, 0x1FF},
	{MP0_OCP_SCAL, 0x4D},
	{MP0_OCP_VOLT_CTRL, 0x19A000},
	{MP0_OCP_OCKP, 0x7800000},
	{MP0_OCP_OCDTKI, 0x3000F5},
	{MP0_MAF_CTRL, 0x0},
	{MP0_OCP_LKG_CTRL, 0x77777},
	{MP0_OCP_FPIDTKI  , 0x4},
	{MP0_OCP_FREQ_CTRL, 0x40},
	{MP0_OCP_FPIKP    , 0x4000000},
};

static const OCP_SETTING_T Cluster1_OCP_setting_table[] =
{
	{MP1_OCP_GENERAL_CTRL, 0x0},
	{MP1_OCP_CPU_R0 , 0x1986},  //1
	{MP1_OCP_CPU_R1 , 0x633D},
	{MP1_OCP_CPU_R2 , 0x0F78},
	{MP1_OCP_CPU_R3 , 0x110A},
	{MP1_OCP_CPU_R4 , 0x051B},
	{MP1_OCP_CPU_R5 , 0x79AD},
	{MP1_OCP_CPU_R6 , 0x1467},
	{MP1_OCP_CPU_R7 , 0x1721},
	{MP1_OCP_CPU_R8 , 0x1213},
	{MP1_OCP_CPU_R9 , 0x5A9B},
	{MP1_OCP_CPU_R10, 0x45F4},
	{MP1_OCP_CPU_R11, 0x1944},
	{MP1_OCP_CPU_R12, 0x3F9B},
	{MP1_OCP_CPU_R13, 0x0EBC},
	{MP1_OCP_CPU_R14, 0x2835},
	{MP1_OCP_CPU_R15, 0x54B1},
	{MP1_OCP_CPU_R16, 0x42CA},
	{MP1_OCP_NCPU_R0, 0x2969},
	{MP1_OCP_NCPU_R1, 0x200C},
	{MP1_OCP_NCPU_R2, 0x1CAB},
	{MP1_OCP_NCPU_R3, 0x3455},
	{MP1_OCP_NCPU_R4, 0x2719},
	{MP1_OCP_NCPU_R5, 0x1586},
	{MP1_OCP_NCPU_R6, 0x1B30},
	{MP1_OCP_NCPU_R7, 0x12DC},
	{MP1_OCP_NCPU_R8, 0x261C}, //26
	{MP1_OCP_CPU_ROFF, 0x038D0000},
	{MP1_OCP_NCPU_LKG, 0x78B5},
	{MP1_OCP_CPU_LKG, 0x5171},
	{MP1_OCP_CPU_EVSHIFT0, 0x00100020},
	{MP1_OCP_CPU_EVSHIFT1, 0x00000210},
	{MP1_OCP_CPU_EVSHIFT2, 0x1},
	{MP1_OCP_CPU_POSTCTRL0, 0x65556},
	{MP1_OCP_CPU_POSTCTRL1, 0x30340C3},
	{MP1_OCP_CPU_POSTCTRL2, 0x8040689},
	{MP1_OCP_CPU_POSTCTRL3, 0x2046080},
	{MP1_OCP_CPU_POSTCTRL4, 0x820C105},
	{MP1_OCP_CPU_POSTCTRL5, 0xAA9828},
	{MP1_OCP_CPU_POSTCTRL6, 0x8118},
	{MP1_OCP_CPU_POSTCTRL7, 0x8},
	{MP1_OCP_NCPU_POSTCTRL0, 0x3249659},
	{MP1_OCP_NCPU_EVSHIFT0, 0x20200222},
	{MP1_OCP_NCPU_EVSHIFT1, 0x0},
	{MP1_OCP_CPU_EVSEL , 0x1FFFF},
	{MP1_OCP_NCPU_EVSEL, 0x1FF},
	{MP1_OCP_SCAL, 0x4D},
	{MP1_OCP_VOLT_CTRL, 0x19A000},
	{MP1_OCP_OCKP, 0x7800000},
	{MP1_OCP_OCDTKI, 0x3000F5},
	{MP1_MAF_CTRL, 0x0},
	{MP1_OCP_LKG_CTRL, 0x77777},
	{MP1_OCP_FPIDTKI  , 0x4},
	{MP1_OCP_FREQ_CTRL, 0x40},
	{MP1_OCP_FPIKP    , 0x4000000},

};

void Write_Big_RCoef(int Address, int RCoef_Lo, int RCoef_Hi, int PowerCal)
{
unsigned int Temp;

Temp = (RCoef_Lo * PowerCal) >> 9;
Temp = (Temp << 16) + ((RCoef_Hi * PowerCal) >> 9);
ocp_write(Address, Temp);

#if HW_API_DEBUG_ON
	printf("RL:RH %x:%x %x\n", RCoef_Lo, RCoef_Hi, Temp);
#endif
}

void Write_Little_RCoef(int Address, int RCoef, int PowerCal)
{
unsigned int Temp;

Temp = (RCoef * PowerCal) >> 9;
ocp_write(Address, Temp);

}

static const unsigned short SRAMLDO_bit_map[] = { 500, 600, 700, 800, 900, 925, 950, 975, 1000, 1025, 1050, 1075, 1100, 1125, 1175};

static const unsigned short DREQ_bit_map[] = {  600, 861,  874,  887,  901,  914,  927,  940,  953,  967,  980,  993, 1006, 1020, 1033, 1046,
                                               1059, 1073, 1086, 1099, 1112, 1126, 1139, 1152, 1165, 1178, 1192, 1205, 1218, 1231, 1245, 1258,
											   1271, 1284, 1298, 1311, 1324, 1337, 1351, 1364, 1377, 1390, 1421, 1417, 1430, 1443, 1456, 1470,
											   1483, 1496, 1509, 1523, 1536, 1549, 1562, 1576, 1589, 1602, 1615, 1628, 1642, 1655, 1668, 1681};



//BIG CPU
int BigOCPConfig(int VOffInmV, int VStepInuV)
{
//1. Check Voff & VStep ranges. Exit/return -1 if out of range
//2. Write all fixed values of OCPAPBCFG
//3. Read LkgMonTRIM[3:0] values from eFuse for each leakage monitor
//4. If any 4-bit  value is all zeros, use  4’h7
//5. Write ALL Lkg TRIM values
//6. Read PowerCal[9:0] value from eFuse
//7. If all zeros, no change to original RCoef provided in SW
//8. Else calculate corrected RCoef’s
//9. Write calibrated (from PwrCalib) RCoef
//10. Calculate & write VOff & VStep

int TopLkgTrim, Cpu0LkgTrim, Cpu1LkgTrim, PowerCal;

	int i;
	for (i=0; i<sizeof(BigOCP_setting_table)/sizeof(OCP_SETTING_T); i++)
    {
		ocp_write(BigOCP_setting_table[i].addr, BigOCP_setting_table[i].value);
	}


//Calculate & write VOff & VStep
VOffInmV = (VOffInmV << 14)/1000;
VStepInuV = (VStepInuV << 14)/1000000;

ocp_write(OCPAPBCFG09, ((VStepInuV << 14) | VOffInmV));

// calculate corrected RCoef’s
PowerCal = ocp_read_field(PTP3_OD0, 15:6);

if (PowerCal != 0x0) {
	for (i = 14; i < 20; i++)
		Write_Big_RCoef(BigOCP_setting_table[i].addr, (BigOCP_setting_table[i].value >> 16), (BigOCP_setting_table[i].value & 0x0000FFFF), PowerCal);

}

//set TRIM
TopLkgTrim = ocp_read_field(PTP3_OD0, 19:16);
if (TopLkgTrim == 0x0)
	TopLkgTrim = 0x7;

Cpu0LkgTrim = ocp_read_field(PTP3_OD0, 23:20);
if (Cpu0LkgTrim == 0x0)
	Cpu0LkgTrim = 0x7;

Cpu1LkgTrim = ocp_read_field(PTP3_OD0, 27:24);
if (Cpu1LkgTrim == 0x0)
	Cpu1LkgTrim = 0x7;

ocp_write((OCPAPBCFG04), ((Cpu1LkgTrim << 8) | (Cpu0LkgTrim << 4) | TopLkgTrim));

#if ATF_LOG_ON
	printf("ocp: B CFG done\n");
#endif

return 0;

}

int BigOCPSetTarget(int OCPMode, int Target)
{
	int OCWATgt, FPIWATgt;

	if (OCPMode == OCP_ALL) {
	//Convert Target to OCWATgt & FPIWATgt
	OCWATgt = ((Target << 12)/1000) & 0x000FFFFF;
	FPIWATgt = ((((Target * 98)/100) << 12)/1000) & 0x000FFFFF;

	 ocp_write_field(OCPAPBCFG05, 31:12, OCWATgt);
	 ocp_write_field(OCPAPBCFG07, 19:0, FPIWATgt);

	} else if (OCPMode == OCP_FPI) {
	//Convert Target to FPIWATgt
	FPIWATgt = ((Target << 12)/1000) & 0x000FFFFF;
	ocp_write_field(OCPAPBCFG07, 19:0, FPIWATgt);

	} else if (OCPMode == OCP_OCPI) {
	//Convert Target to OCWATgt
	OCWATgt = ((Target << 12)/1000) & 0x000FFFFF;
	ocp_write_field(OCPAPBCFG05, 31:12, OCWATgt);

	}

#if ATF_LOG_ON
	printf("ocp: B M=%d T=%d\n", OCPMode, Target);
#endif

	return 0;

}

int BigOCPEnable(int OCPMode,  int Units, int ClkPctMin, int FreqPctMin)
{
//1. If parameters are invalid or out of range,  exit/return -1
//2. Read OCPIEn & FPIEn. If selected ones already enabled, exit/error -2
//3. Convert Target to OCWATgt & FPIWATgt
//    if Select=ALL, set FPIWATgt at 98% of Target
//4. Convert Units to OCWASel & FPIWASel
//5. Convert ClkPctMin to OCMin
//6. Convert FreqPctMin to FreqMin
//7. Write OCWASel, OCWATgt, & OCMin, FPIWASel, FPIWATgt, & FreqMin
//8. Write ALL LkgMonEn bits = 1
//9. Write ALL LkgMonInit = 1
//10. if Sleect=OCPI, write OCPIEn =1, if Select=FPI write FPIEn = 1, if Select=ALL write both = 1

int OCMin, Temp;

if (OCPMode == OCP_ALL) {

	//Read OCPIEn & FPIEn. If selected ones already enabled, exit/error -2
	if (ocp_read_field(OCPAPBCFG00,0:0) == 1) {
#if HW_API_RET_DEBUG_ON
	printf("OCP Error: OCPIEn already enabled.\n");
#endif
	return -2;
	}
	if (ocp_read_field(OCPAPBCFG00,1:1) == 1) {
#if HW_API_RET_DEBUG_ON
	printf("OCP Error: FPIEn already enabled.\n");
#endif

	return -2;
	}

	//Write FreqMin
	ocp_write(OCPAPBCFG08, (FreqPctMin << 12)/100);

	//Convert ClkPctMin to OCMin
	OCMin = ((ClkPctMin * 16) / 10000) - 1;
	OCMin = OCMin << 3;

	//Write OCMin, FPIWASel, OCWASel
	Temp = ocp_read_field(OCPAPBCFG05,31:12);
	Temp = (Temp << 12| (OCMin << 4) | (Units << 2) | Units);
	ocp_write(OCPAPBCFG05, Temp);

	// Enable LkgMonEn
	ocp_write_field(OCPAPBCFG00, 4:2, 0x7);

	//Enable LkgMonInit
	Temp = ocp_read(OCPAPBCFG25);
	ocp_write(OCPAPBCFG25, (Temp | 0xC01));

	//MAFActEn
	ocp_write_field(OCPAPBCFG00, 16:16, 0x1);
	//MAFPredEn
	ocp_write_field(OCPAPBCFG00, 20:20, 0x1);

	// Enable FPIEn , OCPIEn
	ocp_write_field(OCPAPBCFG00, 1:0, 0x3);


} else if (OCPMode == OCP_FPI) {

	//Read FPIEn. If selected ones already enabled, exit/error -2
	if (ocp_read_field(OCPAPBCFG00,1:1) == 1) {
#if HW_API_RET_DEBUG_ON
	printf("OCP Error: FPIEn already enabled.\n");
#endif

	return -2;
	}

	//Write FPIWASel, FreqMin
	ocp_write_field(OCPAPBCFG05, 3:2, Units);
	ocp_write(OCPAPBCFG08, (FreqPctMin << 12)/100);

	// Enable LkgMonEn
	ocp_write_field(OCPAPBCFG00, 4:2, 0x7);

	//Enable LkgMonInit
	Temp = ocp_read(OCPAPBCFG25);
	ocp_write(OCPAPBCFG25, (Temp | 0xC01));

	//MAFPredEn
	ocp_write_field(OCPAPBCFG00, 20:20, 0x1);
	//Enable FPIEn
	ocp_write_field(OCPAPBCFG00, 1:1, 0x1);

} else if (OCPMode == OCP_OCPI) {

	//Read OCPIEn. If selected ones already enabled, exit/error -2
	if (ocp_read_field(OCPAPBCFG00,0:0) == 1) {
#if HW_API_RET_DEBUG_ON
	printf("OCP Error: OCPIEn already enabled.\n");
#endif
	return -2;
	}
	//Convert ClkPctMin to OCMin
	OCMin = ((ClkPctMin * 16) / 10000) - 1;
	OCMin = OCMin << 3;

	//Write OCMin, OCWASel
	Temp = ocp_read_field(OCPAPBCFG05,31:12);
	Temp = (Temp << 12| (OCMin << 4) | Units);
	ocp_write(OCPAPBCFG05, Temp);

	//Enable LkgMonEn
	ocp_write_field(OCPAPBCFG00, 4:2, 0x7);

	//Enable LkgMonInit
	Temp = ocp_read(OCPAPBCFG25);
	ocp_write(OCPAPBCFG25, (Temp | 0xC01));

	//MAFActEn
	ocp_write_field(OCPAPBCFG00, 16:16, 0x1);
	// Enable OCPIEn
	ocp_write_field(OCPAPBCFG00, 0:0, 0x1);

}

#if ATF_LOG_ON
	printf("ocp: B En=%x\n", ocp_read(OCPAPBCFG00));
#endif

	return 0;
}

void BigOCPDisable(void)
{
//1. Read OCPAPBCFG[16].
//2. Simultaneously write OCPIEn=0, FPIEn =0, ALL LkgMonEn bits = 0 & bit[16]=0
//2. Write ALL LkgMonInit = 0 (IRQEn = 0)
//3. Re-configure OCPAPBCFG[16] to previous setting

// Workaround for enable -> disable -> enable bug
//1.	OCWATgt = OCPAPBCFG05[31:12]  = 20’h7FFFF
//2.	FPIWATgt = OCPAPBCFG07[19:0] = 20’h7FFFF
//3.	RCoef = OCPAPBCFG14~20 = 32’h00000000.
//4.	OCProtRate = OCPAPBCFG24[21:20] = 2’b00
//5.	MAFActEn=OCPAPBCFG00[16] = 1’b1, MAFPredEn=OCPAPBCFG00[20] = 1’b1
//6.	OCPIEn=OCPAPBCFG00[0] = 1’b1 & FPIEn=OCPAPBCFG00[1] = 1’b1
//7.	Run for at least 128*5 clock cycles
//8.	OCPIEn=OCPAPBCFG00[0] = 1’b0 & FPIEn=OCPAPBCFG00[1] = 1’b0
//9.	Return to previous settings.

int Temp;

// Disable LkgMonEn , FPIEn , OCPIEn
ocp_write_field(OCPAPBCFG00, 4:0, 0x0);

// LkgMonInit = 0
ocp_write_field(OCPAPBCFG25, 0:0, 0);
ocp_write_field(OCPAPBCFG25, 11:10, 0);

// Disable IRQEn
ocp_write(OCPAPBCFG02, 0x0);

// Workaround for enable -> disable -> enable bug
ocp_write_field(OCPAPBCFG05, 31:12, 0x7FFFF);
ocp_write_field(OCPAPBCFG07, 19:0, 0x7FFFF);

for (Temp = 14; Temp < 21; Temp++) {
		ocp_write(BigOCP_setting_table[Temp].addr, 0x0);
	}

ocp_write_field(OCPAPBCFG24, 21:20, 0x0);
ocp_write_field(OCPAPBCFG00, 16:16, 0x1);
ocp_write_field(OCPAPBCFG00, 20:20, 0x1);
ocp_write_field(OCPAPBCFG00, 1:0, 0x1);
udelay(2);
ocp_write_field(OCPAPBCFG00, 1:0, 0x0);

for (Temp=0; Temp < sizeof(BigOCP_setting_table)/sizeof(OCP_SETTING_T); Temp++) {
		ocp_write(BigOCP_setting_table[Temp].addr, BigOCP_setting_table[Temp].value);
	}

#if ATF_LOG_ON
	printf("ocp: B Dis=%x\n", ocp_read(OCPAPBCFG00));
#endif

}

int BigOCPIntLimit(int Select, int Limit)
{
//1. If parameters are invalid or out of range,  exit/return -1
//For Select: (TotMaxAct, TotMinAct, TotMaxPred, TotMinPred, LkgMax, ClkPctMin)
//2. Convert Limit to WAMaxAct, WAMinAct, WAMaxPred, WAMinPred, LkgMax, OCCGMin
//3. Write converted value

const int  TotMaxAct  =  0;
const int  TotMinAct  =  1;
const int  TotMaxPred =  2;
const int  TotMinPred =  3;
const int  TotLkgMax  =  4;
const int  ClkPctMin  =  5;

int WAMaxAct, WAMinAct, WAMaxPred, WAMinPred, LkgMax, OCCGMin, Temp;

if (Select == TotLkgMax) {

	LkgMax = ((Limit << 12)/1000) & 0x000FFFFF;
	Temp = ocp_read(OCPAPBCFG12);
	ocp_write(OCPAPBCFG12, (Temp & 0x0000FFFF)| (LkgMax << 16));

	Temp = ocp_read(OCPAPBCFG13);
	ocp_write(OCPAPBCFG13, (Temp & 0xF0)|(LkgMax >> 16));

#if HW_API_DEBUG_ON
	printf("ocp: X=%d LM=%x CFG12:13=%x:%x\n", Limit,LkgMax, ocp_read(OCPAPBCFG12),ocp_read(OCPAPBCFG13));
#endif

} else if (Select == ClkPctMin) {

	OCCGMin = ((Limit * 16)/10000) - 1;

	Temp = ocp_read(OCPAPBCFG13);
	ocp_write(OCPAPBCFG13, (Temp & 0x0F)| (OCCGMin << 4));
#if HW_API_DEBUG_ON
	printf("ocp: CP=%d CG=%x CFG13=%x\n", Limit, OCCGMin, ocp_read(OCPAPBCFG13));
#endif

} else if (Select == TotMaxAct) {

	WAMaxAct = ((Limit << 12)/1000) & 0x000FFFFF;
	Temp = ocp_read(OCPAPBCFG10);
	ocp_write(OCPAPBCFG10, (Temp & 0xFFF00000)| WAMaxAct);
#if HW_API_DEBUG_ON
	printf("ocp: X=%d WXA=%x CFG10=%x\n", Limit, WAMaxAct, ocp_read(OCPAPBCFG10));
#endif

} else if (Select == TotMinAct) {

	WAMinAct =  ((Limit << 12)/1000) & 0x000FFFFF;
	Temp = ocp_read(OCPAPBCFG10);
	ocp_write(OCPAPBCFG10, (Temp & 0x000FFFFF)| (WAMinAct << 20));

	Temp = ocp_read((OCPAPBCFG11));
	ocp_write(OCPAPBCFG11, (Temp & 0xFFFFFF00)| (WAMinAct >> 12));
#if HW_API_DEBUG_ON
	printf("ocp: X=%d WNA=%x CFG10:11=%x:%x\n", Limit, WAMinAct, ocp_read(OCPAPBCFG10), ocp_read(OCPAPBCFG11) );
#endif

} else if (Select == TotMaxPred) {

	WAMaxPred = ((Limit << 12)/1000) & 0x000FFFFF;
	Temp = ocp_read((OCPAPBCFG11));
	ocp_write(OCPAPBCFG11, (Temp & 0xF00000FF)| (WAMaxPred << 8));
#if HW_API_DEBUG_ON
	printf("ocp: X=%d WXP=%x BCFG11=%x\n", Limit, WAMaxPred, ocp_read(OCPAPBCFG11) );
#endif

} else if (Select == TotMinPred) {
	WAMinPred =  ((Limit << 12)/1000) & 0x000FFFFF;
	Temp = ocp_read(OCPAPBCFG11);
	ocp_write(OCPAPBCFG11, (Temp & 0x0FFFFFFF)| (WAMinPred << 28));

	Temp = ocp_read((OCPAPBCFG12));
	ocp_write(OCPAPBCFG12, (Temp & 0xFFFF0000)| (WAMinPred >> 4));
#if HW_API_DEBUG_ON
	printf("ocp: X=%d WNP=%x CFG11:12=%x:%x\n", Limit, WAMinPred, ocp_read(OCPAPBCFG11), ocp_read(OCPAPBCFG12));
#endif

}

return 0;

}

int BigOCPIntEnDis(int Value1, int Value0)
{
//1. Check Valid Value0 & Value1 (exit/return error -1 if invalid)
//2. Write 32 bit value IRQEn to enable selected interrupt sources

ocp_write(OCPAPBCFG02, ((Value1 << 16) | Value0));

	return 0;
}


int BigOCPIntClr(int Value1, int Value0)
{
//1. Check Valid Value0 & Value1 (exit/return error -1 if invalid)
//2. Write 32 bit value IRQClr to clear selected bits
//3. Then write all 0's

	ocp_write(OCPAPBCFG01, ((Value1 << 16) | Value0));
	ocp_write(OCPAPBCFG01, 0x00000000);
	return 0;
}


int BigOCPCapture(int EnDis, int Edge, int Count, int Trig)
{
//1.If parameters invalid, exit/return -1
//2. If EnDis = DISABLE, write CapTrigSel = 0, exit/return 0
//3. Convert Edge to CaptureEdge, Count to CaptureHoldOff, Trig to CaptureTrigSel
//4. Write CaptureEdge, CaptureHoldOff
//5. Write CaptureTrigSel

if (EnDis == OCP_DISABLE)
	ocp_write(OCPAPBCFG03, 0x0);
else if (EnDis == OCP_ENABLE) {
	ocp_write(OCPAPBCFG03, 0x0);                                     //3. set Trig = 0
	ocp_write(OCPAPBCFG03, ((Edge << 24) | Count));                  //4. Write CaptureEdge, CaptureHoldOff
	ocp_write(OCPAPBCFG03, ((Edge << 24) | (Trig << 20) | Count));   //5. Write CaptureTrigSel

}

 return 0;
//EnDis: OCP_ENABLE, OCP_DISABLE
//Edge: OCP_RISING, OCP_FALLING
//Count: 0 -> 1048575
//Trig: OCP_CAP_*, *=IMM, IRQ0, IRQ1, EO, EI

}


int BigOCPClkAvg(int EnDis, int CGAvgSel)
{
//1. Set CGAvgSel = OCPAPBCFG00[7:5]
//2. Set CGAvgEn = OCPAPBCFG00[8] = EnDis

if (EnDis == OCP_DISABLE)
	ocp_write_field(OCPAPBCFG00, 8:8, 0);
else if (EnDis == OCP_ENABLE) {
	ocp_write_field(OCPAPBCFG00, 7:5, CGAvgSel);
	ocp_write_field(OCPAPBCFG00, 8:8, 1);
	}

return 0;

}


unsigned int BigOCPAvgPwrGet(unsigned int Count)
{

unsigned int i, Valid, Temp, Avg;
unsigned long long Total;
Total = 0;
Valid = 0;
Temp = 0;
Avg = 0;

for (i = 0; i < Count; i++) {
	BigOCPCapture(1, 1, 0, 15);

	if (ocp_read_field(OCPAPBSTATUS01, 0:0) == 1) {
		Temp = (ocp_read_field(OCPAPBSTATUS03, 18:0) * 1000) >> 12;

		if (Temp < 8800) {
			Total = Total + Temp;
			Valid++;
			Avg = (unsigned int)(Total/Valid);
		}
	}
#if HW_API_DEBUG_ON
	ocp_info("ocp: big total_pwr=%d\n", Total);
#endif
}

return Avg;
}


//Little CPU
int LittleOCPConfig(int Cluster, int VOffInmV, int VStepInuV)
{
//For both clusters
//1. Check Voff & VStep ranges. Exit/return -1 if out of range
//2. Write all fixed values of OCPAPBCFG
//3. Read LkgMonTRIM[3:0] values from eFuse for each leakage monitor
//4. If any 4-bit  value is all zeros, use  4’h7
//5. Write ALL Lkg TRIM values
//6. Read PowerCal[9:0] value from eFuse
//7. If all zeros, no change to original RCoef provided in SW
//8. Else calculate corrected RCoef’s
//9. Write calibrated (from PwrCalib) RCoef
//10. Calculate & write VOff & VStep"

int TopLkgTrim, Cpu0LkgTrim, Cpu1LkgTrim, Cpu2LkgTrim, Cpu3LkgTrim, PowerCal;
int i;

//Calculate & write VOff & VStep
VOffInmV = (VOffInmV << 14)/1000;
VStepInuV = (VStepInuV << 14)/1000000;

if (Cluster == OCP_LL) {

	for (i = 0; i < sizeof(Cluster0_OCP_setting_table)/sizeof(OCP_SETTING_T); i++) {
		ocp_write(Cluster0_OCP_setting_table[i].addr, Cluster0_OCP_setting_table[i].value);
	}

ocp_write((MP0_OCP_VOLT_CTRL), ((VStepInuV << 14) | VOffInmV));

// calculate corrected RCoef’s
PowerCal = ocp_read_field(PTP3_OD2, 15:6); //MP0 = LL

if (PowerCal != 0x0) {
	for (i = 1; i < 27; i++) {
		Write_Little_RCoef(Cluster0_OCP_setting_table[i].addr, Cluster0_OCP_setting_table[i].value, PowerCal);
	}
}

//set TRIM
TopLkgTrim = ocp_read_field(PTP3_OD2, 19:16); //MP0
if (TopLkgTrim == 0x0)
	TopLkgTrim = 0x7;

Cpu0LkgTrim = ocp_read_field(PTP3_OD2, 23:20); //MP0
if (Cpu0LkgTrim == 0x0)
	Cpu0LkgTrim = 0x7;

Cpu1LkgTrim = ocp_read_field(PTP3_OD2, 27:24); //MP0
if (Cpu1LkgTrim == 0x0)
	Cpu1LkgTrim = 0x7;

Cpu2LkgTrim = ocp_read_field(PTP3_OD2, 31:28); //MP0
if (Cpu2LkgTrim == 0x0)
	Cpu2LkgTrim = 0x7;

Cpu3LkgTrim = ocp_read_field(PTP3_OD3, 31:28); //MP0
if (Cpu3LkgTrim == 0x0)
	Cpu3LkgTrim = 0x7;

ocp_write((MP0_OCP_LKG_CTRL), ((Cpu3LkgTrim << 16) | (Cpu2LkgTrim << 12) | (Cpu1LkgTrim << 8) | (Cpu0LkgTrim << 4) | TopLkgTrim));


} else if (Cluster == OCP_L) {

	for (i = 0; i < sizeof(Cluster1_OCP_setting_table)/sizeof(OCP_SETTING_T); i++) {
		ocp_write(Cluster1_OCP_setting_table[i].addr, Cluster1_OCP_setting_table[i].value);
	}


ocp_write((MP1_OCP_VOLT_CTRL), ((VStepInuV << 14) | VOffInmV));

PowerCal = ocp_read_field(PTP3_OD1, 15:6); //MP1 = L

if (PowerCal != 0x0) {
	for (i = 1; i < 27; i++) {
		Write_Little_RCoef(Cluster1_OCP_setting_table[i].addr, Cluster1_OCP_setting_table[i].value, PowerCal);
	}
}

TopLkgTrim = ocp_read_field(PTP3_OD1, 19:16); //MP1
if (TopLkgTrim == 0x0)
	TopLkgTrim = 0x7;

Cpu0LkgTrim = ocp_read_field(PTP3_OD1, 23:20); //MP1
if (Cpu0LkgTrim == 0x0)
	Cpu0LkgTrim = 0x7;

Cpu1LkgTrim = ocp_read_field(PTP3_OD1, 27:24); //MP1
if (Cpu1LkgTrim == 0x0)
	Cpu1LkgTrim = 0x7;

Cpu2LkgTrim = ocp_read_field(PTP3_OD1, 31:28); //MP1
if (Cpu2LkgTrim == 0x0)
	Cpu2LkgTrim = 0x7;

Cpu3LkgTrim = ocp_read_field(PTP3_OD3, 23:20); //MP1
if (Cpu3LkgTrim == 0x0)
	Cpu3LkgTrim = 0x7;

ocp_write((MP1_OCP_LKG_CTRL), ((Cpu3LkgTrim << 16) | (Cpu2LkgTrim << 12) | (Cpu1LkgTrim << 8) | (Cpu0LkgTrim << 4) | TopLkgTrim));

#if ATF_LOG_ON
	printf("ocp: CL%d CFG done.\n", Cluster);
#endif

}

return 0;

}


int LittleOCPSetTarget(int Cluster, int Target)
{
int OCWATgt;

if (Cluster == OCP_LL) {
	//Convert Target to OCWATgt
	OCWATgt = ((Target << 12)/1000) & 0x000FFFFF;
	ocp_write((MP0_OCP_OCWATGT), OCWATgt);

} else if (Cluster == OCP_L) {
	//Convert Target to OCWATgt
	OCWATgt = ((Target << 12)/1000) & 0x000FFFFF;
	ocp_write((MP1_OCP_OCWATGT), OCWATgt);

}

#if ATF_LOG_ON
	printf("ocp: CL%d T=%d\n", Cluster, Target);
#endif

return 0;
}


int LittleOCPEnable(int Cluster, int Units, int ClkPctMin)
{
//1. If Cluster is invalid, exit/return -1
//For selected Cluster
//2. Convert Target to OCWATgt  (exit/return error -2 if out of range)
//2. Convert Units to OCWASel (exit/return error -3 if invalid)
//3. Convert ClkPctMin to OCMin (exit/return error -4 if out of range)
//5. Write OCWASel, OCWATgt, & OCMin
//6. Write ALL LkgMonEn bits = 1
//7. Write ALL LkgMonInit = 1
//8. Write OCPIEn =1

int OCMin;

if (Cluster == OCP_LL) {

	// Convert ClkPctMin to OCMin
	OCMin = ((ClkPctMin * 16) / 10000) - 1;
	OCMin = OCMin << 3;

	ocp_write((MP0_OCP_OC_CTRL), ((OCMin << 4) | Units));
	//ocp_write((MP0_OCP_OC_CTRL), 0x1);

	//Enable LkgMonEn
	ocp_write(MP0_OCP_ENABLE, 0x7C);

	//Enable LkgMoninit
	ocp_write_field(MP0_OCP_NCPU_LKGMON,0:0, 1);
	ocp_write_field(MP0_OCP_CPU_LKGMON,3:0, 0xF);

	// Enable OCPIEn, for lower power
	//ocp_write(MP0_OCP_ENABLE, 0x7D);

#if ATF_LOG_ON
	printf("ocp: CL0 En=%x\n", ocp_read(MP0_OCP_ENABLE));
#endif

//0	0 	ocpien
//1	1	fpien  (不開)
//2	2	toplkgenableout
//6	3	cpulkgenable
//15 7	RESV

} else if(Cluster == OCP_L) {

	// Convert ClkPctMin to OCMin
	OCMin = ((ClkPctMin * 16) / 10000) - 1;
	OCMin = OCMin << 3;

	ocp_write((MP1_OCP_OC_CTRL), ((OCMin << 4) | Units));

	//Enable LkgMonEn
	ocp_write(MP1_OCP_ENABLE, 0x7C);

	//Enable LkgMoninit
	ocp_write_field(MP1_OCP_NCPU_LKGMON,0:0, 1);
	ocp_write_field(MP1_OCP_CPU_LKGMON,3:0, 0xF);

	//Enable LkgMonEn & OCPIEn, for lower power
	//ocp_write((MP1_OCP_ENABLE), 0x7D);

#if ATF_LOG_ON
	 printf("ocp: CL1 En=%x\n", ocp_read(MP1_OCP_ENABLE));
#endif
}

return 0;
}

int LittleOCPDisable(int Cluster)
{
//1. If Cluster is invalid, exit/return -1
//For selected Cluster
//2. Simultaneously write OCPIEn=0, ALL LkgMonEn bits = 0
//3. Write ALL LkgMonInit = 0

//For flushing, L=mp1, LL=mp0
//OCWATgt = mp0_ocpocwatgt[19:0]
//FPIWATgt(L/LL no need to update)
//RCoeff = mp0_ocpcpuroff[31:0], mp0_ocpcpur0[15:0] ~ mp0_ocpncpu8[15:0], mp0_ocpncpulkg, mp0_ocp_cpurlkg
//OCProtRate = mp0_ocpocdtki[21:20]
//MAFActEn = mp0_maf_ctrl[0], MAFPredEn(L/LL no need to update)
//OCPIEn = mp0_ocpenable[0],  FPIEn(L/LL no need to update)
//Run for at least 128*5 clock cycles
//OCPIEn = mp0_ocpenable[0],
//Return to previous settings.

int i;

if (Cluster == OCP_LL) {
	// Disable LkgMonEn & OCPIEn
	ocp_write(MP0_OCP_ENABLE, 0x0);

	// LkgMonInit = 0
	ocp_write_field(MP0_OCP_NCPU_LKGMON,0:0,0);
	ocp_write_field(MP0_OCP_CPU_LKGMON,3:0,0);

	//Disable IRQEn
	ocp_write((MP0_OCP_IRQEN), 0x00);

	// Workaround for enable -> disable -> enable bug
	ocp_write_field(MP0_OCP_OCWATGT, 19:0, 0x7FFFF);

	for (i = 1; i < 30; i++)
		ocp_write(Cluster0_OCP_setting_table[i].addr, 0x0);

	ocp_write_field(MP0_OCP_OCDTKI, 21:20, 0x0);
	ocp_write_field(MP0_MAF_CTRL, 0:0, 0x1);
	ocp_write_field(MP0_OCP_ENABLE, 0:0, 0x1);
	udelay(2);
	ocp_write_field(MP0_OCP_ENABLE, 0:0, 0x0);

	for (i=0; i < sizeof(Cluster0_OCP_setting_table)/sizeof(OCP_SETTING_T); i++)
		ocp_write(Cluster0_OCP_setting_table[i].addr, Cluster0_OCP_setting_table[i].value);

#if ATF_LOG_ON
	printf("ocp: CL0 Dis=%x\n", ocp_read(MP0_OCP_ENABLE));
#endif

} else if (Cluster == OCP_L) {
	// Disable LkgMonEn & OCPIEn
	ocp_write(MP1_OCP_ENABLE, 0x0);

	// LkgMonInit = 0
	ocp_write_field(MP1_OCP_NCPU_LKGMON,0:0,0);
	ocp_write_field(MP1_OCP_CPU_LKGMON,3:0,0);
	//Disable IRQEn
	ocp_write(MP1_OCP_IRQEN, 0x00);

	// Workaround for enable -> disable -> enable bug
	ocp_write_field(MP1_OCP_OCWATGT, 19:0, 0x7FFFF);

	for (i = 1; i < 30; i++)
		ocp_write(Cluster1_OCP_setting_table[i].addr, 0x0);

	ocp_write_field(MP1_OCP_OCDTKI, 21:20, 0x0);
	ocp_write_field(MP1_MAF_CTRL, 0:0, 0x1);
	ocp_write_field(MP1_OCP_ENABLE, 0:0, 0x1);
	udelay(2);
	ocp_write_field(MP1_OCP_ENABLE, 0:0, 0x0);

	for (i=0; i < sizeof(Cluster1_OCP_setting_table)/sizeof(OCP_SETTING_T); i++)
		ocp_write(Cluster1_OCP_setting_table[i].addr, Cluster1_OCP_setting_table[i].value);

#if ATF_LOG_ON
	printf("ocp: CL1 Dis=%x\n", ocp_read(MP1_OCP_ENABLE));
#endif

}

return 0;

}


void LittleOCPDVFSSet_togle(int addr)
{
	ocp_write(addr, 0x1);	//togle write signal step 1
	ocp_read(addr);			//togle write signal step 2
	ocp_write(addr, 0x0);	//togle write signal step 3
	ocp_read(addr);			//togle write signal step 4
}

int LittleOCPDVFSSet(int Cluster, int FreqMHz, int VoltInmV)
{
//For Cluster:
//1. Check if values are in range, exit/return -1 if out of range
//2. Write FAct & VAct"

int Temp, Voffset, Vstep;

if (Cluster == OCP_LL) {
	// Write VAct
	Temp = ocp_read((MP0_OCP_VOLT_CTRL));
	Voffset = Temp & 0x00003FFF;
	Vstep = (Temp & 0x0FFFC000) >> 14;
	Temp = (VoltInmV << 14)/1000;
	ocp_write(MP0_OCP_IDVFS_VACT, (Temp - Voffset)/Vstep);

	LittleOCPDVFSSet_togle(MP0_OCP_IDVFS_VACT_WR);

	// Write FAct
	Temp = FreqMHz << 12;
	ocp_write(MP0_OCP_IDVFS_FACT, (Temp/MP0_MAX_FREQ_MHZ)* 100); // percentage of MAX freqency

	LittleOCPDVFSSet_togle(MP0_OCP_IDVFS_FACT_WR);

} else if (Cluster == OCP_L) {
	// Write VAct
	Temp = ocp_read((MP1_OCP_VOLT_CTRL));
	Voffset = Temp & 0x00003FFF;
	Vstep = (Temp & 0x0FFFC000) >> 14;
	Temp = (VoltInmV << 14)/1000;
	ocp_write(MP1_OCP_IDVFS_VACT, (Temp - Voffset)/Vstep);

	LittleOCPDVFSSet_togle(MP1_OCP_IDVFS_VACT_WR);

	// Write FAct
	Temp = FreqMHz << 12;
	ocp_write(MP1_OCP_IDVFS_FACT, (Temp/MP1_MAX_FREQ_MHZ)* 100); // percentage of MAX freqency

	LittleOCPDVFSSet_togle(MP1_OCP_IDVFS_FACT_WR);

	}

return 0;
}

int LittleOCPIntLimit(int Cluster, int Select, int Limit)
{
//For Cluster:
//1. Convert Limit to WAMaxAct, WAMinAct, LkgMax, OCCGMin (exit/return error -1 if out of range)
//2. Write converted value

const int  TotMaxAct  =  0;
const int  TotMinAct  =  1;
const int  TotLkgMax  =  4;
const int  ClkPctMin  =  5;

int WAMaxAct, WAMinAct, LkgMax, OCCGMin;

if (Cluster == OCP_LL) {
	if (Select == TotLkgMax) {

		LkgMax =  ((Limit << 12)/1000) & 0x000FFFFF;
		ocp_write(MP0_OCP_LKGMAX, LkgMax);

	} else if(Select == ClkPctMin) {

		OCCGMin = ((Limit * 16)/10000) - 1;
		ocp_write(MP0_OCP_OCCGMIN, OCCGMin);

	} else if(Select == TotMaxAct) {
		WAMaxAct = ((Limit << 12)/1000) & 0x000FFFFF;
		ocp_write(MP0_OCP_WAMAXACT, WAMaxAct);

	} else if (Select == TotMinAct) {

		WAMinAct = ((Limit << 12)/1000) & 0x000FFFFF;
		ocp_write(MP0_OCP_WAMINACT, WAMinAct);

	}
} else if (Cluster == OCP_L) {
	if (Select == TotLkgMax) {
		LkgMax = ((Limit << 12)/1000) & 0x000FFFFF;
		ocp_write(MP1_OCP_LKGMAX, LkgMax);

	} else if (Select == ClkPctMin) {

		OCCGMin = ((Limit * 16)/10000) - 1;
		ocp_write(MP1_OCP_OCCGMIN, OCCGMin);

	} else if (Select == TotMaxAct) {

		WAMaxAct = ((Limit << 12)/1000) & 0x000FFFFF;
		ocp_write(MP1_OCP_WAMAXACT, WAMaxAct);


	} else if (Select == TotMinAct) {

		WAMinAct = ((Limit << 12)/1000) & 0x000FFFFF;
		ocp_write(MP1_OCP_WAMINACT, WAMinAct);
	}
}

return 0;
}

int LittleOCPIntEnDis(int Cluster, int Value1, int Value0)
{
//For Cluster:
//1. Check Valid Value0 & Value1 (exit/return error -1 if invalid)
//2. Write 32 bit value IRQEn to enable selected interrupt sources

if (Cluster == OCP_LL)
	ocp_write(MP0_OCP_IRQEN, ((Value1 << 16) | Value0));
else if (Cluster == OCP_L)
	ocp_write(MP1_OCP_IRQEN, ((Value1 << 16) | Value0));

return 0;
}


int LittleOCPIntClr(int Cluster, int Value1, int Value0)
{

if (Cluster == OCP_LL) {

	ocp_write(MP0_OCP_IRQCLR, ((Value1 << 16) | Value0));
	ocp_write(MP0_OCP_IRQCLR, 0x00000000);

} else if (Cluster == OCP_L) {

	ocp_write(MP1_OCP_IRQCLR, ((Value1 << 16) | Value0));
	ocp_write(MP1_OCP_IRQCLR, 0x00000000);

}

return 0;
}


void LittleOCPCapture_Trig(int CTRL, int HOLDOFF, int Count, int Edge, int Trig)
{
	ocp_write(CTRL, 0x0);					//5. set Trig = 0
	ocp_write(HOLDOFF, Count);				//6. Write CaptureHoldOff
	ocp_write(CTRL, (Edge << 4));			//6. Write CaptureEdge
	ocp_write(CTRL, (Edge << 4)| Trig);		//7. Write CaptureTrigSel
}

int LittleOCPCapture(int Cluster, int EnDis, int Edge, int Count, int Trig)
{
//For Cluster:
//1.If EnDis invalid, exit/return -1
//2. If EnDis = DISABLE, write CapTrigSel = 0, exit/return 0
//3. Convert Edge to CaptureEdge (exit/return error -1 if invalid)
//4. Convert Count to CaptureHoldOff (exit/return error -2 if out of range)
//5. Convert Trig to CaptureTrigSel (exit/return error -3 if invalid)
//6. Write CaptureEdge, CaptureHoldOff
//7. Write CaptureTrigSel

if (Cluster == OCP_LL) {

	if (EnDis == OCP_DISABLE)
		ocp_write((MP0_OCP_CAP_CTRL), 0x0);
	else if (EnDis == OCP_ENABLE)
		LittleOCPCapture_Trig(MP0_OCP_CAP_CTRL, MP0_OCP_CAP_HOLDOFF, Count, Edge, Trig);

} else if (Cluster == OCP_L) {

	if( EnDis == OCP_DISABLE)
		ocp_write((MP1_OCP_CAP_CTRL), 0x0);
	else if (EnDis == OCP_ENABLE)
		LittleOCPCapture_Trig(MP1_OCP_CAP_CTRL, MP1_OCP_CAP_HOLDOFF, Count, Edge, Trig);
}

return 0;
}



/* lower power for BU */
int LittleOCPAvgPwr(int Cluster, int EnDis, int Count)
{

if (Cluster == OCP_LL) {
	ocp_write(MP0_OCP_GENERAL_CTRL, 0x6);
	ocp_write_field(MP0_OCP_ENABLE, 0:0, 1);
	ocp_write_field(MP0_OCP_DBG_IFCTRL, 1:1, EnDis);
	ocp_write_field(MP0_OCP_DBG_IFCTRL1, 21:0, Count);
	ocp_write_field(MP0_OCP_DBG_STAT, 0:0, ~ocp_read_field(MP0_OCP_DBG_STAT, 0:0));
	}
else if (Cluster == OCP_L) {
	ocp_write(MP1_OCP_GENERAL_CTRL, 0x6);
	ocp_write_field(MP1_OCP_ENABLE, 0:0, 1);
	ocp_write_field(MP1_OCP_DBG_IFCTRL, 1:1, EnDis);
	ocp_write_field(MP1_OCP_DBG_IFCTRL1, 21:0, Count);
	ocp_write_field(MP1_OCP_DBG_STAT, 0:0, ~ocp_read_field(MP1_OCP_DBG_STAT, 0:0));
}

return 0;
}

/* lower power for BU */
unsigned int LittleOCPAvgPwrGet(int Cluster)
{

unsigned long long AvgAct;
AvgAct = 0;

if (Cluster == OCP_LL) {
	if (ocp_read_field(MP0_OCP_DBG_STAT, 31:31) == 1) {
		AvgAct = ((((((unsigned long long)ocp_read(MP0_OCP_DBG_ACT_H) << 32) +
			(unsigned long long)ocp_read(MP0_OCP_DBG_ACT_L)) * 32) /
			(unsigned long long)ocp_read_field(MP0_OCP_DBG_IFCTRL1, 21:0)) * 1000) >> 12;

		ocp_write_field(MP0_OCP_ENABLE, 0:0, 0);
		ocp_write(MP0_OCP_GENERAL_CTRL, 0x0);
	}

} else if (Cluster == OCP_L) {
	if (ocp_read_field(MP1_OCP_DBG_STAT, 31:31) == 1) {
		AvgAct = ((((((unsigned long long)ocp_read(MP1_OCP_DBG_ACT_H) << 32) +
			(unsigned long long)ocp_read(MP1_OCP_DBG_ACT_L)) * 32) /
			(unsigned long long)ocp_read_field(MP1_OCP_DBG_IFCTRL1, 21:0)) * 1000) >> 12;

		ocp_write_field(MP1_OCP_ENABLE, 0:0, 0);
		ocp_write(MP1_OCP_GENERAL_CTRL, 0x0);
	}
}

return (unsigned int)AvgAct;
}


//Cluster 2 DREQ

int BigSRAMLDOEnable(int mVolts)
{
return 0;
}


int BigDREQHWEn(int VthHi, int VthLo)
{
//1.Convert inputs in mVolts to 6 bit value VthLo & VthHi
//2. Exit/return -1 if VthHi < VthLo
//3. exit/return -2 if either out of range
//4. Set dreq_vthhi & dreq_vthlo
//5. Set dreq_hw_en = 1 to enable the circuit
//6. Set dreq_mode = 1 to enable HW control
int dreq_vthhi=0, dreq_vthlo=0;
int i;

	for (i = 0; i < 64; i++)
	{
		if(VthHi > DREQ_bit_map[i])
			dreq_vthhi = i;

		if(VthLo > DREQ_bit_map[i])
			dreq_vthlo = i;
	}

	ocp_write((BIG_SRAMDREQ), (dreq_vthlo << 16)|(dreq_vthhi << 8));
	ocp_write_field(BIG_SRAMDREQ, 0:0, 1);
	ocp_write_field(BIG_SRAMDREQ, 2:2, 1);

	return 0;

}


int BigDREQSWEn(int Value)
{
//1. Set dreq_sw_en = Value (exit/return -1 if invalid value)
//2. Set dreq_mode = 0  //enable SW control
//3. Set dreq_hw_en = 0  //disable the DREQ circuit

	ocp_write_field(BIG_SRAMDREQ, 1:1, Value);
	ocp_write_field(BIG_SRAMDREQ, 2:2, 0);
	ocp_write_field(BIG_SRAMDREQ, 0:0, 0);

	return 0;
}

int BigDREQGet(void)
{
	return ocp_read_field(OCPAPBSTATUS07,0:0);
}


//Cluster 0,1 DREQ
int LittleDREQSWEn(int EnDis)
{
//1. If enable, set 0x1000_1000 to 4'b1111 when CA7_DVFS & CA7_DVDDRAM both at highest voltage
//2. If disable, set 0x1000_1000 to 4'b0000 when one of them intend to leave highest voltage"

	if (EnDis == 1)
		ocp_write_field(LITTLE_SRAMDREQ, 3:0, 0xF);
	else if (EnDis == 0)
		ocp_write_field(LITTLE_SRAMDREQ, 3:0, 0);

	return 0;
}

int LittleDREQGet(void)
{
	return ocp_read(LITTLE_SRAMDREQ);
}




