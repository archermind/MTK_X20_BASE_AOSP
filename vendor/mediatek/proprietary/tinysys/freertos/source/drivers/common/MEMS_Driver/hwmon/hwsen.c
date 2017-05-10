#include "hwsen.h"
#include "sensor_manager.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <FreeRTOSConfig.h>
#include <platform.h>
#include "FreeRTOS.h"
#include "task.h"

struct sensor_driver_convert map[] = {
    { { 1, 1, 1}, {0, 1, 2} },
    { {-1, 1, 1}, {1, 0, 2} },
    { {-1,-1, 1}, {0, 1, 2} },
    { { 1,-1, 1}, {1, 0, 2} },

    { {-1, 1,-1}, {0, 1, 2} },
    { { 1, 1,-1}, {1, 0, 2} },
    { { 1,-1,-1}, {0, 1, 2} },
    { {-1,-1,-1}, {1, 0, 2} },

};
/*----------------------------------------------------------------------------*/
int sensor_driver_get_convert(int direction, struct sensor_driver_convert *cvt)
{
    struct sensor_driver_convert *src;
    if (!cvt)
        return -1;
    else if (direction >= sizeof(map)/sizeof(map[0]))
        return -1;

    //*cvt = map[direction];
    src = &map[direction];
    memcpy(cvt, src, sizeof(struct sensor_driver_convert));
    return 0;
}
