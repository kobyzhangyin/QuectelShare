/*
 * Copyright (c) 2015 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */
#ifndef __FOVCROP40_REG_H__
#define __FOVCROP40_REG_H__

#define ISP_FOV40_ENC_OFF 0x00000854
#define ISP_FOV40_ENC_LEN 4

#define ISP_FOV40_VIEW_OFF 0x00000864
#define ISP_FOV40_VIEW_LEN 4

/* Crop Config */
typedef struct ISP_FOV_CropConfig {
  uint32_t  lastPixel        : 13;
  uint32_t  /* reserved */   :  3;
  uint32_t  firstPixel       : 13;
  uint32_t  /* reserved */   :  3;

  uint32_t  lastLine         : 13;
  uint32_t  /* reserved */   :  3;
  uint32_t  firstLine        : 13;
  uint32_t  /* reserved */   :  3;
}__attribute__((packed, aligned(4))) ISP_FOV_CropConfig;

/* Field Of View (FOV) Crop Config Command */
typedef struct ISP_FOV_CropConfigCmdType {
  /* Y config */
  ISP_FOV_CropConfig  y_crop_cfg;
  /* CbCr Config */
  ISP_FOV_CropConfig  cbcr_crop_cfg;
}__attribute__((packed, aligned(4))) ISP_FOV_CropConfigCmdType;

#endif //__FOVCROP40_REG_H__
