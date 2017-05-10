#ifndef __ICM20645GY_H__
#define __ICM20645GY_H__

#include "cust_gyro.h"
#include "hwsen.h"
#include "typedefs.h"
#include "mpe_cm4_API.h"

#define GYRO_AXIS_X             0
#define GYRO_AXIS_Y             1
#define GYRO_AXIS_Z             2
#define GYRO_AXES_NUM        3
#define ICM20645_DATA_LEN       6

struct data_resolution {
    float   sensitivity;
};
struct gyro_fifo {
    unsigned int fifo_enable;
    unsigned int hw_rate;
    UINT32 ap_delay;
    mtk_int8 TB_PERCENT;
    UINT64 fifo_timestamp;
};
struct icm20645gy_data {
    struct gyro_hw           *hw;
    struct sensor_driver_convert   cvt;
    struct data_resolution *reso;
    int                     cali_sw[GYRO_AXES_NUM + 1];
    int                     trace;
    unsigned int            use_in_factory;
    struct gyro_fifo        gyro_fifo_param;
};

#define ICM20645_I2C_SLAVE_ADDR         0xD0


#define REG_BANK_SEL                    0x7F
#define BANK_SEL_0                      0x00
#define BANK_SEL_1                      0x10
#define BANK_SEL_2                      0x20
#define BANK_SEL_3                      0x30
#define ICM20645_REG_USER_CTRL          0x03
#define ICM20645_REG_FIFO_EN_2          0x67
#define ICM20645_REG_FIFO_RST           0x68
#define ICM20645_REG_FIFO_COUNTH        0x70
#define ICM20645_REG_FIFO_COUNTL        0x71
#define ICM20645_REG_FIFO_R_W           0x72

#define ICM20645_REG_LP_CONFIG          0x05
#define ICM20645_PWR_MGMT_1             0x06
#define ICM20645_PWR_MGMT_2             0x07

#define BIT_PWR_GYRO_STBY               0x07
#define BIT_GYRO_LP_EN                  0x10
#define BIT_LP_EN                       0x20
#define BIT_CLK_PLL                     0x01
#define BIT_TEMP_DIS                    (1<<3)

/* ICM20645 Register Map  (Please refer to ICM20645 Specifications) */

#define ICM20645_REG_DEVID              0x00

#define ICM20645_TIMEBASE_PLL           0x28
#define ICM20645_REG_SAMRT_DIV          0x00
#define ICM20645_REG_CFG                0x01
#define SHIFT_GYRO_FS_SEL                  1
#define ICM20645_GYRO_CFG2              0x02
#define GYRO_AVGCFG_1X                     0
#define GYRO_AVGCFG_2X                     1
#define GYRO_AVGCFG_4X                     2
#define GYRO_AVGCFG_8X                     3
#define GYRO_AVGCFG_16X                    4
#define GYRO_AVGCFG_32X                    5
#define GYRO_AVGCFG_64X                    6
#define GYRO_AVGCFG_128X                   7


#define ICM20645_REG_GYRO_XH            0x33


#define GYRO_FS_SEL_250                     (0x00 << SHIFT_GYRO_FS_SEL)
#define GYRO_FS_SEL_500                     (0x01 << SHIFT_GYRO_FS_SEL)
#define GYRO_FS_SEL_1000                    (0x02 << SHIFT_GYRO_FS_SEL)
#define GYRO_FS_SEL_2000                    (0x03 << SHIFT_GYRO_FS_SEL)
// for ICM20645_REG_GYRO_CFG

#define ICM20645_FS_1000             0x02


#define ICM20645_FS_250_LSB             131.0f
#define ICM20645_FS_500_LSB             65.5f
#define ICM20645_FS_1000_LSB            32.8f
#define ICM20645_FS_2000_LSB            16.4f

#define GYRO_DLPFCFG    (7<<3)
#define GYRO_FCHOICE    0x01


#define ICM20645_SLEEP               0x40   //enable low power sleep mode
#define ICM20645_DEV_RESET              0x80


#define ICM20645_BUFSIZE 60

// 1 rad = 180/PI degree, MAX_LSB = 131,
// 180*131/PI = 7506
#define DEGREE_TO_RAD   7506

#define ICM20645_DEFAULT_FS         ICM20645_FS_1000
#define ICM20645_DEFAULT_LSB        ICM20645_FS_1000_LSB


#define ICM20645_SUCCESS                0
#define ICM20645_ERR_I2C                -1
#define ICM20645_ERR_STATUS             -2
int icm20645gy_sensor_init(void);

#endif
