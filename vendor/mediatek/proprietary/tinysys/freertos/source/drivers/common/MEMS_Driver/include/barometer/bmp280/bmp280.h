#ifndef __BMP280_H__
#define __BMP280_H__


#include "cust_baro.h"
#define BMP280_I2C_ADDRESS 0x77
/* sensor type */
enum SENSOR_TYPE_ENUM {
	BMP280_TYPE = 0x0,

	INVALID_TYPE = 0xff
};
enum BMP_POWERMODE_ENUM {
	BMP_SUSPEND_MODE = 0x0,
	BMP_NORMAL_MODE,

	BMP_UNDEFINED_POWERMODE = 0xff
};

/* power mode */
enum BARO_DATA_ENUM {
	PRESSURE = 0,
	TEMP  = 1,
	DATA_SIZE = 2
};

/* filter */
enum BMP_FILTER_ENUM {
	BMP_FILTER_OFF = 0x0,
	BMP_FILTER_2,
	BMP_FILTER_4,
	BMP_FILTER_8,
	BMP_FILTER_16,

	BMP_UNDEFINED_FILTER = 0xff
};

/* oversampling */
enum BMP_OVERSAMPLING_ENUM {
	BMP_OVERSAMPLING_SKIPPED = 0x0,
	BMP_OVERSAMPLING_1X,
	BMP_OVERSAMPLING_2X,
	BMP_OVERSAMPLING_4X,
	BMP_OVERSAMPLING_8X,
	BMP_OVERSAMPLING_16X,

	BMP_UNDEFINED_OVERSAMPLING = 0xff
};


/* bmp280 calibration */
struct bmp280_calibration_data {
	unsigned short 	dig_T1;
	short 			dig_T2;
	short 			dig_T3;
	unsigned short 	dig_P1;
	short 			dig_P2;
	short 			dig_P3;
	short 			dig_P4;
	short 			dig_P5;
	short 			dig_P6;
	short 			dig_P7;
	short 			dig_P8;
	short 			dig_P9;
};

struct bmp280_data {
    struct baro_hw           		*hw;
	enum SENSOR_TYPE_ENUM 			sensor_type;
	enum BMP_POWERMODE_ENUM 		power_mode;
	unsigned char hw_filter;
	unsigned char oversampling_p;
	unsigned char oversampling_t;
	int t_fine;
	struct bmp280_calibration_data 	bmp280_cali;
	int trace;
	unsigned char i2c_addr;
};

#define C_MAX_FIR_LENGTH (32)
#define MAX_SENSOR_NAME  (32)
#define BMP_DATA_NUM 1
#define BMP_PRESSURE         0
#define BMP_BUFSIZE			128

/* common definition */
#define BMP_GET_BITSLICE(regvar, bitname)\
	((regvar & bitname##__MSK) >> bitname##__POS)

#define BMP_SET_BITSLICE(regvar, bitname, val)\
	((regvar & ~bitname##__MSK) | ((val<<bitname##__POS)&bitname##__MSK))


#define BMP_CHIP_ID_REG	0xD0

/*********************************[BMP280]*************************************/


/* chip id */
#define BMP280_CHIP_ID1 0x56
#define BMP280_CHIP_ID2 0x57
#define BMP280_CHIP_ID3 0x58
       
/* calibration data */
#define BMP280_CALIBRATION_DATA_START       0x88 /* BMP280_DIG_T1_LSB_REG */
#define BMP280_CALIBRATION_DATA_LENGTH		12
            
#define SHIFT_RIGHT_4_POSITION				 4
#define SHIFT_LEFT_2_POSITION                2
#define SHIFT_LEFT_4_POSITION                4
#define SHIFT_LEFT_5_POSITION                5
#define SHIFT_LEFT_8_POSITION                8
#define SHIFT_LEFT_12_POSITION               12
#define SHIFT_LEFT_16_POSITION               16

/* power mode */
#define BMP280_SLEEP_MODE                    0x00
#define BMP280_FORCED_MODE                   0x01
#define BMP280_NORMAL_MODE                   0x03

#define BMP280_CTRLMEAS_REG                  0xF4  /* Ctrl Measure Register */

#define BMP280_CTRLMEAS_REG_MODE__POS              0
#define BMP280_CTRLMEAS_REG_MODE__MSK              0x03
#define BMP280_CTRLMEAS_REG_MODE__LEN              2
#define BMP280_CTRLMEAS_REG_MODE__REG              BMP280_CTRLMEAS_REG

/* filter */
#define BMP280_FILTERCOEFF_OFF               0x00
#define BMP280_FILTERCOEFF_2                 0x01
#define BMP280_FILTERCOEFF_4                 0x02
#define BMP280_FILTERCOEFF_8                 0x03
#define BMP280_FILTERCOEFF_16                0x04

#define BMP280_CONFIG_REG                    0xF5  /* Configuration Register */

#define BMP280_CONFIG_REG_FILTER__POS              2
#define BMP280_CONFIG_REG_FILTER__MSK              0x1C
#define BMP280_CONFIG_REG_FILTER__LEN              3
#define BMP280_CONFIG_REG_FILTER__REG              BMP280_CONFIG_REG

/* oversampling */
#define BMP280_OVERSAMPLING_SKIPPED          0x00
#define BMP280_OVERSAMPLING_1X               0x01
#define BMP280_OVERSAMPLING_2X               0x02
#define BMP280_OVERSAMPLING_4X               0x03
#define BMP280_OVERSAMPLING_8X               0x04
#define BMP280_OVERSAMPLING_16X              0x05

#define BMP280_CTRLMEAS_REG_OSRST__POS             5
#define BMP280_CTRLMEAS_REG_OSRST__MSK             0xE0
#define BMP280_CTRLMEAS_REG_OSRST__LEN             3
#define BMP280_CTRLMEAS_REG_OSRST__REG             BMP280_CTRLMEAS_REG

#define BMP280_CTRLMEAS_REG_OSRSP__POS             2
#define BMP280_CTRLMEAS_REG_OSRSP__MSK             0x1C
#define BMP280_CTRLMEAS_REG_OSRSP__LEN             3
#define BMP280_CTRLMEAS_REG_OSRSP__REG             BMP280_CTRLMEAS_REG

/* data */
#define BMP280_PRESSURE_MSB_REG              0xF7  /* Pressure MSB Register */
#define BMP280_TEMPERATURE_MSB_REG           0xFA  /* Temperature MSB Reg */

/* i2c disable switch */
#define BMP280_I2C_DISABLE_SWITCH            0x87




#define BMP280_SUCCESS                0
#define BMP280_ERR_I2C                -1
#define BMP280_ERR_STATUS             -2

#endif
