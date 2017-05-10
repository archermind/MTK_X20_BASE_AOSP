#ifndef __CUST_MAG_H__
#define __CUST_MAG_H__

#define M_CUST_I2C_ADDR_NUM 2

struct mag_hw {
    int i2c_num;    /*!< the i2c bus used by the chip */
    int direction;  /*!< the direction of the chip */
	unsigned char   i2c_addr[M_CUST_I2C_ADDR_NUM];
};

extern struct mag_hw* get_cust_mag_hw(void); 
#endif 

