/*
 * Copyright (C) 2011, 2012 The Android Open Source Project
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

#ifndef ANDROID_LEDS_HW_HAL_INTERFACE_H
#define ANDROID_LEDS_HW_HAL_INTERFACE_H

#include <stdint.h>
#include <strings.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include <hardware/hardware.h>

__BEGIN_DECLS


#define LOCAL_96BOARD_LEDS_HARDWARE_MODULE_ID "leds"  


struct local_96board_leds_hw_device {
    
    struct hw_device_t common;
    
    int (*set_96board_user_led0)(struct local_96board_leds_hw_device* dev,const char * flg);

    int (*set_96board_user_led1)(struct local_96board_leds_hw_device* dev,const char * flg);
    
    int (*set_96board_user_led2)(struct local_96board_leds_hw_device* dev,const char * flg);
	
	int (*set_96board_user_led3)(struct local_96board_leds_hw_device* dev,const char * flg);
	
	int (*set_96board_wlan_led4)(struct local_96board_leds_hw_device* dev,const char * flg);
	
	int (*set_96board_ble_led5)(struct local_96board_leds_hw_device* dev,const char * flg);
   
};

struct local_96board_leds_module {
    struct hw_module_t common;
};

__END_DECLS

#endif 
