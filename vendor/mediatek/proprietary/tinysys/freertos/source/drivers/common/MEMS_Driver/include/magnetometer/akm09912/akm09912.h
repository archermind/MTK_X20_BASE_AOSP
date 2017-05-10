#ifndef __AKM09912_H__
#define __AKM09912_H__


#include "cust_mag.h"
#include "hwsen.h"
#define CONVERT_TO_uT 	6.6666
#define VENDOR_DIV 	100

#define AKM09912_I2C_ADDRESS_7BIT 0x0c
#define MAG_AXIS_X          	0
#define MAG_AXIS_Y          	1
#define MAG_AXIS_Z          	2
#define MAG_AXES_NUM        	3
#define AKM09912_DATA_LEN       8

struct akm09912_data {
    struct mag_hw           *hw;
    struct sensor_driver_convert   cvt;
    unsigned char    i2c_addr;
    int              trace;
};

#define AKM09912_SUCCESS                0
#define AKM09912_ERR_I2C                -1
#define AKM09912_ERR_STATUS             -2

/* Device specific constant values */
#define AKM09912_REG_WIA1			0x00
#define AKM09912_REG_WIA2			0x01
#define AKM09912_REG_INFO1			0x02
#define AKM09912_REG_INFO2			0x03
#define AKM09912_REG_ST1				0x10
#define AKM09912_REG_HXL				0x11
#define AKM09912_REG_HXH				0x12
#define AKM09912_REG_HYL				0x13
#define AKM09912_REG_HYH				0x14
#define AKM09912_REG_HZL				0x15
#define AKM09912_REG_HZH				0x16
#define AKM09912_REG_TMPS			0x17
#define AKM09912_REG_ST2				0x18
#define AKM09912_REG_CNTL1			0x30
#define AKM09912_REG_CNTL2			0x31
#define AKM09912_REG_CNTL3			0x32
#define AKM09912_REG_ASAX			0x60
#define AKM09912_REG_ASAY			0x61
#define AKM09912_REG_ASAZ			0x62


/*! \name AKM09912 operation mode
 \anchor AKM09912_Mode
 Defines an operation mode of the AKM09912.*/
#define AKM09912_MODE_SNG_MEASURE	0x01
#define AKM09912_MODE_10HZ_MEASURE  0x02
#define AKM09912_MODE_20HZ_MEASURE  0x04
#define AKM09912_MODE_50HZ_MEASURE  0x06
#define AKM09912_MODE_100HZ_MEASURE 0x08
#define AKM09912_MODE_SELF_TEST		0x10
#define AKM09912_MODE_FUSE_ACCESS	0x1F
#define AKM09912_MODE_POWERDOWN		0x00
#define AKM09912_RESET_DATA			0x01

#define DATA_READY_BIT				0X01

int akm09912_sensor_init(void);

#endif
