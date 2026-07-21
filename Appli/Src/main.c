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
#include "tx_api.h"
#include "usbpd.h"
#include "stm32n6xx_ll_bus.h"
#include "stm32n6xx_ll_dma.h"
#include "stm32n6xx_ll_venc.h"

void VENC_APP_ThreadCreate(void); /* venc_app.c */

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define USBPD_DEVICE_APP_MEM_POOL_SIZE 5000

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
static void MX_UCPD1_Init(void);
static void SystemIsolation_Config(void);
static void MPU_Config(void);

static TX_BYTE_POOL usbpd_app_byte_pool;
static UCHAR	    usbpd_byte_pool_buffer[USBPD_DEVICE_APP_MEM_POOL_SIZE];

static TX_THREAD heartbeat_thread;
static uint8_t	 heartbeat_thread_stack[HEARTBEAT_THREAD_STACK_SIZE];

static void heartbeat_thread_entry(ULONG initial_input)
{
	(void)initial_input;
	while (1)
	{
		BSP_LED_Toggle(LED_RED);
		tx_thread_sleep(20);
	}
}

void tx_application_define(void *first_unused_memory)
{
	(void)first_unused_memory;
	printf("DIAG: tx_application_define() STARTED\r\n");
	tx_thread_create(&heartbeat_thread, "heartbeat_thread", heartbeat_thread_entry, 0,
			 heartbeat_thread_stack, HEARTBEAT_THREAD_STACK_SIZE,
			 16, 16, TX_NO_TIME_SLICE, TX_AUTO_START);

	if (tx_byte_pool_create(&usbpd_app_byte_pool, "USBPD App memory pool",
				 usbpd_byte_pool_buffer, USBPD_DEVICE_APP_MEM_POOL_SIZE) != TX_SUCCESS)
	{
		Error_Handler();
	}
	printf("DIAG: usbpd_app_byte_pool created, calling MX_USBPD_Init()\r\n");
	if (MX_USBPD_Init(&usbpd_app_byte_pool) != USBPD_OK)
	{
		printf("DIAG: MX_USBPD_Init() FAILED\r\n");
		Error_Handler();
	}
	printf("DIAG: MX_USBPD_Init() OK\r\n");

	VENC_APP_ThreadCreate();
	printf("DIAG: VENC_APP_ThreadCreate() done, tx_application_define() returning\r\n");
}
/**
 * @brief DIAG: encode a checkpoint number 1-7 as a 3-bit LED pattern
 *        (GREEN=4, RED=2, BLUE=1), so we can see how far boot got even if it
 *        hangs before UART is initialized. TEMPORARY - remove once the Phase
 *        2/3 boot hang is root-caused. BSP_LED_Init()/On()/Off() each handle
 *        their own GPIOG clock enable, so this is safe to call before
 *        MX_GPIO_Init() has run.
 */
static void DIAG_LedCode(int code)
{
	BSP_LED_Init(LED_GREEN);
	BSP_LED_Init(LED_RED);
	BSP_LED_Init(LED_BLUE);
	(code & 4) ? BSP_LED_On(LED_GREEN) : BSP_LED_Off(LED_GREEN);
	(code & 2) ? BSP_LED_On(LED_RED) : BSP_LED_Off(LED_RED);
	(code & 1) ? BSP_LED_On(LED_BLUE) : BSP_LED_Off(LED_BLUE);
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
	DIAG_LedCode(1); /* checkpoint 1: reached after SystemCoreClockUpdate() */

	/* MPU non-cacheable region over the linker's .noncacheable section, BEFORE
	 * enabling caches. Phase 3 places DCMIPP capture buffers and VENC's
	 * encoder pool there (see camera.c / venc_h264_config.c) - without this,
	 * D-cache would silently serve stale data for DMA-written memory instead
	 * of a crash, which looks exactly like a video/encoder bug, not a cache
	 * bug (flagged as the single highest-severity item in the Phase 3 plan). */
	MPU_Config();
	DIAG_LedCode(2); /* checkpoint 2: reached after MPU_Config() */

	/* Enable CPU caches, as ST's template Appli does first thing. */
	SCB_EnableICache();
	SCB_EnableDCache();
	DIAG_LedCode(3); /* checkpoint 3: reached after cache enable */

	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/
	HAL_Init();
	DIAG_LedCode(4); /* checkpoint 4: reached after HAL_Init() */

	/* USER CODE BEGIN Init */

	/* VDDA (analog supply) - HAL's own comment on this function says it's
	 * "mandatory to use the analog to digital converters", and UCPD1's CC-line
	 * Type-C voltage detection is fundamentally an analog comparator function
	 * (RM0486 75.4.7), very likely on the same VDDA rail. Never called
	 * anywhere in this project before. Confirmed via ST's own NUCLEO-N657X0-Q
	 * USBPD_SNK reference (same board, same UCPD1 peripheral), which enables
	 * this early in main() - ours never did. Root-caused via direct register
	 * read: UCPD1->CR showed ANAMODE=1 (Sink, correct) and CCENABLE=3 (both
	 * lines enabled, correct) but UCPD1->SR read 0x00000000 - both CC lines
	 * reporting vRa/nothing - with a real PD-capable source firmly plugged
	 * into CN8. Digitally configured correctly, but the analog front end
	 * reporting a "nothing attached" default suggests it was never powered. */
	HAL_PWREx_EnableVddA();

	/* VDD33USB domain unlock - THE root cause of USBPD CAD never detecting a
	 * plugged-in PD source (found 2026-07-22 via differential test against
	 * ST's unmodified USBPD_SNK reference on this same board/cable/source):
	 * UCPD1's CC-line analog front end lives in the VDD33USB power domain,
	 * which stays ISOLATED out of reset until USB33SV is set (RM0486: "Set
	 * USB33SV in PWR_SVMCR3 to remove the VDD33USB power isolation"). With it
	 * isolated, every UCPD digital register reads/configures fine (ANAMODE,
	 * CCENABLE, UCPDEN all looked correct) but TYPEC_VSTATE_CC1/CC2 read 0
	 * forever - the PHY is electrically disconnected from the pins. Confirmed
	 * by SVMCR3 register dump: ours read USB33SV=0/USB33VMEN=0 where the
	 * working reference sets both. Sequence per RM0486 + ST's reference
	 * main(): enable the 3.3V monitor, wait for USB33RDY, remove isolation.
	 * Guard-counter bounded (not tick-based - HAL tick isn't running yet at
	 * this point) so a broken rail degrades to "USBPD doesn't work" instead
	 * of "board hangs before any output". */
	HAL_PWREx_EnableVddUSBVMEN();
	for (uint32_t guard = 0; guard < 1000000U; guard++)
	{
		if (__HAL_PWR_GET_FLAG(PWR_FLAG_USB33RDY))
		{
			break;
		}
	}
	HAL_PWREx_EnableVddUSB();

	/* By default a Cortex-M55 lockup state (fault escalation failing, e.g. a fault
	 * while already in a same/higher-priority fault handler) is completely silent on
	 * this chip: no reset, no NMI, core just stops. Route it to NMI so it becomes
	 * observable instead of indistinguishable from any other silent hang.
	 * SystemInit() disables the SYSCFG clock again after using it, so re-enable it
	 * before touching any SYSCFG register - otherwise this write hangs the bus. */
	__HAL_RCC_SYSCFG_CLK_ENABLE();
	(void)RCC->APB4ENR2; /* delay: ensure the clock enable has taken effect */
	SYSCFG->CM55RSTCR |= SYSCFG_CM55RSTCR_LOCKUP_NMI_EN;
	DIAG_LedCode(5); /* checkpoint 5: reached after SYSCFG lockup-NMI setup */

	/* USER CODE END Init */

	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_CACHEAXI_Init();
	MX_LPUART1_UART_Init();
	MX_RAMCFG_Init();
	MX_UCPD1_Init();
	DIAG_LedCode(6); /* checkpoint 6: reached after all MX_*_Init() calls, before SystemIsolation_Config() */
	SystemIsolation_Config();
	DIAG_LedCode(7); /* checkpoint 7: reached after SystemIsolation_Config() (RIF/VENC/DCMIPP master config) */
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

	/* Force stdout fully UNBUFFERED. By default newlib(-nano) fully buffers stdout
	 * (~1KB), so a hang leaves up to ~1KB of already-"printed" trace stuck in the
	 * buffer, making the last visible line ~1KB BEHIND the real point of failure.
	 * Unbuffered => the last byte you see is the last byte the CPU actually ran. */
	setvbuf(stdout, NULL, _IONBF, 0);

	BSP_LED_Init(LED_GREEN);
	BSP_LED_Init(LED_RED);
	BSP_LED_Init(LED_BLUE);
	BSP_LED_On(LED_GREEN);

	printf("Appli: booting, SystemCoreClock=%lu\r\n", (unsigned long)SystemCoreClock);

	BSP_LED_On(LED_RED); /* DIAG checkpoint: reached here => printf call returned */

	if (CAMERA_init() != HAL_OK)
	{
		Error_Handler();
	}
	BSP_LED_On(LED_BLUE); /* camera initialized */

	/* VENC peripheral clock/VENCRAM bring-up (Phase 3). Self-contained, no
	 * separate clock-enable call needed. */
	LL_VENC_Init();

	/* USBPD HW global init + DPM core init - must run before the scheduler
	 * starts, same as ST's own USBPD_SNK reference. */
	printf("DIAG: about to call USBPD_PreInitOs()\r\n");
	if (USBPD_PreInitOs() != USBPD_OK)
	{
		printf("DIAG: USBPD_PreInitOs() FAILED\r\n");
		Error_Handler();
	}
	printf("DIAG: USBPD_PreInitOs() OK\r\n");

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
 * @brief  Configure the MPU non-cacheable region over the linker's
 *         .noncacheable section (Phase 3: DCMIPP capture buffers + VENC's
 *         encoder pool live there). Uses __snoncacheable/__enoncacheable,
 *         our own linker-defined symbols - not VENC_USB's reference
 *         __NON_CACHEABLE_SECTION_BEGIN/END, which don't resolve under our
 *         GNU toolchain (they appear to be an ARMCLANG scatter-file symbol).
 */
static void MPU_Config(void)
{
	extern uint32_t __snoncacheable;
	extern uint32_t __enoncacheable;

	MPU_Region_InitTypeDef	    region_config = {0};
	MPU_Attributes_InitTypeDef attr_config   = {0};
	uint32_t		    primask_bit	  = __get_PRIMASK();

	__disable_irq();

	HAL_MPU_Disable();

	attr_config.Attributes = INNER_OUTER(MPU_NOT_CACHEABLE);
	attr_config.Number	= MPU_ATTRIBUTES_NUMBER0;
	HAL_MPU_ConfigMemoryAttributes(&attr_config);

	region_config.Enable		= MPU_REGION_ENABLE;
	region_config.Number		= MPU_REGION_NUMBER0;
	region_config.BaseAddress	= (uint32_t)&__snoncacheable;
	region_config.LimitAddress	= (uint32_t)&__enoncacheable;
	region_config.DisableExec	= MPU_INSTRUCTION_ACCESS_ENABLE;
	region_config.AccessPermission = MPU_REGION_ALL_RW;
	region_config.IsShareable	= MPU_ACCESS_NOT_SHAREABLE;
	region_config.AttributesIndex	= MPU_ATTRIBUTES_NUMBER0;
	HAL_MPU_ConfigRegion(&region_config);

	HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

	__set_PRIMASK(primask_bit);
}

/**
 * @brief UCPD1 Initialization Function
 * @param None
 * @retval None
 * @note   No GPIO pin config here: UCPD1's CC1/CC2 lines are dedicated analog
 *         pins, not part of the normal GPIO AF mux. Just clock, the two
 *         GPDMA1 channels UCPD1 uses for byte-level PD message TX/RX, and
 *         the IRQ.
 */
static void MX_UCPD1_Init(void)
{
	LL_DMA_InitTypeDef DMA_InitStruct = {0};

	/* Peripheral clock enable */
	LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_UCPD1);

	/* GPDMA1_REQUEST_UCPD1_RX Init */
	DMA_InitStruct.SrcAddress	    = 0x00000000U;
	DMA_InitStruct.DestAddress	    = 0x00000000U;
	DMA_InitStruct.Direction	    = LL_DMA_DIRECTION_PERIPH_TO_MEMORY;
	DMA_InitStruct.BlkHWRequest	    = LL_DMA_HWREQUEST_SINGLEBURST;
	DMA_InitStruct.DataAlignment	    = LL_DMA_DATA_ALIGN_ZEROPADD;
	DMA_InitStruct.SrcBurstLength	    = 1;
	DMA_InitStruct.DestBurstLength	    = 1;
	DMA_InitStruct.SrcDataWidth	    = LL_DMA_SRC_DATAWIDTH_BYTE;
	DMA_InitStruct.DestDataWidth	    = LL_DMA_DEST_DATAWIDTH_BYTE;
	DMA_InitStruct.SrcIncMode	    = LL_DMA_SRC_FIXED;
	DMA_InitStruct.DestIncMode	    = LL_DMA_DEST_INCREMENT;
	DMA_InitStruct.Priority	    = LL_DMA_LOW_PRIORITY_LOW_WEIGHT;
	DMA_InitStruct.BlkDataLength	    = 0x00000000U;
	DMA_InitStruct.TriggerMode	    = LL_DMA_TRIGM_BLK_TRANSFER;
	DMA_InitStruct.TriggerPolarity	    = LL_DMA_TRIG_POLARITY_MASKED;
	DMA_InitStruct.TriggerSelection    = 0x00000000U;
	DMA_InitStruct.Request		    = LL_GPDMA1_REQUEST_UCPD1_RX;
	DMA_InitStruct.TransferEventMode   = LL_DMA_TCEM_BLK_TRANSFER;
	DMA_InitStruct.SrcAllocatedPort    = LL_DMA_SRC_ALLOCATED_PORT0;
	DMA_InitStruct.DestAllocatedPort   = LL_DMA_DEST_ALLOCATED_PORT0;
	DMA_InitStruct.LinkAllocatedPort   = LL_DMA_LINK_ALLOCATED_PORT1;
	DMA_InitStruct.LinkStepMode	    = LL_DMA_LSM_FULL_EXECUTION;
	DMA_InitStruct.LinkedListBaseAddr  = 0x00000000U;
	DMA_InitStruct.LinkedListAddrOffset = 0x00000000U;
	LL_DMA_Init(GPDMA1, LL_DMA_CHANNEL_3, &DMA_InitStruct);

	/* GPDMA1_REQUEST_UCPD1_TX Init */
	DMA_InitStruct.SrcAddress	  = 0x00000000U;
	DMA_InitStruct.DestAddress	  = 0x00000000U;
	DMA_InitStruct.Direction	  = LL_DMA_DIRECTION_MEMORY_TO_PERIPH;
	DMA_InitStruct.BlkHWRequest	  = LL_DMA_HWREQUEST_SINGLEBURST;
	DMA_InitStruct.DataAlignment	  = LL_DMA_DATA_ALIGN_ZEROPADD;
	DMA_InitStruct.SrcBurstLength	  = 1;
	DMA_InitStruct.DestBurstLength	  = 1;
	DMA_InitStruct.SrcDataWidth	  = LL_DMA_SRC_DATAWIDTH_BYTE;
	DMA_InitStruct.DestDataWidth	  = LL_DMA_DEST_DATAWIDTH_BYTE;
	DMA_InitStruct.SrcIncMode	  = LL_DMA_SRC_INCREMENT;
	DMA_InitStruct.DestIncMode	  = LL_DMA_DEST_FIXED;
	DMA_InitStruct.Priority	  = LL_DMA_LOW_PRIORITY_LOW_WEIGHT;
	DMA_InitStruct.BlkDataLength	  = 0x00000000U;
	DMA_InitStruct.TriggerMode	  = LL_DMA_TRIGM_BLK_TRANSFER;
	DMA_InitStruct.TriggerPolarity	  = LL_DMA_TRIG_POLARITY_MASKED;
	DMA_InitStruct.TriggerSelection  = 0x00000000U;
	DMA_InitStruct.Request		  = LL_GPDMA1_REQUEST_UCPD1_TX;
	DMA_InitStruct.TransferEventMode = LL_DMA_TCEM_BLK_TRANSFER;
	DMA_InitStruct.SrcAllocatedPort  = LL_DMA_SRC_ALLOCATED_PORT0;
	DMA_InitStruct.DestAllocatedPort = LL_DMA_DEST_ALLOCATED_PORT0;
	DMA_InitStruct.LinkAllocatedPort = LL_DMA_LINK_ALLOCATED_PORT1;
	DMA_InitStruct.LinkStepMode	  = LL_DMA_LSM_FULL_EXECUTION;
	DMA_InitStruct.LinkedListBaseAddr   = 0x00000000U;
	DMA_InitStruct.LinkedListAddrOffset = 0x00000000U;
	LL_DMA_Init(GPDMA1, LL_DMA_CHANNEL_2, &DMA_InitStruct);

	/* UCPD1 interrupt Init */
	NVIC_SetPriority(UCPD1_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));
	NVIC_EnableIRQ(UCPD1_IRQn);
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

	/* Phase 3: VENC master, same CID as DCMIPP since they're both part of the
	 * camera->encode pipeline. */
	RIMC_master.MasterCID = RIF_CID_1;
	HAL_RIF_RIMC_ConfigMasterAttributes(RIF_MASTER_INDEX_VENC, &RIMC_master);

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

/**
 * @brief  Period elapsed callback in non blocking mode
 * @note   This function is called when TIM6 interrupt takes place, inside
 *         HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to
 *         increment a global variable "uwTick" used as application time base.
 * @param  htim : TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM6)
	{
		HAL_IncTick();
	}
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
