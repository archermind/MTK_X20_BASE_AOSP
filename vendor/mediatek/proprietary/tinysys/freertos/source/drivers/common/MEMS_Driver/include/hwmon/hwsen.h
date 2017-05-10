#ifndef __HWSEN_H__
#define __HWSEN_H__
#define C_MAX_SENSOR_AXIS_NUM          	4
#define GRAVITY_EARTH_1000              9807
#define MAX_PHYSIC_SENSOR_NUM			6
struct sensor_driver_convert {
	signed char    		sign[C_MAX_SENSOR_AXIS_NUM];
	unsigned char   map[C_MAX_SENSOR_AXIS_NUM];
};
int sensor_driver_get_convert(int direction, struct sensor_driver_convert *cvt);
#endif
