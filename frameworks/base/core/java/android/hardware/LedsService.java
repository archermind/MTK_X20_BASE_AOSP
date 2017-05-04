/*
 * Copyright (C) 2009 The Android Open Source Project
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
 
package android.hardware;

import android.util.Log;

public class LedsService
{
	private final static String TAG = "96board_leds";
	
	static{
			Log.d(TAG, "96board_leds run here begin!");
			System.loadLibrary("android_runtime");
			Log.d(TAG, "96board_leds run here!");
	}
	
	public LedsService()
	{
		Log.d(TAG, "LedsService init_native begin!");
		init_native();
		Log.d(TAG, "LedsService init_native end!");
	}

	public native boolean init_native();
	public native void set_user_led0(int switch_val);
	public native void set_user_led1(int switch_val);
	public native void set_user_led2(int switch_val);
	public native void set_user_led3(int switch_val);
	public native void set_wlan_led4(int switch_val);
	public native void set_ble_led5(int switch_val);
}