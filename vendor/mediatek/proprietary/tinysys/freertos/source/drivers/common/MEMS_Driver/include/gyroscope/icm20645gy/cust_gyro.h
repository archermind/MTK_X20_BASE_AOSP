#ifndef __CUST_GYRO_H__
#define __CUST_GYRO_H__

#define GY_CUST_I2C_ADDR_NUM 2

struct gyro_hw {
    int i2c_num;    /*!< the i2c bus used by the chip */
    int direction;  /*!< the direction of the chip */
	unsigned char   i2c_addr[GY_CUST_I2C_ADDR_NUM];
};

extern struct gyro_hw* get_cust_gyro_hw(void);
#endif 

