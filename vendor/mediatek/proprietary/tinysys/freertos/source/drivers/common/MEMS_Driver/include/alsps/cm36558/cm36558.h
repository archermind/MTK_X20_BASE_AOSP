
#ifndef __CM36558_H__
#define __CM36558_H__
#include "cust_alsps.h"
#include "sensor_manager.h"
#include <FreeRTOS.h>
#include <semphr.h>

#define CM36558_SLAVE_ADDR      (0x60)

/*CM36558 als/ps sensor register related macro*/
#define CM36558_REG_ALS_CONF 		0X00
#define CM36558_REG_ALS_THDH 		0X01
#define CM36558_REG_ALS_THDL 		0X02
#define CM36558_REG_PS_CONF1_2		0X03
#define CM36558_REG_PS_CONF3_MS		0X04
#define CM36558_REG_PS_CANC			0X05
#define CM36558_REG_PS_THDL			0X06
#define CM36558_REG_PS_THDH			0X07
#define CM36558_REG_PS_DATA			0X08
#define CM36558_REG_ALS_DATA		0X09
#define CM36558_REG_UVAS_DATA		0X0B
#define CM36558_REG_UVBS_DATA		0X0C
#define CM36558_REG_INT_FLAG		0X0D
#define CM36558_REG_ID				0X0E

/*CM36558 related driver tag macro*/
#define CM36558_SUCCESS				 		 0
#define CM36558_ERR_I2C						-1
#define CM36558_ERR_STATUS					-3
#define CM36558_ERR_SETUP_FAILURE			-4
#define CM36558_ERR_GETGSENSORDATA			-5
#define CM36558_ERR_IDENTIFICATION			-6

struct cm36558_priv {
	struct alsps_hw  *hw;
	//struct work_struct	eint_work;

	/*misc*/
	unsigned short  als_modulus;
	unsigned int	als_suspend;
	unsigned int	als_debounce;
	unsigned int	als_deb_on; 	/*indicates if the debounce is on*/
	portTickType	als_deb_end;
	unsigned int	ps_mask;		/*mask ps: always return far away*/
	unsigned int	ps_debounce;
	unsigned int	ps_deb_on;		/*indicates if the debounce is on*/
	portTickType	ps_deb_end;
	unsigned int	ps_suspend;
	unsigned int 	trace;
    unsigned int  gpio_pin;
    unsigned int  gpio_mode;
    unsigned int  eint_num;
    unsigned int  eint_is_deb_en;
    unsigned int  eint_type;
    //event_t e;
	
	/*data*/
	unsigned short  als;
	unsigned int  ps;
	//uint8_t   _align;
	unsigned short  als_level_num;
	unsigned short  als_value_num;
	int			ps_cali;
	
	unsigned int	als_cmd_val;	/*the cmd value can't be read, stored in ram*/
	unsigned int	ps_cmd_val; 	/*the cmd value can't be read, stored in ram*/
	unsigned int	ps_thd_val_high;	 /*the cmd value can't be read, stored in ram*/
	unsigned int	ps_thd_val_low; 	/*the cmd value can't be read, stored in ram*/
	unsigned int	als_thd_val_high;	 /*the cmd value can't be read, stored in ram*/
	unsigned int	als_thd_val_low; 	/*the cmd value can't be read, stored in ram*/
	unsigned int	ps_thd_val;
	unsigned int		enable; 		/*enable mask*/
	unsigned int		pending_intr;	/*pending interrupt*/
	unsigned int		is_interrupt_context;	/*pending interrupt*/
	SemaphoreHandle_t xSemaphore;
	int intr_flag;
	unsigned char i2c_addr;
};

typedef enum{
    CM36558_NOTIFY_PROXIMITY_CHANGE = 1,
} CM36558_NOTIFY_TYPE;
struct PS_CALI_DATA_STRUCT
{
	int close;
	int far_away;
	int valid;
};
typedef enum {
	CMC_BIT_ALS    = 1,
	CMC_BIT_PS	   = 2,
}CMC_BIT;

typedef enum {
    CMC_TRC_ALS_DATA= 0x0001,
    CMC_TRC_PS_DATA = 0x0002,
    CMC_TRC_EINT    = 0x0004,
    CMC_TRC_IOCTL   = 0x0008,
    CMC_TRC_I2C     = 0x0010,
    CMC_TRC_CVT_ALS = 0x0020,
    CMC_TRC_CVT_PS  = 0x0040,
    CMC_TRC_DEBUG   = 0x8000,
} CMC_TRC;

#endif

