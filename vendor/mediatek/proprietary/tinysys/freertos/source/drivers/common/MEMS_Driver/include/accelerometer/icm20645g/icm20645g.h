#ifndef __ICM20645G_H__
#define __ICM20645G_H__

#include "cust_acc.h"
#include "hwsen.h"
#include "mpe_cm4_API.h"
#include "typedefs.h"



#define ICM20645_I2C_ADDRESS_8BIT 0xD0
struct scale_factor {
    unsigned char  whole;
    unsigned char  fraction;
};

struct data_resolution {
    struct scale_factor scalefactor;
    int                 sensitivity;
};
typedef enum {
    ICM20645_TRC_RAWDATA = 0x02,
    ICM20645_TRC_CALI    = 0X08,
    ICM20645_TRC_INFO    = 0X10,
} ICM20645_TRC;

#define ACCEL_AXIS_X            0
#define ACCEL_AXIS_Y            1
#define ACCEL_AXIS_Z            2
#define ACCEL_AXES_NUM          3
#define ICM20645_DATA_LEN       6

struct acc_fifo {
    unsigned int fifo_enable;
    unsigned int hw_rate;
    UINT32 ap_delay;
    mtk_int8 TB_PERCENT;
    UINT64 fifo_timestamp;
};

struct icm20645g_data {
    struct acc_hw                   *hw;
    struct sensor_driver_convert    cvt;
    /*misc*/
    struct data_resolution *reso;
    int                             cali_sw[ACCEL_AXES_NUM + 1];
    /*data*/
    unsigned char                   offset[ACCEL_AXES_NUM + 1]; /*+1: for 4-byte alignment*/
    unsigned char                   i2c_addr;
    int                             trace;
    unsigned int            use_in_factory;
    struct acc_fifo        g_fifo_param;
};

#define G_ICM20645_TIMEBASE_PLL           0x28

#define ICM20645_I2C_SLAVE_ADDR         0xD0


#define REG_BANK_SEL                    0x7F
#define BANK_SEL_0                      0x00
#define BANK_SEL_1                      0x10
#define BANK_SEL_2                      0x20
#define BANK_SEL_3                      0x30


/* ICM20645 Register Map  (Please refer to ICM20645 Specifications) */
#define ICM20645_REG_DEVID              0x00
#define ICM20645_REG_LP_CONFIG          0x05
#define ICM20645_PWR_MGMT_1             0x06
#define ICM20645_PWR_MGMT_2             0x07
#define G_ICM20645_REG_FIFO_R_W         0x72
#define G_ICM20645_REG_FIFO_COUNTL        0x71
#define G_ICM20645_REG_FIFO_EN_2            0x67
#define G_ICM20645_REG_FIFO_RST             0x68
#define G_ICM20645_REG_FIFO_COUNTH        0x70
#define G_ICM20645_REG_USER_CTRL            0x03




#define ICM20645_REG_SAMRT_DIV1         0x10
#define ICM20645_REG_SAMRT_DIV2         0x11

#define BIT_ACC_LP_EN                   0x20
#define BIT_ACC_I2C_MST                 0x40

#define BIT_LP_EN                       0x20
#define BIT_CLK_PLL                     0x01
#define BIT_TEMP_DIS                    (1<<3)

#define BIT_PWR_ACCEL_STBY              0x38

#define ICM20645_REG_INT_ENABLE         0x11
#define ICM20645_ACC_CONFIG             0x14
#define ICM20645_ACC_CONFIG_2           0x15
#define ACCEL_AVGCFG_1_4X                  0
#define ACCEL_AVGCFG_8X                    1
#define ACCEL_AVGCFG_16X                   2
#define ACCEL_AVGCFG_32X                   3

#define ACCEL_FCHOICE                      1
#define ACCEL_DLPFCFG                   (7<<3)
#define ACCEL_AVG_SAMPLE                (0x3)


#define ICM20645_REG_DATAX0             0x2d
#define ICM20645_REG_DATAY0             0x2f
#define ICM20645_REG_DATAZ0             0x31

#define ICM20645_RANGE_2G               (0x00 << 1)
#define ICM20645_RANGE_4G               (0x01 << 1)
#define ICM20645_RANGE_8G               (0x02 << 1)
#define ICM20645_RANGE_16G              (0x03 << 1)



#define ICM20645_SLEEP                  0x40    //enable low power sleep mode
#define ICM20645_DEV_RESET              0x80

#define ICM20645_SUCCESS                0
#define ICM20645_ERR_I2C                -1
#define ICM20645_ERR_STATUS             -2
int icm20645g_sensor_init(void);

#endif
