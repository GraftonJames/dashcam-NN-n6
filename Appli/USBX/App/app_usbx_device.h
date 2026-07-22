/**
 ******************************************************************************
 * @file    app_usbx_device.h
 * @brief   USBX Device applicative header - Phase 4 (UVC port plan).
 ******************************************************************************
 * Adapted from ST's VENC_USB reference (Appli/USBX/App/app_usbx_device.h),
 * unchanged apart from the header comment.
 ******************************************************************************
 */
#ifndef __APP_USBX_DEVICE_H__
#define __APP_USBX_DEVICE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ux_api.h"
#include "ux_device_descriptors.h"
#include "ux_device_video.h"
#include "ux_dcd_stm32.h"

#define USBX_DEVICE_MEMORY_STACK_SIZE (30 * 1024)

#define UX_DEVICE_APP_THREAD_STACK_SIZE (8 * 1024)
#define UX_DEVICE_APP_THREAD_PRIO	 10

#define APP_QUEUE_SIZE 5

UINT MX_USBX_Device_Init(VOID *memory_ptr);
VOID USBX_APP_Device_Init(VOID);

#ifndef UX_DEVICE_APP_THREAD_NAME
#define UX_DEVICE_APP_THREAD_NAME "USBX Device App Main Thread"
#endif

#ifndef UX_DEVICE_APP_THREAD_PREEMPTION_THRESHOLD
#define UX_DEVICE_APP_THREAD_PREEMPTION_THRESHOLD UX_DEVICE_APP_THREAD_PRIO
#endif

#ifndef UX_DEVICE_APP_THREAD_TIME_SLICE
#define UX_DEVICE_APP_THREAD_TIME_SLICE TX_NO_TIME_SLICE
#endif

#ifndef UX_DEVICE_APP_THREAD_START_OPTION
#define UX_DEVICE_APP_THREAD_START_OPTION TX_AUTO_START
#endif

typedef enum
{
	STOP_USB_DEVICE = 1,
	START_USB_DEVICE,
} USB_MODE_STATE;

#ifdef __cplusplus
}
#endif
#endif /* __APP_USBX_DEVICE_H__ */
