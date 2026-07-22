/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    stm32n6xx_it.c
 * @brief   Interrupt Service Routines.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "stm32n6xx_it.h"
#include "main.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usbpd_hw_if.h"
#include "stm32n6xx_nucleo_usbpd_pwr.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
static void TRACE_FaultSafe(const char *handlerName);
extern TIM_HandleTypeDef htim6;
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
 * @brief This function handles Non maskable interrupt.
 */
void NMI_Handler(void)
{
	/* USER CODE BEGIN NonMaskableInt_IRQn 0 */
	/* Reached only if SYSCFG_CM55RSTCR.LOCKUP_NMI_EN routed a core lockup here. */
	TRACE_FaultSafe("NMI_Handler (possible lockup)");
	/* USER CODE END NonMaskableInt_IRQn 0 */
	/* USER CODE BEGIN NonMaskableInt_IRQn 1 */
	while (1)
	{
		GPIOG->ODR ^= GPIO_PIN_10;
		for (volatile uint32_t d = 0; d < 2000000U; d++)
		{
		}
	}
	/* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
 * @brief  Blocking, bounded, register-only UART byte send. No HAL, no tick
 *         dependency: every wait is instruction-count bounded so this can never
 *         hang forever, unlike HAL_UART_Transmit()'s HAL_GetTick()-based timeout,
 *         which never expires if called from a handler priority SysTick can't
 *         preempt (i.e. every fault handler here) and the flag never sets.
 */
void TRACE_RawPutChar(char c)
{
	uint32_t guard = 1000000U;
	while (((USART1->ISR & USART_ISR_TXE_TXFNF) == 0U) && (--guard != 0U))
	{
	}
	USART1->TDR = (uint32_t)c;
}

void TRACE_RawPutString(const char *s)
{
	while (*s != '\0')
	{
		TRACE_RawPutChar(*s++);
	}
}

void TRACE_RawPutHex32(uint32_t v)
{
	static const char hexdigits[16] = "0123456789ABCDEF";
	for (int shift = 28; shift >= 0; shift -= 4)
	{
		TRACE_RawPutChar(hexdigits[(v >> shift) & 0xFU]);
	}
}

/**
 * @brief  Fault-safe indicator: lights LED_RED first (visible even if UART
 *         itself is wedged), then dumps fault status/address registers plus
 *         ICSR (active exception number) over UART using only raw, bounded
 *         register polling.
 */
static void TRACE_FaultSafe(const char *handlerName)
{
	__HAL_RCC_GPIOG_CLK_ENABLE();
	GPIOG->BSRR = GPIO_PIN_10; /* LED_RED on */

	uint32_t cfsr  = *(volatile uint32_t *)0xE000ED28U;
	uint32_t hfsr  = *(volatile uint32_t *)0xE000ED2CU;
	uint32_t shcsr = *(volatile uint32_t *)0xE000ED24U;
	uint32_t mmfar = *(volatile uint32_t *)0xE000ED34U;
	uint32_t bfar  = *(volatile uint32_t *)0xE000ED38U;
	uint32_t sfsr  = *(volatile uint32_t *)0xE000EDE4U;
	uint32_t sfar  = *(volatile uint32_t *)0xE000EDE8U;
	uint32_t icsr  = SCB->ICSR;

	TRACE_RawPutString("\r\nRAWFAULT: ");
	TRACE_RawPutString(handlerName);
	TRACE_RawPutString(" CFSR=");
	TRACE_RawPutHex32(cfsr);
	TRACE_RawPutString(" HFSR=");
	TRACE_RawPutHex32(hfsr);
	TRACE_RawPutString(" SHCSR=");
	TRACE_RawPutHex32(shcsr);
	TRACE_RawPutString(" MMFAR=");
	TRACE_RawPutHex32(mmfar);
	TRACE_RawPutString(" BFAR=");
	TRACE_RawPutHex32(bfar);
	TRACE_RawPutString(" SFSR=");
	TRACE_RawPutHex32(sfsr);
	TRACE_RawPutString(" SFAR=");
	TRACE_RawPutHex32(sfar);
	TRACE_RawPutString(" ICSR=");
	TRACE_RawPutHex32(icsr);
	TRACE_RawPutString("\r\n");
}

/**
 * @brief This function handles Memory management fault.
 */
void MemManage_Handler(void)
{
	/* USER CODE BEGIN MemoryManagement_IRQn 0 */
	TRACE_FaultSafe("MemManage_Handler");
	/* USER CODE END MemoryManagement_IRQn 0 */
	while (1)
	{
		/* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
		GPIOG->ODR ^= GPIO_PIN_10;
		for (volatile uint32_t d = 0; d < 2000000U; d++)
		{
		}
		/* USER CODE END W1_MemoryManagement_IRQn 0 */
	}
}

/**
 * @brief This function handles Prefetch fault, memory access fault.
 */
void BusFault_Handler(void)
{
	/* USER CODE BEGIN BusFault_IRQn 0 */
	TRACE_FaultSafe("BusFault_Handler");
	/* USER CODE END BusFault_IRQn 0 */
	while (1)
	{
		/* USER CODE BEGIN W1_BusFault_IRQn 0 */
		GPIOG->ODR ^= GPIO_PIN_10;
		for (volatile uint32_t d = 0; d < 2000000U; d++)
		{
		}
		/* USER CODE END W1_BusFault_IRQn 0 */
	}
}

/**
 * @brief This function handles Secure fault.
 */
void SecureFault_Handler(void)
{
	/* USER CODE BEGIN SecureFault_IRQn 0 */
	TRACE_FaultSafe("SecureFault_Handler");
	/* USER CODE END SecureFault_IRQn 0 */
	while (1)
	{
		/* USER CODE BEGIN W1_SecureFault_IRQn 0 */
		GPIOG->ODR ^= GPIO_PIN_10;
		for (volatile uint32_t d = 0; d < 2000000U; d++)
		{
		}
		/* USER CODE END W1_SecureFault_IRQn 0 */
	}
}

/**
 * @brief This function handles Debug monitor.
 */
void DebugMon_Handler(void)
{
	/* USER CODE BEGIN DebugMonitor_IRQn 0 */

	/* USER CODE END DebugMonitor_IRQn 0 */
	/* USER CODE BEGIN DebugMonitor_IRQn 1 */

	/* USER CODE END DebugMonitor_IRQn 1 */
}

/******************************************************************************/
/* STM32N6xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32n6xx.s).                    */
/******************************************************************************/

/* USER CODE BEGIN 1 */

/**
 * @brief This function handles TIM6 global interrupt (HAL timebase tick,
 *        used instead of SysTick since ThreadX owns SysTick).
 */
void TIM6_IRQHandler(void)
{
	HAL_TIM_IRQHandler(&htim6);
}

/**
 * @brief This function handles UCPD1 global interrupt.
 * @note  Without this, UCPD1_IRQn (armed by MX_UCPD1_Init()) falls through to
 *        the default weak handler - a silent infinite loop - the moment a
 *        real Type-C CC event fires. Same failure class as the July 2026
 *        unhandled-EXTI hang.
 */
void UCPD1_IRQHandler(void)
{
	USBPD_PORT0_IRQHandler();
}

extern DCMIPP_HandleTypeDef hcamera_dcmipp; /* Drivers/cmw_camera/cmw_camera.c */

/**
 * @brief This function handles CSI global interrupt.
 * @note  Same failure class as UCPD1/the July 2026 unhandled-EXTI hang:
 *        CSI_IRQn is never explicitly "armed" by our own code, but the
 *        DCMIPP/CSI receiver hardware starts generating it the moment real
 *        capture begins (Phase 3's VENC pipe) - without a real handler here
 *        it falls through to Default_Handler (silent while(1)) the instant
 *        the first interrupt fires, which is exactly what happened: capture
 *        started, VENC's H264EncInit succeeded, and the very next CSI event
 *        froze the board with no fault, no further UART output. ST's own
 *        HAL_DCMIPP_CSI_IRQHandler() doc comment says it must be called from
 *        here.
 */
void CSI_IRQHandler(void)
{
	HAL_DCMIPP_CSI_IRQHandler(&hcamera_dcmipp);
}

/**
 * @brief This function handles DCMIPP global interrupt (frame-complete etc.)
 * @note  Same reasoning as CSI_IRQHandler above - also unhandled, also armed
 *        implicitly by starting real capture.
 */
void DCMIPP_IRQHandler(void)
{
	HAL_DCMIPP_IRQHandler(&hcamera_dcmipp);
}

extern PCD_HandleTypeDef hpcd_USB1_OTG_HS;

/**
 * @brief This function handles USB1 OTG HS global interrupt.
 * @note  Same reasoning as UCPD1_IRQHandler above: USB1_OTG_HS_IRQn gets
 *        armed by HAL_PCD_MspInit() (Phase 4) the moment enumeration starts -
 *        without a real handler it falls through to Default_Handler.
 * @note  DIAG (2026-07-22, enumeration debug): edge-triggered (only logs when
 *        GINTSTS.[RXFLVL|USBRST|ENUMDNE|IEPINT|OEPINT] changes from the last
 *        logged value, masking off the free-running SOF/FNSOF noise), bounded
 *        (first 200 distinct states) ISR-safe raw trace - printf() is
 *        unsafe/unreliable here (not ISR-safe, and observed dropping lines
 *        under concurrent CAD/PE thread access elsewhere this session), so
 *        this reuses the same raw/bounded UART primitives TRACE_FaultSafe()
 *        uses. A prior plain "first 60 raw entries" version exhausted its cap
 *        within ~7ms of pure SOF ticks (RXFLVL was 0 in every one) and never
 *        observed the actual SETUP exchange, which happens later.
 */
void USB1_OTG_HS_IRQHandler(void)
{
	static volatile uint32_t otg_irq_count = 0;
	static uint32_t	  last_masked	   = 0xFFFFFFFFU;
	uint32_t	  gintsts	   = USB1_OTG_HS->GINTSTS;
	uint32_t	  masked =
		gintsts & (USB_OTG_GINTSTS_RXFLVL | USB_OTG_GINTSTS_USBRST | USB_OTG_GINTSTS_ENUMDNE |
			   USB_OTG_GINTSTS_IEPINT | USB_OTG_GINTSTS_OEPINT);

	uint8_t should_log = ((masked != last_masked) && (otg_irq_count < 200U)) ? 1U : 0U;
	uint8_t had_oepint = ((masked & USB_OTG_GINTSTS_OEPINT) != 0U) ? 1U : 0U;
	uint8_t had_iepint = ((masked & USB_OTG_GINTSTS_IEPINT) != 0U) ? 1U : 0U;

	/* DIAG: raw per-endpoint interrupt/size registers, read BEFORE
	 * HAL_PCD_IRQHandler processes+clears them - GINTSTS.OEPINT/IEPINT are
	 * just aggregates ("some OUT/IN endpoint has *a* pending condition");
	 * DOEPINT0/DIEPINT0 say exactly which one (XFRC/STUP/AHBErr/NAK/...),
	 * and DOEPTSIZ0's STUPCNT/XFRSIZ fields say how many SETUP packets and
	 * bytes the core itself thinks are outstanding. A prior version of this
	 * diagnostic read hpcd->Setup AFTER the HAL call and always saw zero -
	 * traced USB_EP0_OutStart() (stm32n6xx_ll_usb.c) and confirmed it never
	 * writes the buffer's contents (only DOEPDMA/DOEPCTL registers), so that
	 * emptiness needs an explanation from the endpoint's own interrupt/size
	 * state, not assumed to be a rearm side-effect. */
	USB_OTG_OUTEndpointTypeDef *otg_out0 =
		(USB_OTG_OUTEndpointTypeDef *)((uint32_t)USB1_OTG_HS + USB_OTG_OUT_ENDPOINT_BASE);
	uint32_t doepint0  = otg_out0->DOEPINT;
	uint32_t doeptsiz0 = otg_out0->DOEPTSIZ;

	/* DIAG (2026-07-22, streaming-stall debug): SETUP/enumeration is fixed
	 * (DMA disabled - see MX_USB1_OTG_HS_PCD_Init() in main.c); now chasing
	 * why the video IN endpoint (EP1) stops after ~5 payloads (one fragmented
	 * keyframe) - video_write_payload()'s own counter (ux_device_video.c)
	 * confirms USBX stops calling us, but not whether the OTG hardware ever
	 * raises a 6th IN-complete at all. DIEPINT1 says definitively whether
	 * this is a HW-level stall (no further IN interrupt ever) or a
	 * USBX-class-layer issue (HW completes fine, class layer just doesn't
	 * relay it to our StreamPayloadDone callback). */
	USB_OTG_INEndpointTypeDef *otg_in1 =
		(USB_OTG_INEndpointTypeDef *)((uint32_t)USB1_OTG_HS + USB_OTG_IN_ENDPOINT_BASE + USB_OTG_EP_REG_SIZE);
	uint32_t diepint1  = otg_in1->DIEPINT;
	uint32_t dieptsiz1 = otg_in1->DIEPTSIZ;

	if (should_log != 0U)
	{
		USB_OTG_DeviceTypeDef *otg_dev =
			(USB_OTG_DeviceTypeDef *)((uint32_t)USB1_OTG_HS + USB_OTG_DEVICE_BASE);

		last_masked = masked;
		otg_irq_count++;
		TRACE_RawPutString("OTGIRQ #");
		TRACE_RawPutHex32(otg_irq_count);
		TRACE_RawPutString(" GINTSTS=");
		TRACE_RawPutHex32(gintsts);
		TRACE_RawPutString(" DSTS=");
		TRACE_RawPutHex32(otg_dev->DSTS);
		if (had_oepint != 0U)
		{
			TRACE_RawPutString(" DOEPINT0=");
			TRACE_RawPutHex32(doepint0);
			TRACE_RawPutString(" DOEPTSIZ0=");
			TRACE_RawPutHex32(doeptsiz0);
		}
		if (had_iepint != 0U)
		{
			TRACE_RawPutString(" DIEPINT1=");
			TRACE_RawPutHex32(diepint1);
			TRACE_RawPutString(" DIEPTSIZ1=");
			TRACE_RawPutHex32(dieptsiz1);
		}
		TRACE_RawPutString("\r\n");
	}

	HAL_PCD_IRQHandler(&hpcd_USB1_OTG_HS);
}

#if defined(TCPP0203_SUPPORT)
/**
 * @brief This function handles the TCPP0203's FLG/alert line (PD2 -> EXTI2
 *        on this board; TCPP0203_PORT0_FLG_EXTI_IRQHANDLER expands to
 *        EXTI2_IRQHandler via stm32n6xx_nucleo_usbpd_pwr.h).
 */
void TCPP0203_PORT0_FLG_EXTI_IRQHANDLER(void)
{
	if (TCPP0203_PORT0_FLG_EXTI_IS_ACTIVE_FLAG() != RESET)
	{
		BSP_USBPD_PWR_EventCallback(USBPD_PWR_TYPE_C_PORT_1);
		TCPP0203_PORT0_FLG_EXTI_CLEAR_FLAG();
	}
}
#endif /* TCPP0203_SUPPORT */

/* USER CODE END 1 */
