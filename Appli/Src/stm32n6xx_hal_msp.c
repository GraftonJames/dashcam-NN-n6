/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file         stm32n6xx_hal_msp.c
 * @brief        This file provides code for the MSP Initialization
 *               and de-Initialization codes.
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
#include "main.h"
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN Define */

/* USER CODE END Define */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN Macro */

/* USER CODE END Macro */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* External functions --------------------------------------------------------*/
/* USER CODE BEGIN ExternalFunctions */

/* USER CODE END ExternalFunctions */

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */
/**
 * Initializes the Global MSP.
 */
void HAL_MspInit(void)
{

	/* USER CODE BEGIN MspInit 0 */

	/* USER CODE END MspInit 0 */

	/* System interrupt init*/

	HAL_PWREx_EnableVddIO2();

	HAL_PWREx_EnableVddIO3();

	HAL_PWREx_EnableVddIO4();

	HAL_PWREx_EnableVddIO5();

	/* USER CODE BEGIN MspInit 1 */

	/* USER CODE END MspInit 1 */
}

/**
 * @brief CACHEAXI MSP Initialization
 * This function configures the hardware resources used in this example
 * @param hcacheaxi: CACHEAXI handle pointer
 * @retval None
 */
void HAL_CACHEAXI_MspInit(CACHEAXI_HandleTypeDef *hcacheaxi)
{
	if (hcacheaxi->Instance == CACHEAXI)
	{
		/* USER CODE BEGIN CACHEAXI_MspInit 0 */

		/* USER CODE END CACHEAXI_MspInit 0 */
		/* Peripheral clock enable */
		__HAL_RCC_CACHEAXI_CLK_ENABLE();
		/* USER CODE BEGIN CACHEAXI_MspInit 1 */

		/* USER CODE END CACHEAXI_MspInit 1 */
	}
}

/**
 * @brief CACHEAXI MSP De-Initialization
 * This function freeze the hardware resources used in this example
 * @param hcacheaxi: CACHEAXI handle pointer
 * @retval None
 */
void HAL_CACHEAXI_MspDeInit(CACHEAXI_HandleTypeDef *hcacheaxi)
{
	if (hcacheaxi->Instance == CACHEAXI)
	{
		/* USER CODE BEGIN CACHEAXI_MspDeInit 0 */

		/* USER CODE END CACHEAXI_MspDeInit 0 */
		/* Peripheral clock disable */
		__HAL_RCC_CACHEAXI_CLK_DISABLE();
		/* USER CODE BEGIN CACHEAXI_MspDeInit 1 */

		/* USER CODE END CACHEAXI_MspDeInit 1 */
	}
}

/**
 * @brief UART MSP Initialization
 * This function configures the hardware resources used in this example
 * @param huart: UART handle pointer
 * @retval None
 */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
	GPIO_InitTypeDef	 GPIO_InitStruct     = {0};
	RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
	if (huart->Instance == LPUART1)
	{
		/* USER CODE BEGIN LPUART1_MspInit 0 */

		/* USER CODE END LPUART1_MspInit 0 */

		/** Initializes the peripherals clock
		 */
		PeriphClkInitStruct.PeriphClockSelection  = RCC_PERIPHCLK_LPUART1;
		PeriphClkInitStruct.Lpuart1ClockSelection = RCC_LPUART1CLKSOURCE_PCLK4;
		if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
		{
			Error_Handler();
		}

		/* Peripheral clock enable */
		__HAL_RCC_LPUART1_CLK_ENABLE();

		__HAL_RCC_GPIOA_CLK_ENABLE();
		/**LPUART1 GPIO Configuration
		PA9     ------> LPUART1_TX
		PA10     ------> LPUART1_RX
		(PA11 CTS deliberately NOT claimed here: HwFlowCtl is NONE so it was never
		 functionally used, and Phase 2 needs PA11 free for the TCPP0203's
		 VBUS-sense ADC input.)
		*/
		GPIO_InitStruct.Pin	  = GPIO_PIN_9 | GPIO_PIN_10;
		GPIO_InitStruct.Mode	  = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull	  = GPIO_NOPULL;
		GPIO_InitStruct.Speed	  = GPIO_SPEED_FREQ_LOW;
		GPIO_InitStruct.Alternate = GPIO_AF3_LPUART1;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

		/* USER CODE BEGIN LPUART1_MspInit 1 */

		/* USER CODE END LPUART1_MspInit 1 */
	}
}

/**
 * @brief UART MSP De-Initialization
 * This function freeze the hardware resources used in this example
 * @param huart: UART handle pointer
 * @retval None
 */
void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
	if (huart->Instance == LPUART1)
	{
		/* USER CODE BEGIN LPUART1_MspDeInit 0 */

		/* USER CODE END LPUART1_MspDeInit 0 */
		/* Peripheral clock disable */
		__HAL_RCC_LPUART1_CLK_DISABLE();

		/**LPUART1 GPIO Configuration
		PA9     ------> LPUART1_TX
		PA10     ------> LPUART1_RX
		*/
		HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9 | GPIO_PIN_10);

		/* USER CODE BEGIN LPUART1_MspDeInit 1 */

		/* USER CODE END LPUART1_MspDeInit 1 */
	}
}

/**
 * @brief RAMCFG MSP Initialization
 * This function configures the hardware resources used in this example
 * @param hramcfg: RAMCFG handle pointer
 * @retval None
 */
void HAL_RAMCFG_MspInit(RAMCFG_HandleTypeDef *hramcfg)
{
	/* USER CODE BEGIN RAMCFG_MspInit 0 */

	/* USER CODE END RAMCFG_MspInit 0 */
	/* Peripheral clock enable */
	__HAL_RCC_RAMCFG_CLK_ENABLE();
	/* USER CODE BEGIN RAMCFG_MspInit 1 */

	/* USER CODE END RAMCFG_MspInit 1 */
}

/**
 * @brief RAMCFG MSP De-Initialization
 * This function freeze the hardware resources used in this example
 * @param hramcfg: RAMCFG handle pointer
 * @retval None
 */
void HAL_RAMCFG_MspDeInit(RAMCFG_HandleTypeDef *hramcfg)
{
	/* USER CODE BEGIN RAMCFG_MspDeInit 0 */

	/* USER CODE END RAMCFG_MspDeInit 0 */
	/* Peripheral clock disable */
	__HAL_RCC_RAMCFG_CLK_DISABLE();
	/* USER CODE BEGIN RAMCFG_MspDeInit 1 */

	/* USER CODE END RAMCFG_MspDeInit 1 */
}

/* USER CODE BEGIN 1 */

/**
 * @brief PCD MSP Initialization (USB1_OTG_HS device controller)
 * @param hpcd: PCD handle pointer
 * @retval None
 * @note   Nucleo's X3 crystal is 48MHz (UM3417 7.7.2, R72/R67 ON by default) -
 *         same HSE frequency VENC_USB's DK reference uses for its USB PHY
 *         clock, so RCC_USBPHY1CLKSOURCE_HSE_DIRECT is directly applicable
 *         here too, not just copied blind. HSE was never turned on before
 *         this (the whole clock tree up to now is HSI-derived, see FSBL's
 *         SystemClock_Config) - enabled here, scoped to just this peripheral,
 *         rather than touching FSBL's proven clock bring-up.
 * @note   2026-07-22 (Phase 4 first hardware test): board attached and got a
 *         USB address from the host but every GET_DESCRIPTOR read failed
 *         (dmesg: "device descriptor read/64, error -71", "Device not
 *         responding to setup address") - a bit-level PHY timing fault, not
 *         a descriptor-table bug. Root cause: this function was missing the
 *         embedded HS PHY's reset + reference-clock-select sequence that
 *         ST's VENC_USB reference (stm32n6xx_hal_msp.c) performs - in
 *         particular USB1_HS_PHYC->USBPHYC_CR's FSEL bits[6:4], which select
 *         the PHY-internal PLL's input reference divider. Left at its
 *         power-on-reset default, the PHY's internal bit clock doesn't match
 *         our real 48MHz HSE, so chirp/attach (coarse timing) still works
 *         but real 480Mbps HS transfers don't - exactly this symptom. Added
 *         the full force-reset -> HSEDiv2 select -> clock enable -> FSEL
 *         program -> phy-reset-release -> delay -> core-reset-release
 *         sequence below, matching the reference exactly (same 48MHz HSE).
 */
void HAL_PCD_MspInit(PCD_HandleTypeDef *hpcd)
{
	RCC_OscInitTypeDef	 RCC_OscInitStruct    = {0};
	RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

	if (hpcd->Instance == USB1_OTG_HS)
	{
		RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
		RCC_OscInitStruct.HSEState	  = RCC_HSE_ON;
		RCC_OscInitStruct.PLL1.PLLState  = RCC_PLL_NONE;
		if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
		{
			Error_Handler();
		}

		PeriphClkInitStruct.PeriphClockSelection    = RCC_PERIPHCLK_USBOTGHS1;
		PeriphClkInitStruct.UsbOtgHs1ClockSelection = RCC_USBOTGHS1CLKSOURCE_HSE_DIRECT;
		if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
		{
			Error_Handler();
		}

		PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USBPHY1;
		PeriphClkInitStruct.UsbPhy1ClockSelection = RCC_USBPHY1CLKSOURCE_HSE_DIRECT;
		if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
		{
			Error_Handler();
		}

		__HAL_RCC_PWR_CLK_ENABLE();
		HAL_PWREx_EnableVddUSBVMEN();
		{
			uint32_t guard = 1000000U;
			while ((__HAL_PWR_GET_FLAG(PWR_FLAG_USB33RDY) == 0U) && (--guard != 0U))
			{
			}
		}
		HAL_PWREx_EnableVddUSB();

		__HAL_RCC_GPIOA_CLK_ENABLE();

		/* Hold OTG core + PHY in reset while the PHY reference clock is
		 * (re)selected - programming USBPHYC_CR live, without this, leaves
		 * the PHY briefly running on a stale/undefined reference. */
		__HAL_RCC_USB1_OTG_HS_FORCE_RESET();
		__HAL_RCC_USB1_OTG_HS_PHY_FORCE_RESET();

		LL_RCC_HSE_SelectHSEDiv2AsDiv2Clock();

		__HAL_RCC_USB1_OTG_HS_CLK_ENABLE();

		/* USBPHYC_CR FSEL[2:0] (bits 6:4) = 0x2: PHY-internal PLL reference
		 * divider matching a 48MHz HSE input (RM0486) - the actual fix. */
		USB1_HS_PHYC->USBPHYC_CR &= ~(0x7UL << 4);
		USB1_HS_PHYC->USBPHYC_CR |= (0x2UL << 4);

		__HAL_RCC_USB1_OTG_HS_PHY_RELEASE_RESET();
		HAL_Delay(1);
		__HAL_RCC_USB1_OTG_HS_RELEASE_RESET();

		__HAL_RCC_USB1_OTG_HS_PHY_CLK_ENABLE();

		HAL_NVIC_SetPriority(USB1_OTG_HS_IRQn, 10, 0);
		HAL_NVIC_EnableIRQ(USB1_OTG_HS_IRQn);
	}
}

/**
 * @brief PCD MSP De-Initialization
 * @param hpcd: PCD handle pointer
 * @retval None
 */
void HAL_PCD_MspDeInit(PCD_HandleTypeDef *hpcd)
{
	if (hpcd->Instance == USB1_OTG_HS)
	{
		HAL_NVIC_DisableIRQ(USB1_OTG_HS_IRQn);
		__HAL_RCC_USB1_OTG_HS_PHY_CLK_DISABLE();
		__HAL_RCC_USB1_OTG_HS_CLK_DISABLE();
	}
}

/* USER CODE END 1 */
