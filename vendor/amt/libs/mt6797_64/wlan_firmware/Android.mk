# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
ifneq ($(findstring banyan,$(MTK_PROJECT)),)
  LOCAL_SRC_FILES := WIFI_RAM_CODE_SOC
  LOCAL_MODULE := $(LOCAL_SRC_FILES)
else ifeq ($(strip $(BOARD_CONNECTIVITY_MODULE)), conn_soc)
# remove prefix and subffix chars, only left numbers.
  WLAN_CHIP_ID := $(patsubst %m,%,$(patsubst mt%,%,$(patsubst MT%,%,$(strip $(MTK_PLATFORM)))))
#if your chip will share the same ramcode with other chips, and the ramcode name is not WIFI_RAM_CODE_SOC,
#please give a specific chip id to WLAN_CHIP_ID, it will override the previous value of WLAN_CHIP_ID
# for example
# ifeq ($(MTK_PLATFORM), xxx)
#   WLAN_CHIP_ID := yyy
# endif
  ifneq ($(wildcard $(LOCAL_PATH)/WIFI_RAM_CODE_$(WLAN_CHIP_ID)),)
    LOCAL_SRC_FILES := WIFI_RAM_CODE_$(WLAN_CHIP_ID)
  else
    LOCAL_SRC_FILES := WIFI_RAM_CODE_SOC
  endif
#new ram code mechanism only apply to new chips who don't share ram code with old chips
#if the new chips don't share ram code with old chips, please assign platform name to variable NEW_CHIPS
#by default, we apply old mechanism, to do backward
#after most old chips phased out, we will apply new mechanism as default

  NEW_CHIPS :=
  ifneq ($(filter $(MTK_PLATFORM),$(NEW_CHIPS)),)
    LOCAL_MODULE := WIFI_RAM_CODE_AD
  else
    LOCAL_MODULE := $(LOCAL_SRC_FILES)
  endif
else ifneq ($(strip $(BOARD_CONNECTIVITY_MODULE)),)
  LOCAL_SRC_FILES := WIFI_RAM_CODE_$(strip $(BOARD_CONNECTIVITY_MODULE))
  LOCAL_MODULE := $(LOCAL_SRC_FILES)
else
  $(error no firmware for project=$(MTK_PROJECT), platform=$(MTK_PLATFORM)!)
endif

LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/firmware
include $(BUILD_PREBUILT)
