#!/bin/bash
# Copyright Statement:
#
# This software/firmware and related documentation ("MediaTek Software") are
# protected under relevant copyright laws. The information contained herein
# is confidential and proprietary to MediaTek Inc. and/or its licensors.
# Without the prior written permission of MediaTek inc. and/or its licensors,
# any reproduction, modification, use or disclosure of MediaTek Software,
# and information contained herein, in whole or in part, shall be strictly prohibited.

# MediaTek Inc. (C) 2014. All rights reserved.
#
# BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
# THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
# RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
# AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
# NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
# SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
# SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
# THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
# THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
# CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
# SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
# STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
# CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
# AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
# OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
# MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
#
# The following software/firmware and/or related documentation ("MediaTek Software")
# have been modified by MediaTek Inc. All revisions are subject to any receiver's
# applicable license agreements with MediaTek Inc.

##############################################################
# Program:
# Program to create ARM trusted firmware and tee binary
#


# WARNING: build.sh is used only for fast standalone build, no longer called from Android.mk
# WARNING: post_process is moved to Makefile for incremental build
# WARNING: parameters in build.sh and Android.mk must be identical

# tells sh to exit with a non-zero status as soon as any executed command fails
# (i.e. exits with a non-zero exit status).
set -e

TRUSTZONE_PROJECT=$1
if [ -z ${ANDROID_BUILD_TOP} ]; then
  if [ -f "build/core/envsetup.mk" ]; then
    ANDROID_BUILD_TOP=`PWD= /bin/pwd`
  else
    echo "[ERROR] source build/envsetup.sh"
    exit 1
  fi
fi
if [ -z ${TARGET_PRODUCT} ]; then
  if [ -z ${TRUSTZONE_PROJECT} ]; then
    echo "[ERROR] source build/envsetup.sh; lunch TARGET_PRODUCT-TARGET_BUILD_VARIANT"
    exit 1
  fi
fi

echo "source ${ANDROID_BUILD_TOP}/build/envsetup.sh"
source ${ANDROID_BUILD_TOP}/build/envsetup.sh

if [ -z ${TARGET_PRODUCT} ]; then
  if [ ! -z ${TRUSTZONE_PROJECT} ]; then
    export TARGET_PRODUCT="full_${TRUSTZONE_PROJECT}"
    echo "lunch ${TARGET_PRODUCT}-eng"
    lunch ${TARGET_PRODUCT}-eng
  fi
fi

TRUSTZONE_ROOT_DIR=${ANDROID_BUILD_TOP}
TRUSTZONE_CUSTOM_BUILD_PATH=$(dirname $(readlink -f $0))
MTK_GOOGLE_TRUSTY_SUPPORT=$(get_build_var MTK_GOOGLE_TRUSTY_SUPPORT)
MICROTRUST_TEE_SUPPORT=$(get_build_var MICROTRUST_TEE_SUPPORT)
echo "TARGET_PRODUCT = ${TARGET_PRODUCT}"
echo "TRUSTZONE_ROOT_DIR = ${TRUSTZONE_ROOT_DIR}"
echo "MTK_GOOGLE_TRUSTY_SUPPORT = ${MTK_GOOGLE_TRUSTY_SUPPORT}"
echo "MICROTRUST_TEE_SUPPORT = ${MICROTRUST_TEE_SUPPORT}"

if [ "${MTK_GOOGLE_TRUSTY_SUPPORT}" = "yes" ]; then
  echo ${TRUSTZONE_ROOT_DIR}/trusty/lk/trusty/build.sh ${TRUSTZONE_PROJECT}
  ${TRUSTZONE_ROOT_DIR}/trusty/lk/trusty/build.sh ${TRUSTZONE_PROJECT}
fi
echo mmm -j24 ${TRUSTZONE_CUSTOM_BUILD_PATH}:trustzone
mmm -j24 ${TRUSTZONE_CUSTOM_BUILD_PATH}:trustzone

