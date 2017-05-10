#ifndef __CUST_ACC_H__
#define __CUST_ACC_H__

#define G_CUST_I2C_ADDR_NUM 2

struct acc_hw {
    int i2c_num;    /*!< the i2c bus used by the chip */
    int direction;  /*!< the direction of the chip */
	unsigned char   i2c_addr[G_CUST_I2C_ADDR_NUM];
};

extern struct acc_hw* get_cust_acc_hw(void);
#endif 

