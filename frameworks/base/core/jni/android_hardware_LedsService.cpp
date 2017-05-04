/*
* Copyright 2014 The Android Open Source Project
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

#include "jni.h"
#include "JNIHelp.h"
#include "android_runtime/AndroidRuntime.h"
#include <utils/Log.h>
#include <utils/misc.h>
#include <hardware/hardware.h>
#include <hardware/leds_hw.h>
#include <stdio.h>


struct local_96board_leds_hw_device *leds_device = NULL;


static void  set_led0(JNIEnv *env, jobject clazz,jint switch_val)
{
	ALOGE("set_led0 JNI: set  value %d to device.", switch_val);
	
    /*const char *val =  env->GetStringUTFChars(switch_val, NULL); 
	
	ALOGE("set_led0 JNI: set  value %s to device.", val);
	
	if(!leds_device)
	{
		ALOGE("set_led0 JNI:device is not open.");
	}
	
	leds_device->set_96board_user_led0(leds_device,val);*/
}

static void set_led1(JNIEnv *env, jobject clazz,jint switch_val)
{
	ALOGE("set_led1 JNI: set  value %d to device.", switch_val);
	
	/*const char *val =  env->GetStringUTFChars(switch_val, NULL); 
	
	ALOGE("set_led1 JNI: set  value %s to device.", val);
	
	if(!leds_device)
	{
		ALOGE("set_led1 JNI:device is not open.");
	}
	
	leds_device->set_96board_user_led1(leds_device,val);*/
}

static void set_led2(JNIEnv *env, jobject clazz,jint switch_val)
{
	ALOGE("set_led2 JNI: set  value %d to device.", switch_val);
	
	/*const char *val =  env->GetStringUTFChars(switch_val, NULL); 
	
	ALOGE("set_led2 JNI: set  value %s to device.", val);
	
	if(!leds_device)
	{
		ALOGE("set_led2 JNI:device is not open.");
	}
	
	leds_device->set_96board_user_led2(leds_device,val); */
}

static void set_led3(JNIEnv *env, jobject clazz,jint switch_val)
{
	ALOGE("set_led3 JNI: set  value %d to device.", switch_val);
	
	/*const char *val =  env->GetStringUTFChars(switch_val, NULL); 
	
	ALOGE("set_led3 JNI: set  value %s to device.", val);
	
	if(!leds_device)
	{
		ALOGE("set_led3 JNI:device is not open.");
	}
	
	leds_device->set_96board_user_led3(leds_device,val);*/
}

static void set_led4(JNIEnv *env, jobject clazz,jint switch_val)
{
	ALOGE("set_led4 JNI: set  value %d to device.", switch_val);
	
	//const char *val =  env->GetStringUTFChars(switch_val, NULL);

	//ALOGE("set_led4 JNI: set  value %s to device.", val);	
	
	if(!leds_device)
	{
		ALOGE("set_led4 JNI:device is not open.");
	}
	
	if(switch_val)
	{
		leds_device->set_96board_wlan_led4(leds_device,"on");
	}
	else
	{
		leds_device->set_96board_wlan_led4(leds_device,"off");
	}
}

static void set_led5(JNIEnv *env, jobject clazz,jint switch_val)
{
	ALOGE("set_led5 JNI: set  value %d to device.", switch_val);
	 
	 //const char *val =  env->GetStringUTFChars(switch_val, NULL); 
	 
	 //ALOGE("set_led5 JNI: set  value %s to device.", val);
	
	if(!leds_device)
	{
		ALOGE("set_led5 JNI:device is not open.");
	}
	
	if(switch_val)
	{
		leds_device->set_96board_ble_led5(leds_device,"on");
	}
	else
	{
		leds_device->set_96board_ble_led5(leds_device,"off");
	}
}

static inline int leds_device_open(const hw_module_t *module,struct local_96board_leds_hw_device** device)
{
	ALOGE("96board_leds JNI:leds_device_open");
	
	return module->methods->open(module,LOCAL_96BOARD_LEDS_HARDWARE_MODULE_ID,(struct hw_device_t**)device);
}

static  jboolean leds_init(JNIEnv *env ,jclass clazz)
{
	int ret ;
	local_96board_leds_module *module;

	ALOGE("96board_leds JNI:initializing......");
		
	if((ret = hw_get_module(LOCAL_96BOARD_LEDS_HARDWARE_MODULE_ID,(const struct hw_module_t**)(&module)))==0)	
	{

		ALOGE("96board_leds JNI:96board_leds Stub found.");
		
		if(leds_device_open(&(module->common),&leds_device)==0)
		{
			ALOGE("96board_leds JNI: 96board_leds device is open.");
			return 0;
		}
		
		ALOGE("96board_leds JNI:failed to get 96board_leds stub module 0000.");
		
		return -1;
	}
	ALOGE("96board_leds JNI:failed to get 96board_leds stub module 111.%d",ret);
	return -1;
}


static const JNINativeMethod method_table[] = {
	{"init_native","()Z",(void*)leds_init},
	{"set_user_led0","(I)V",(void*)set_led0},
	{"set_user_led1","(I)V",(void*)set_led1},
	{"set_user_led2","(I)V",(void*)set_led2},
	{"set_user_led3","(I)V",(void*)set_led3},
	{"set_wlan_led4","(I)V",(void*)set_led4},
	{"set_ble_led5","(I)V",(void*)set_led5},	
};

int register_android_hardware_LedsService(JNIEnv *env){
	
	ALOGE("[chaobing] register_android_hardware_LedsService"); 
	return jniRegisterNativeMethods(env,"android/hardware/LedsService",method_table,NELEM(method_table));
}

jint JNI_OnLoad(JavaVM *jvm, void *reserved)  
{ 
	JNIEnv *env; 
		
    ALOGE("[chaobing] begin JNI_OnLoad");
	
	if (jvm->GetEnv((void **)&env, JNI_VERSION_1_6)) 
	{  
		ALOGE("JNI version mismatch error.");
		return -1;  
	}  
	
	if (register_android_hardware_LedsService(env) < 0) 
	{  
		ALOGE("jni adapter fail."); 
		return -1;  
	}  
	
	ALOGE("[chaobing] begin JNI_OnLoad"); 
	
	return JNI_VERSION_1_6;  
 }
/*namespace android{
	int android_hardware_LedsService(JNIEnv *env){
		return AndroidRuntime::registerNativeMethods(env,"android/hardware/LedsService",method_table,NELEM(method_table));
	}
}*/


