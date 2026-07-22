/**
 ******************************************************************************
 * @file    app_usbx_device.c
 * @brief   USBX Device applicative file - Phase 4 (UVC port plan).
 ******************************************************************************
 * Adapted from ST's VENC_USB reference (Appli/USBX/App/app_usbx_device.c).
 * Board-agnostic apart from referencing our own hpcd_USB1_OTG_HS/
 * MX_USB1_OTG_HS_PCD_Init (defined in main.c, matching the project's existing
 * MX_*_Init() convention for other peripherals).
 ******************************************************************************
 */
#include "app_usbx_device.h"
#include "main.h"

static ULONG				  video_interface_number;
static ULONG				  video_configuration_number;
static UX_DEVICE_CLASS_VIDEO_PARAMETER	  video_parameter;
static UX_DEVICE_CLASS_VIDEO_STREAM_PARAMETER video_stream_parameter[USBD_VIDEO_STREAM_NMNBER];
static TX_THREAD			  ux_device_app_thread;

extern PCD_HandleTypeDef hpcd_USB1_OTG_HS;

TX_QUEUE ux_app_MsgQueue_UCPD;
__ALIGN_BEGIN USB_MODE_STATE USB_Device_State_Msg __ALIGN_END;

static VOID app_ux_device_thread_entry(ULONG thread_input);

/**
 * @brief  Application USBX Device Initialization.
 * @param  memory_ptr: memory pointer
 * @retval status
 */
UINT MX_USBX_Device_Init(VOID *memory_ptr)
{
	UINT	      ret = UX_SUCCESS;
	UCHAR	     *device_framework_high_speed;
	UCHAR	     *device_framework_full_speed;
	ULONG	      device_framework_hs_length;
	ULONG	      device_framework_fs_length;
	ULONG	      string_framework_length;
	ULONG	      language_id_framework_length;
	UCHAR	     *string_framework;
	UCHAR	     *language_id_framework;
	UCHAR	     *pointer;
	TX_BYTE_POOL *byte_pool = (TX_BYTE_POOL *)memory_ptr;

	if (tx_byte_allocate(byte_pool, (VOID **)&pointer, USBX_DEVICE_MEMORY_STACK_SIZE, TX_NO_WAIT) !=
	    TX_SUCCESS)
	{
		return TX_POOL_ERROR;
	}

	if (ux_system_initialize(pointer, USBX_DEVICE_MEMORY_STACK_SIZE, UX_NULL, 0) != UX_SUCCESS)
	{
		return UX_ERROR;
	}

	device_framework_high_speed = USBD_Get_Device_Framework_Speed(USBD_HIGH_SPEED, &device_framework_hs_length);
	device_framework_full_speed = USBD_Get_Device_Framework_Speed(USBD_FULL_SPEED, &device_framework_fs_length);
	string_framework	     = USBD_Get_String_Framework(&string_framework_length);
	language_id_framework	     = USBD_Get_Language_Id_Framework(&language_id_framework_length);

	if (ux_device_stack_initialize(device_framework_high_speed, device_framework_hs_length,
					device_framework_full_speed, device_framework_fs_length, string_framework,
					string_framework_length, language_id_framework,
					language_id_framework_length, UX_NULL) != UX_SUCCESS)
	{
		return UX_ERROR;
	}

	video_stream_parameter[0].ux_device_class_video_stream_parameter_callbacks
		.ux_device_class_video_stream_change = USBD_VIDEO_StreamChange;

	video_stream_parameter[0].ux_device_class_video_stream_parameter_callbacks
		.ux_device_class_video_stream_payload_done = USBD_VIDEO_StreamPayloadDone;

	video_stream_parameter[0].ux_device_class_video_stream_parameter_callbacks
		.ux_device_class_video_stream_request = USBD_VIDEO_StreamRequest;

	video_stream_parameter[0].ux_device_class_video_stream_parameter_max_payload_buffer_nb =
		USBD_VIDEO_PAYLOAD_BUFFER_NUMBER;

	video_stream_parameter[0].ux_device_class_video_stream_parameter_max_payload_buffer_size =
		USBD_VIDEO_StreamGetMaxPayloadBufferSize();

	video_stream_parameter[0].ux_device_class_video_stream_parameter_thread_entry =
		ux_device_class_video_write_thread_entry;

	video_parameter.ux_device_class_video_parameter_streams_nb = USBD_VIDEO_STREAM_NMNBER;
	video_parameter.ux_device_class_video_parameter_streams    = video_stream_parameter;

	video_parameter.ux_device_class_video_parameter_callbacks.ux_slave_class_video_instance_activate =
		USBD_VIDEO_Activate;

	video_parameter.ux_device_class_video_parameter_callbacks.ux_slave_class_video_instance_deactivate =
		USBD_VIDEO_Deactivate;

	video_configuration_number = USBD_Get_Configuration_Number(CLASS_TYPE_VIDEO, 0);
	video_interface_number	    = USBD_Get_Interface_Number(CLASS_TYPE_VIDEO, 0);

	if (ux_device_stack_class_register(_ux_system_device_class_video_name, ux_device_class_video_entry,
					    video_configuration_number, video_interface_number,
					    (VOID *)&video_parameter) != UX_SUCCESS)
	{
		return UX_ERROR;
	}

	if (tx_byte_allocate(byte_pool, (VOID **)&pointer, UX_DEVICE_APP_THREAD_STACK_SIZE, TX_NO_WAIT) !=
	    TX_SUCCESS)
	{
		return TX_POOL_ERROR;
	}

	if (tx_thread_create(&ux_device_app_thread, UX_DEVICE_APP_THREAD_NAME, app_ux_device_thread_entry, 0,
			      pointer, UX_DEVICE_APP_THREAD_STACK_SIZE, UX_DEVICE_APP_THREAD_PRIO,
			      UX_DEVICE_APP_THREAD_PREEMPTION_THRESHOLD, UX_DEVICE_APP_THREAD_TIME_SLICE,
			      UX_DEVICE_APP_THREAD_START_OPTION) != TX_SUCCESS)
	{
		return TX_THREAD_ERROR;
	}

	if (tx_byte_allocate(byte_pool, (VOID **)&pointer, APP_QUEUE_SIZE * sizeof(ULONG), TX_NO_WAIT) !=
	    TX_SUCCESS)
	{
		return TX_POOL_ERROR;
	}

	if (tx_queue_create(&ux_app_MsgQueue_UCPD, "Message Queue app", TX_1_ULONG, pointer,
			     APP_QUEUE_SIZE * sizeof(ULONG)) != TX_SUCCESS)
	{
		return TX_QUEUE_ERROR;
	}

	return ret;
}

/**
 * @brief  Function implementing app_ux_device_thread_entry.
 * @param  thread_input: User thread input parameter.
 * @retval none
 */
static VOID app_ux_device_thread_entry(ULONG thread_input)
{
	(void)thread_input;

	USBX_APP_Device_Init();

	while (1)
	{
		if (tx_queue_receive(&ux_app_MsgQueue_UCPD, &USB_Device_State_Msg, TX_WAIT_FOREVER) != TX_SUCCESS)
		{
			Error_Handler();
		}
		if (USB_Device_State_Msg == START_USB_DEVICE)
		{
			HAL_PCD_Start(&hpcd_USB1_OTG_HS);
		}
		else if (USB_Device_State_Msg == STOP_USB_DEVICE)
		{
			HAL_PCD_Stop(&hpcd_USB1_OTG_HS);
		}
		else
		{
			Error_Handler();
		}
	}
}

/**
 * @brief USBX_APP_Device_Init
 *        Initialization of USB device.
 * @retval None
 */
VOID USBX_APP_Device_Init(VOID)
{
	MX_USB1_OTG_HS_PCD_Init();

	HAL_PCDEx_SetRxFiFo(&hpcd_USB1_OTG_HS, 0x100);
	HAL_PCDEx_SetTxFiFo(&hpcd_USB1_OTG_HS, 0, 0x10);
	HAL_PCDEx_SetTxFiFo(&hpcd_USB1_OTG_HS, 1, 0x200);

	ux_dcd_stm32_initialize((ULONG)USB1_OTG_HS, (ULONG)&hpcd_USB1_OTG_HS);
}
