/**
******************************************************************************
* @file          plugin_venc.h
* @author        MCD Application Team
* @brief         venc support
******************************************************************************
* @attention
*
* Copyright (c) 2018(-2022) STMicroelectronics.
* All rights reserved.
*
* This software is licensed under terms that can be found in the LICENSE file
* in the root directory of this software component.
* If no LICENSE file comes with this software, it is provided AS-IS.
*
******************************************************************************
*/



#ifndef plugin_venc_h
#define plugin_venc_h


#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdarg.h>
#include <stdint.h>
#include "stm32n6xx_ll_venc.h"
#include "h264encapi.h"

/* Exported types ------------------------------------------------------------*/

typedef struct cam_h264_cfg_t
{
  uint32_t                  width;
  uint32_t                  height;
}cam_h264_cfg_t;


typedef struct dcmipp_h264_cfg_t
{
  float                  bytes_per_pixel;
  uint32_t               format;
  uint32_t               pitch;
}dcmipp_h264_cfg_t;

  
typedef struct venc_h264_cfg_t
{
  H264EncConfig                 cfgH264Main;
  H264EncPreProcessingCfg       cfgH264Preproc;
  H264EncCodingCtrl             cfgH264Coding;
  H264EncRateCtrl               cfgH264Rate;
}venc_h264_cfg_t;




extern cam_h264_cfg_t     hCamH264Instance;
extern dcmipp_h264_cfg_t  hDcmippH264Instance;
extern venc_h264_cfg_t    hVencH264Instance;


void EWLPoolChoiceCb(u8 **pool_ptr, size_t *size);
uint8_t * GetInputFrame(uint32_t * framSize);


#ifdef __cplusplus
};
#endif

#endif


