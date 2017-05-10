/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2014. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#ifndef __OEM_SVC_H__
#define __OEM_SVC_H__

/*******************************************************************************
 * Defines for runtime services func ids
 ******************************************************************************/
/* SMC32 ID range from 0x83000000 to 0x83000FFF */
/* SMC64 ID range from 0xC3000000 to 0xC3000FFF */
#define OEM_SMC_CALL_1_AARCH32        0x83000000
#define OEM_SMC_CALL_1_AARCH64        0xC3000000
#define OEM_SMC_CALL_2_AARCH32        0x83000001
#define OEM_SMC_CALL_2_AARCH64        0xC3000001
#define OEM_SMC_CALL_3_AARCH32        0x83000002
#define OEM_SMC_CALL_3_AARCH64        0xC3000002

/*
 * Number of OEM calls (above) implemented.
 */
#define OEM_SVC_NUM_CALLS             3

/*******************************************************************************
 * Defines for OEM Service queries
 ******************************************************************************/
/* 0x83000000 - 0x8300FEFF is OEM service calls */
#define OEM_SVC_CALL_COUNT      0x8300ff00
#define OEM_SVC_UID             0x8300ff01
/* 0x8300ff02 is reserved */
#define OEM_SVC_VERSION         0x8300ff03
/* 0x8300ff04 - 0x8300FFFF is reserved for future expansion */

/* OEM Service Calls version numbers */
#define OEM_VERSION_MAJOR		0x0
#define OEM_VERSION_MINOR		0x1

/* The macros below are used to identify OEM calls from the SMC function ID */
/* SMC32 ID range from 0x83000000 to 0x83000FFF */
/* SMC64 ID range from 0xC3000000 to 0xC3000FFF */
#define OEM_FID_MASK                0xf000u
#define OEM_FID_VALUE               0u
#define is_oem_fid(_fid) \
	(((_fid) & OEM_FID_MASK) == OEM_FID_VALUE)

#endif /* __OEM_SVC_H__ */
