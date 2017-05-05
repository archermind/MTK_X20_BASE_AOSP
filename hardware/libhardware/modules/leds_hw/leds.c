/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define LOG_TAG "96board_leds"

//#define LOG_NDEBUG 0

#include <errno.h>
#include <stdint.h>
#include <fcntl.h>
#include <cutils/log.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/wait.h>
#include <netdb.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>


#include <hardware/hardware.h>
#include <hardware/leds_hw.h>

int led0_set_user_led(struct local_96board_leds_hw_device* dev,const char * flg)
{
	int fd=-1;
	
	const char *devnum_dir = "/sys/class/misc/96board_leds/96_led0";
	 
	ALOGD("led0_set_user_led:  flg:%s \r\n",flg);

    fd = open(devnum_dir, O_RDWR);
    if(fd<0)
    {
        ALOGD("open %s failure! \r\n",devnum_dir);
        return -1;
    }
  
	
    write(fd, flg, strlen(flg));
	
    close(fd);
	
	ALOGD("led0_set_user_led:  flg:%s \r\n",flg);
	
    return 0;
	
}

int led1_set_user_led(struct local_96board_leds_hw_device* dev,const char * flg)
{
	int fd=-1;
	
	
	const char *devnum_dir = "/sys/class/misc/96board_leds/96_led1";
	 
	ALOGD("led1_set_user_led:  flg:%s \r\n",flg);

    fd = open(devnum_dir, O_RDWR);
    if(fd<0)
    {
        ALOGD("open %s failure! \r\n",devnum_dir);
        return -1;
    }
  
	
    write(fd, flg, strlen(flg));
	
    close(fd);
	
	ALOGD("led1_set_user_led:  flg:%s \r\n",flg);
	
    return 0;
	
}

int led2_set_user_led(struct local_96board_leds_hw_device* dev,const char * flg)
{
	int fd=-1;
	
	
	const char *devnum_dir = "/sys/class/misc/96board_leds/96_led2";
	
	ALOGD("led2_set_user_led:  flg:%s \r\n",flg);
	 
    fd = open(devnum_dir, O_RDWR);
    if(fd<0)
    {
        ALOGD("open %s failure! \r\n",devnum_dir);
        return -1;
    }
  
	
    write(fd, flg, strlen(flg));
	
    close(fd);
	
	ALOGD("led2_set_user_led:  flg:%s \r\n",flg);
	
    return 0;	
}
  
int led3_set_user_led(struct local_96board_leds_hw_device* dev,const char * flg) 
{
	int fd=-1;
	
	
	const char *devnum_dir = "/sys/class/misc/96board_leds/96_led3";
	
	ALOGD("led3_set_user_led:  flg:%s \r\n",flg);

    fd = open(devnum_dir, O_RDWR);
    if(fd<0)
    {
        ALOGD("open %s failure! \r\n",devnum_dir);
        return -1;
    }
  
	
    write(fd, flg, strlen(flg));
	
    close(fd);
	
	ALOGD("led2_set_user_led:  flg:%s \r\n",flg);
	
    return 0;
	
	
}

int led4_set_wlan_led(struct local_96board_leds_hw_device* dev,const char * flg)
{
	int fd=-1;
	
	const char *devnum_dir = "/sys/class/misc/96board_leds/96_led4";
	
	ALOGD("led4_set_user_led:  flg:%s \r\n",flg);

    fd = open(devnum_dir, O_RDWR);
    if(fd<0)
    {
        ALOGD("open %s failure! \r\n",devnum_dir);
        return -1;
    }
  
	
    write(fd, flg, strlen(flg));
	
    close(fd);
	
	ALOGD("led4_set_user_led:  flg:%s \r\n",flg);
	
    return 0;
	
	
}

int led5_set_ble_led(struct local_96board_leds_hw_device* dev,const char * flg)
{
	int fd=-1;
	
	
	const char *devnum_dir = "/sys/class/misc/96board_leds/96_led5";
	
	ALOGD("led5_set_user_led:  flg:%s \r\n",flg);	

    fd = open(devnum_dir, O_RDWR);
    if(fd<0)
    {
        ALOGD("open %s failure! \r\n",devnum_dir);
        return -1;
    }
  
	
    write(fd, flg, strlen(flg));
	
    close(fd);
	
	ALOGD("led5_set_user_led:  flg:%s \r\n",flg);
	
    return 0;
	
	
}

static int leds_dev_close(hw_device_t *device)
{
    free(device);
	
    return 0;
}

static int leds_dev_open(const hw_module_t* module, const char* name,hw_device_t** device)
{
    struct local_96board_leds_hw_device *leds_device;
	
    int ret;

	ALOGE("96board_leds:leds_dev_open begin.");
	
    leds_device = calloc(1, sizeof(struct local_96board_leds_hw_device));
    if (!leds_device)
	{
		ALOGE("96board_leds:calloc leds_device failed!");
		return -ENOMEM;
	}
    
	
    leds_device->common.tag = HARDWARE_DEVICE_TAG;
    leds_device->common.version = 0;
    leds_device->common.module = (struct hw_module_t *) module;
    leds_device->common.close = leds_dev_close;

	
    leds_device->set_96board_user_led0 = led0_set_user_led;
	leds_device->set_96board_user_led1 = led1_set_user_led;;
	leds_device->set_96board_user_led2 = led2_set_user_led;;
	leds_device->set_96board_user_led3 = led3_set_user_led;;
	leds_device->set_96board_wlan_led4 = led4_set_wlan_led;;
	leds_device->set_96board_ble_led5 =  led5_set_ble_led;;
	
    *device = &leds_device->common;
	
	
    return 0;
}

static struct hw_module_methods_t hal_module_methods = {
    .open = leds_dev_open,
};

struct local_96board_leds_module HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .version_major = 1,
        .version_minor = 0,
        .id = LOCAL_96BOARD_LEDS_HARDWARE_MODULE_ID,
        .name = "Default 96board_leds HW HAL",
        .author = "The Android Open Source Project",
        .methods = &hal_module_methods,
    },
};
