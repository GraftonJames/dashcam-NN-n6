/**
 ******************************************************************************
 * @file    usbpd_usb_if.h
 * @brief   USBPD <-> USBX data-role integration - Phase 4 (UVC port plan).
 ******************************************************************************
 * Adapted from ST's usbpd_usb_if_template.h (Drivers/STM32_USBPD_Library/Core/
 * inc/), trimmed to the subset this SNK-only (no DFP/host mode) port needs.
 ******************************************************************************
 */
#ifndef USBPD_USBIF_H_
#define USBPD_USBIF_H_

#ifdef __cplusplus
extern "C" {
#endif

int32_t USBPD_USBIF_Init(void);
void	USBPD_USBIF_DeviceStart(uint32_t PortNum);
void	USBPD_USBIF_DeviceStop(uint32_t PortNum);

#ifdef __cplusplus
}
#endif

#endif /* USBPD_USBIF_H_ */
