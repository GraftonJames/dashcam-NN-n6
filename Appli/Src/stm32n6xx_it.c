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
static void TRACE_RawPutChar(char c)
{
	uint32_t guard = 1000000U;
	while (((USART1->ISR & USART_ISR_TXE_TXFNF) == 0U) && (--guard != 0U))
	{
	}
	USART1->TDR = (uint32_t)c;
}

static void TRACE_RawPutString(const char *s)
{
	while (*s != '\0')
	{
		TRACE_RawPutChar(*s++);
	}
}

static void TRACE_RawPutHex32(uint32_t v)
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
