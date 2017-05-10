#ifndef __CUST_HMDY_H__
#define __CUST_HMDY_H__

#define H_CUST_I2C_ADDR_NUM 2

struct hmdy_hw {
    int i2c_num;    /*!< the i2c bus used by the chip */
    int direction;  /*!< the direction of the chip */
	unsigned char   i2c_addr[H_CUST_I2C_ADDR_NUM];
};

extern struct hmdy_hw* get_cust_hmdy_hw(void);
#endif 

