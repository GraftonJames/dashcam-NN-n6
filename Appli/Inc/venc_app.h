/**
 ******************************************************************************
 * @file    venc_app.h
 * @brief   VENC H264 encoder driving loop header - Phase 3/4 (UVC port plan).
 ******************************************************************************
 */
#ifndef VENC_APP_H
#define VENC_APP_H

#include "tx_api.h"

UINT VENC_APP_GetData(UCHAR **data, ULONG *size);
void VENC_APP_ThreadCreate(void);

#endif /* VENC_APP_H */
