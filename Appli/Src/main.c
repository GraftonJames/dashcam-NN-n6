/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
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

#include "camera.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

COM_InitTypeDef	       BspCOMInit;
CACHEAXI_HandleTypeDef hcacheaxi;

UART_HandleTypeDef hlpuart1;

RAMCFG_HandleTypeDef hramcfg_SRAM3;
RAMCFG_HandleTypeDef hramcfg_SRAM4;
RAMCFG_HandleTypeDef hramcfg_SRAM5;
RAMCFG_HandleTypeDef hramcfg_SRAM6;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static void MX_GPIO_Init(void);
static void MX_CACHEAXI_Init(void);
static void MX_LPUART1_UART_Init(void);
static void MX_RAMCFG_Init(void);
static void SystemIsolation_Config(void);

static TX_THREAD dummy_thread;
static uint8_t	 dummy_thread_stack[DUMMY_THREAD_STACK_SIZE];

// tx threads
static void dummy_thread_entry(uint32_t initial_input)
{
	uint32_t counter = 0;
	(void)initial_input;
	while (1)
	{
		BSP_LED_Toggle(LED_RED);
		printf("dummy_thread: alive, counter=%lu\r\n", (unsigned long)counter++);
		tx_thread_sleep(20);
	}
}
/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{

	/* USER CODE BEGIN 1 */

	/* Clock tree is inherited from FSBL; recompute SystemCoreClock from the
	 * registers (matches ST's Template_FSBL_LRUN Appli). */
	SystemCoreClockUpdate();

	/* Enable CPU caches, as ST's template Appli does first thing (it also sets
	 * an MPU non-cacheable region over the linker's .noncacheable section - ours
	 * is currently EMPTY so that region is skipped here; it MUST be added when
	 * camera/DMA frame buffers are placed in .noncacheable, or D-cache will
	 * serve stale data for DMA-written memory). */
	SCB_EnableICache();
	SCB_EnableDCache();

	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/
	HAL_Init();

	/* USER CODE BEGIN Init */

	/* By default a Cortex-M55 lockup state (fault escalation failing, e.g. a fault
	 * while already in a same/higher-priority fault handler) is completely silent on
	 * this chip: no reset, no NMI, core just stops. Route it to NMI so it becomes
	 * observable instead of indistinguishable from any other silent hang.
	 * SystemInit() disables the SYSCFG clock again after using it, so re-enable it
	 * before touching any SYSCFG register - otherwise this write hangs the bus. */
	__HAL_RCC_SYSCFG_CLK_ENABLE();
	(void)RCC->APB4ENR2; /* delay: ensure the clock enable has taken effect */
	SYSCFG->CM55RSTCR |= SYSCFG_CM55RSTCR_LOCKUP_NMI_EN;

	/* USER CODE END Init */

	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_CACHEAXI_Init();
	MX_LPUART1_UART_Init();
	MX_RAMCFG_Init();
	SystemIsolation_Config();
	/* USER CODE BEGIN 2 */

	/* Initialize COM1 port (115200, 8 bits (7-bit data + 1 stop bit), no parity */
	BspCOMInit.BaudRate   = 115200;
	BspCOMInit.WordLength = COM_WORDLENGTH_8B;
	BspCOMInit.StopBits   = COM_STOPBITS_1;
	BspCOMInit.Parity     = COM_PARITY_NONE;
	BspCOMInit.HwFlowCtl  = COM_HWCONTROL_NONE;
	if (BSP_COM_Init(COM1, &BspCOMInit) != BSP_ERROR_NONE)
	{
		Error_Handler();
	}

	BSP_LED_Init(LED_GREEN);
	BSP_LED_Init(LED_RED);
	BSP_LED_Init(LED_BLUE);
	BSP_LED_On(LED_GREEN);

	printf("Appli: booting, SystemCoreClock=%lu\r\n", (unsigned long)SystemCoreClock);

	if (CAMERA_init() != HAL_OK)
	{
		Error_Handler();
	}
	BSP_LED_On(LED_BLUE); /* camera initialized */
	(void)first_unused_memory;
	tx_thread_create(&dummy_thread, "dummy_thread", dummy_thread_entry, 0,
			 dummy_thread_stack, DUMMY_THREAD_STACK_SIZE,
			 16, 16, TX_NO_TIME_SLICE, TX_AUTO_START);

	tx_kernel_enter();
}

/**
 * @brief CACHEAXI Initialization Function
 * @param None
 * @retval None
 */
static void MX_CACHEAXI_Init(void)
{

	/* USER CODE BEGIN CACHEAXI_Init 0 */

	/* USER CODE END CACHEAXI_Init 0 */

	/* USER CODE BEGIN CACHEAXI_Init 1 */

	/* USER CODE END CACHEAXI_Init 1 */
	hcacheaxi.Instance = CACHEAXI;
	if (HAL_CACHEAXI_Init(&hcacheaxi) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN CACHEAXI_Init 2 */

	/* USER CODE END CACHEAXI_Init 2 */
}

/**
 * @brief DCMIPP Clock Configuration Function, called by CMW_CAMERA_Init()
 * @param hdcmipp DCMIPP handle
 * @retval HAL status
 */
HAL_StatusTypeDef MX_DCMIPP_ClockConfig(DCMIPP_HandleTypeDef *hdcmipp)
{
	RCC_PeriphCLKInitTypeDef RCC_PeriphCLKInitStruct = {0};
	HAL_StatusTypeDef	 ret;

	UNUSED(hdcmipp);

	printf("TRACE: MX_DCMIPP_ClockConfig: configuring IC17/DCMIPP from PLL2\r\n");
	RCC_PeriphCLKInitStruct.PeriphClockSelection		     = RCC_PERIPHCLK_DCMIPP;
	RCC_PeriphCLKInitStruct.DcmippClockSelection		     = RCC_DCMIPPCLKSOURCE_IC17;
	RCC_PeriphCLKInitStruct.ICSelection[RCC_IC17].ClockSelection = RCC_ICCLKSOURCE_PLL2;
	RCC_PeriphCLKInitStruct.ICSelection[RCC_IC17].ClockDivider   = 3;
	ret							     = HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphCLKInitStruct);
	printf("TRACE: IC17/DCMIPP config returned %d\r\n", (int)ret);
	if (ret != HAL_OK)
	{
		return ret;
	}

	printf("TRACE: MX_DCMIPP_ClockConfig: configuring IC18/CSI from PLL1\r\n");
	RCC_PeriphCLKInitStruct.PeriphClockSelection		     = RCC_PERIPHCLK_CSI;
	RCC_PeriphCLKInitStruct.ICSelection[RCC_IC18].ClockSelection = RCC_ICCLKSOURCE_PLL1;
	RCC_PeriphCLKInitStruct.ICSelection[RCC_IC18].ClockDivider   = 60; /* PLL1 VCO is now 1200MHz (was 800): keep CSI kernel at 20MHz */
	ret							     = HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphCLKInitStruct);
	printf("TRACE: IC18/CSI config returned %d\r\n", (int)ret);
	return ret;
}

/**
 * @brief LPUART1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_LPUART1_UART_Init(void)
{

	/* USER CODE BEGIN LPUART1_Init 0 */

	/* USER CODE END LPUART1_Init 0 */

	/* USER CODE BEGIN LPUART1_Init 1 */

	/* USER CODE END LPUART1_Init 1 */
	hlpuart1.Instance		     = LPUART1;
	hlpuart1.Init.BaudRate		     = 209700;
	hlpuart1.Init.WordLength	     = UART_WORDLENGTH_8B;
	hlpuart1.Init.StopBits		     = UART_STOPBITS_1;
	hlpuart1.Init.Parity		     = UART_PARITY_NONE;
	hlpuart1.Init.Mode		     = UART_MODE_TX_RX;
	hlpuart1.Init.HwFlowCtl		     = UART_HWCONTROL_NONE;
	hlpuart1.Init.OneBitSampling	     = UART_ONE_BIT_SAMPLE_DISABLE;
	hlpuart1.Init.ClockPrescaler	     = UART_PRESCALER_DIV1;
	hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	hlpuart1.FifoMode		     = UART_FIFOMODE_DISABLE;
	if (HAL_UART_Init(&hlpuart1) != HAL_OK)
	{
		Error_Handler();
	}
	if (HAL_UARTEx_SetTxFifoThreshold(&hlpuart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
	{
		Error_Handler();
	}
	if (HAL_UARTEx_SetRxFifoThreshold(&hlpuart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
	{
		Error_Handler();
	}
	if (HAL_UARTEx_DisableFifoMode(&hlpuart1) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN LPUART1_Init 2 */

	/* USER CODE END LPUART1_Init 2 */
}

/**
 * @brief RAMCFG Initialization Function
 * @param None
 * @retval None
 */
static void MX_RAMCFG_Init(void)
{

	/* USER CODE BEGIN RAMCFG_Init 0 */

	/* USER CODE END RAMCFG_Init 0 */

	/* USER CODE BEGIN RAMCFG_Init 1 */

	/* USER CODE END RAMCFG_Init 1 */

	/** Initialize RAMCFG SRAM3
	 */
	hramcfg_SRAM3.Instance = RAMCFG_SRAM3_AXI;
	if (HAL_RAMCFG_Init(&hramcfg_SRAM3) != HAL_OK)
	{
		Error_Handler();
	}

	/** Initialize RAMCFG SRAM4
	 */
	hramcfg_SRAM4.Instance = RAMCFG_SRAM4_AXI;
	if (HAL_RAMCFG_Init(&hramcfg_SRAM4) != HAL_OK)
	{
		Error_Handler();
	}

	/** Initialize RAMCFG SRAM5
	 */
	hramcfg_SRAM5.Instance = RAMCFG_SRAM5_AXI;
	if (HAL_RAMCFG_Init(&hramcfg_SRAM5) != HAL_OK)
	{
		Error_Handler();
	}

	/** Initialize RAMCFG SRAM6
	 */
	hramcfg_SRAM6.Instance = RAMCFG_SRAM6_AXI;
	if (HAL_RAMCFG_Init(&hramcfg_SRAM6) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN RAMCFG_Init 2 */

	/* USER CODE END RAMCFG_Init 2 */
}

/**
 * @brief RIF Initialization Function
 * @param None
 * @retval None
 */
static void SystemIsolation_Config(void)
{

	/* USER CODE BEGIN RIF_Init 0 */

	/* USER CODE END RIF_Init 0 */

	/* set all required IPs as secure privileged */
	__HAL_RCC_RIFSC_CLK_ENABLE();

	/*RIMC configuration*/
	RIMC_MasterConfig_t RIMC_master = {0};
	RIMC_master.MasterCID		= RIF_CID_1;
	RIMC_master.SecPriv		= RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_NPRIV;
	HAL_RIF_RIMC_ConfigMasterAttributes(RIF_MASTER_INDEX_DCMIPP, &RIMC_master);

	RIMC_master.MasterCID = RIF_CID_0;
	HAL_RIF_RIMC_ConfigMasterAttributes(RIF_MASTER_INDEX_ETH1, &RIMC_master);

	/* RIF-Aware IPs Config */

	/* set up GPIO configuration */
	__HAL_RCC_GPIOO_CLK_ENABLE();							/* needed for NRST_CAM SECCFGR access below; GPIOA already enabled by MX_GPIO_Init */
	HAL_GPIO_ConfigPinAttributes(GPIOA, GPIO_PIN_0, GPIO_PIN_SEC | GPIO_PIN_NPRIV); /* EN_CAM (camera enable pin) */
	HAL_GPIO_ConfigPinAttributes(GPIOO, GPIO_PIN_5, GPIO_PIN_SEC | GPIO_PIN_NPRIV); /* NRST_CAM (camera reset pin) */
	HAL_GPIO_ConfigPinAttributes(GPIOA, GPIO_PIN_9, GPIO_PIN_SEC | GPIO_PIN_NPRIV);
	HAL_GPIO_ConfigPinAttributes(GPIOA, GPIO_PIN_10, GPIO_PIN_SEC | GPIO_PIN_NPRIV);
	HAL_GPIO_ConfigPinAttributes(GPIOA, GPIO_PIN_11, GPIO_PIN_SEC | GPIO_PIN_NPRIV);
	HAL_GPIO_ConfigPinAttributes(GPIOB, GPIO_PIN_0, GPIO_PIN_SEC | GPIO_PIN_NPRIV);
	HAL_GPIO_ConfigPinAttributes(GPIOB, GPIO_PIN_3, GPIO_PIN_SEC | GPIO_PIN_NPRIV);
	HAL_GPIO_ConfigPinAttributes(GPIOB, GPIO_PIN_6, GPIO_PIN_SEC | GPIO_PIN_NPRIV);
	HAL_GPIO_ConfigPinAttributes(GPIOB, GPIO_PIN_7, GPIO_PIN_SEC | GPIO_PIN_NPRIV);
	HAL_GPIO_ConfigPinAttributes(GPIOB, GPIO_PIN_12, GPIO_PIN_SEC | GPIO_PIN_NPRIV);
	HAL_GPIO_ConfigPinAttributes(GPIOC, GPIO_PIN_5, GPIO_PIN_SEC | GPIO_PIN_NPRIV);
	HAL_GPIO_ConfigPinAttributes(GPIOC, GPIO_PIN_6, GPIO_PIN_SEC | GPIO_PIN_NPRIV);
	HAL_GPIO_ConfigPinAttributes(GPIOD, GPIO_PIN_5, GPIO_PIN_SEC | GPIO_PIN_NPRIV);
	HAL_GPIO_ConfigPinAttributes(GPIOD, GPIO_PIN_7, GPIO_PIN_SEC | GPIO_PIN_NPRIV);
	HAL_GPIO_ConfigPinAttributes(GPIOE, GPIO_PIN_3, GPIO_PIN_SEC | GPIO_PIN_NPRIV);
	HAL_GPIO_ConfigPinAttributes(GPIOE, GPIO_PIN_4, GPIO_PIN_SEC | GPIO_PIN_NPRIV);
	HAL_GPIO_ConfigPinAttributes(GPIOE, GPIO_PIN_8, GPIO_PIN_SEC | GPIO_PIN_NPRIV);
	HAL_GPIO_ConfigPinAttributes(GPIOE, GPIO_PIN_10, GPIO_PIN_SEC | GPIO_PIN_NPRIV);
	HAL_GPIO_ConfigPinAttributes(GPIOF, GPIO_PIN_1, GPIO_PIN_SEC | GPIO_PIN_NPRIV);
	HAL_GPIO_ConfigPinAttributes(GPIOF, GPIO_PIN_5, GPIO_PIN_SEC | GPIO_PIN_NPRIV);

	/* USER CODE BEGIN RIF_Init 1 */

	/* USER CODE END RIF_Init 1 */
	/* USER CODE BEGIN RIF_Init 2 */

	/* USER CODE END RIF_Init 2 */
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
	/* USER CODE BEGIN MX_GPIO_Init_1 */

	/* USER CODE END MX_GPIO_Init_1 */

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOE_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOF_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();

	/* USER CODE BEGIN MX_GPIO_Init_2 */

	/* USER CODE END MX_GPIO_Init_2 */
}

void tx_application_define(void *first_unused_memory)
{
	void *memory_ptr = first_unused_memory;
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1)
	{
	}
	/* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
	/* USER CODE BEGIN 6 */
	/* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
