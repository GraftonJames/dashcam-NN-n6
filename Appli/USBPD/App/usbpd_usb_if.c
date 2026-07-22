/**
 ******************************************************************************
 * @file    usbpd_usb_if.c
 * @brief   USBPD <-> USBX data-role integration - Phase 4 (UVC port plan).
 ******************************************************************************
 * Adapted from ST's VENC_USB reference (Appli/USBPD/App/usbpd_usb_if.c),
 * trimmed to the DeviceStart/DeviceStop pair this SNK-only (no DFP/host mode)
 * port actually calls - see USBPD_DPM_Notification() in usbpd_dpm_user.c for
 * where USBPD_NOTIFY_USBSTACK_START/STOP triggers these.
 ******************************************************************************
 */
#include "main.h"
#include "usbpd_usb_if.h"
#include "app_usbx_device.h"

extern TX_QUEUE ux_app_MsgQueue_UCPD;

__ALIGN_BEGIN USB_MODE_STATE USB_Device_EVENT __ALIGN_END;

int32_t USBPD_USBIF_Init(void)
{
	return 0;
}

void USBPD_USBIF_DeviceStart(uint32_t PortNum)
{
	(void)PortNum;

	USB_Device_EVENT = START_USB_DEVICE;
	if (tx_queue_send(&ux_app_MsgQueue_UCPD, &USB_Device_EVENT, TX_WAIT_FOREVER) != TX_SUCCESS)
	{
		Error_Handler();
	}
}

void USBPD_USBIF_DeviceStop(uint32_t PortNum)
{
	(void)PortNum;

	USB_Device_EVENT = STOP_USB_DEVICE;
	if (tx_queue_send(&ux_app_MsgQueue_UCPD, &USB_Device_EVENT, TX_WAIT_FOREVER) != TX_SUCCESS)
	{
		Error_Handler();
	}
}
