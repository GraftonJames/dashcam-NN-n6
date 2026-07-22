/**
 ******************************************************************************
 * @file    ux_device_video.c
 * @brief   USBX Device Video applicative file - Phase 4 (UVC port plan).
 ******************************************************************************
 * Adapted from ST's VENC_USB reference (Appli/USBX/App/ux_device_video.c) -
 * board/resolution-agnostic (driven by the macros in ux_device_video.h /
 * ux_device_descriptors.h), so copied with only the venc_app.h include and
 * VENC_APP_EncodingStart/Stop calls removed (this port's venc_thread starts
 * capture/encoding unconditionally in Phase 3, not gated on USB activation).
 ******************************************************************************
 */
#include "ux_device_video.h"
#include "venc_app.h"

#define UVC_PLAY_STATUS_STOP	   0x00U
#define UVC_PLAY_STATUS_READY	   0x01U
#define UVC_PLAY_STATUS_STREAMING 0x02U

uint32_t		       payload_count;
ULONG			       uvc_state;
UX_DEVICE_CLASS_VIDEO	      *video;
UX_DEVICE_CLASS_VIDEO_STREAM *stream_write;
UCHAR			       video_frame_buffer[1024];
uint8_t			       new_frame_requested = 1;

static USBD_VideoControlTypeDef video_Probe_Control = {
	.bmHint			   = 0x0000U,
	.bFormatIndex		   = 0x01U,
	.bFrameIndex		   = 0x01U,
	.dwFrameInterval	   = UVC_INTERVAL(UVC_CAM_FPS_FS),
	.wKeyFrameRate		   = 0x0000U,
	.wPFrameRate		   = 0x0000U,
	.wCompQuality		   = 0x0000U,
	.wCompWindowSize	   = 0x0000U,
	.wDelay			   = 0x0000U,
	.dwMaxVideoFrameSize	   = 0x0000U,
	.dwMaxPayloadTransferSize  = 0x00000000U,
	.dwClockFrequency	   = 0x00000000U,
	.bmFramingInfo		   = 0x00U,
	.bPreferedVersion	   = 0x00U,
	.bMinVersion		   = 0x00U,
	.bMaxVersion		   = 0x00U,
	.bUsage			   = 0x1U,
	.bBitDepthLuma		   = 0x0U,
	.bmSettings		   = 0x0U,
	.bMaxNumberOfRefFramesPlus1 = 0x1U,
	.bmRateControlModes	   = 0x0U,
	.bmLayoutPerStream	   = 0x0U,
};

static USBD_VideoControlTypeDef video_Commit_Control = {
	.bmHint			  = 0x0000U,
	.bFormatIndex		  = 0x01U,
	.bFrameIndex		  = 0x01U,
	.dwFrameInterval	  = UVC_INTERVAL(UVC_CAM_FPS_FS),
	.wKeyFrameRate		  = 0x0000U,
	.wPFrameRate		  = 0x0000U,
	.wCompQuality		  = 0x0000U,
	.wCompWindowSize	  = 0x0000U,
	.wDelay			  = 0x0000U,
	.dwMaxVideoFrameSize	  = 0x0000U,
	.dwMaxPayloadTransferSize = 0x00000000U,
	.dwClockFrequency	  = 0x00000000U,
	.bmFramingInfo		  = 0x00U,
	.bPreferedVersion	  = 0x00U,
	.bMinVersion		  = 0x00U,
	.bMaxVersion		  = 0x00U,
};

static VOID video_write_payload(UX_DEVICE_CLASS_VIDEO_STREAM *stream);
UINT	    usb_sender_session_h264_send(UX_DEVICE_CLASS_VIDEO_STREAM *stream, UCHAR *frame_data,
					  ULONG frame_data_size, ULONG timestamp, UINT marker);

/**
 * @brief  usb_video_data_read
 * @param  data: Pointer to the video Data.
 * @param  size: Pointer to the video Data length.
 * @retval status
 */
UINT usb_video_data_read(UCHAR **data, ULONG *size)
{
	return VENC_APP_GetData(data, size);
}

/**
 * @brief  USBD_VIDEO_Activate
 *         This function is called when insertion of a Video device.
 * @param  video_instance: Pointer to the video class instance.
 * @retval none
 */
VOID USBD_VIDEO_Activate(VOID *video_instance)
{
	video = (UX_DEVICE_CLASS_VIDEO *)video_instance;

	ux_device_class_video_stream_get(video, 0, &stream_write);

	return;
}

/**
 * @brief  USBD_VIDEO_Deactivate
 *         This function is called when extraction of a Video device.
 * @param  video_instance: Pointer to the video class instance.
 * @retval none
 */
VOID USBD_VIDEO_Deactivate(VOID *video_instance)
{
	UX_PARAMETER_NOT_USED(video_instance);

	video	     = UX_NULL;
	stream_write = UX_NULL;

	return;
}

/**
 * @brief  USBD_VIDEO_StreamChange
 *         This function is invoked to inform application that the
 *         alternate setting are changed.
 * @param  video_stream: Pointer to video class stream instance.
 * @param  alternate_setting: interface alternate setting.
 * @retval none
 */
VOID USBD_VIDEO_StreamChange(UX_DEVICE_CLASS_VIDEO_STREAM *video_stream, ULONG alternate_setting)
{
	if (alternate_setting == 0U)
	{
		uvc_state = UVC_PLAY_STATUS_STOP;

		return;
	}

	uvc_state = UVC_PLAY_STATUS_STREAMING;

	video_write_payload(video_stream);

	ux_device_class_video_transmission_start(video_stream);

	return;
}

/**
 * @brief  USBD_VIDEO_StreamPayloadDone
 *         This function is invoked when stream data payload received.
 * @param  video_stream: Pointer to video class stream instance.
 * @param  length: transfer length.
 * @retval none
 */
VOID USBD_VIDEO_StreamPayloadDone(UX_DEVICE_CLASS_VIDEO_STREAM *video_stream, ULONG length)
{
	if (length != 0U)
	{
		video_write_payload(video_stream);
	}

	return;
}

/**
 * @brief  USBD_VIDEO_StreamRequest
 *         This function is invoked to manage the UVC class requests.
 * @param  video_stream: Pointer to video class stream instance.
 * @param  transfer: Pointer to the transfer request.
 * @retval status
 */
UINT USBD_VIDEO_StreamRequest(UX_DEVICE_CLASS_VIDEO_STREAM *video_stream, UX_SLAVE_TRANSFER *transfer)
{
	UINT status = UX_SUCCESS;

	UCHAR  *data, bRequest;
	USHORT	wValue_CS, wLength;

	(void)video_stream;

	bRequest  = transfer->ux_slave_transfer_request_setup[UX_SETUP_REQUEST];
	wValue_CS = transfer->ux_slave_transfer_request_setup[UX_SETUP_VALUE + 1];
	wLength	  = ux_utility_short_get(transfer->ux_slave_transfer_request_setup + UX_SETUP_LENGTH);
	data	  = transfer->ux_slave_transfer_request_data_pointer;

	switch (wValue_CS)
	{
	case UX_DEVICE_CLASS_VIDEO_VS_PROBE_CONTROL:

		switch (bRequest)
		{
		case UX_DEVICE_CLASS_VIDEO_SET_CUR:

			status = UX_SUCCESS;

			break;

		case UX_DEVICE_CLASS_VIDEO_GET_DEF:
		case UX_DEVICE_CLASS_VIDEO_GET_CUR:
		case UX_DEVICE_CLASS_VIDEO_GET_MIN:
		case UX_DEVICE_CLASS_VIDEO_GET_MAX:

			video_Probe_Control.bPreferedVersion	 = 0x00U;
			video_Probe_Control.bMinVersion	 = 0x00U;
			video_Probe_Control.bMaxVersion	 = 0x00U;
			video_Probe_Control.dwMaxVideoFrameSize = UVC_MAX_FRAME_SIZE;
			video_Probe_Control.dwClockFrequency	 = 0x02DC6C00U;

			if (_ux_system_slave->ux_system_slave_speed == UX_FULL_SPEED_DEVICE)
			{
				video_Probe_Control.dwFrameInterval	     = (UVC_INTERVAL(UVC_CAM_FPS_FS));
				video_Probe_Control.dwMaxPayloadTransferSize = USBD_VIDEO_EPIN_FS_MPS;
			}
			else
			{
				video_Probe_Control.dwFrameInterval	     = (UVC_INTERVAL(UVC_CAM_FPS_HS));
				video_Probe_Control.dwMaxPayloadTransferSize = USBD_VIDEO_EPIN_HS_MPS;
			}

			ux_utility_memory_copy(data, (VOID *)&video_Probe_Control,
						sizeof(USBD_VideoControlTypeDef));

			status = ux_device_stack_transfer_request(
				transfer, UX_MIN(wLength, sizeof(USBD_VideoControlTypeDef)),
				UX_MIN(wLength, sizeof(USBD_VideoControlTypeDef)));

			break;

		default:
			break;
		}
		break;

	case UX_DEVICE_CLASS_VIDEO_VS_COMMIT_CONTROL:

		if (wLength < 26)
		{
			break;
		}

		switch (bRequest)
		{
		case UX_DEVICE_CLASS_VIDEO_SET_CUR:

			status = UX_SUCCESS;

			break;

		case UX_DEVICE_CLASS_VIDEO_GET_CUR:

			if (_ux_system_slave->ux_system_slave_speed == UX_FULL_SPEED_DEVICE)
			{
				video_Commit_Control.dwFrameInterval	      = (UVC_INTERVAL(UVC_CAM_FPS_FS));
				video_Commit_Control.dwMaxPayloadTransferSize = USBD_VIDEO_EPIN_FS_MPS;
			}
			else
			{
				video_Commit_Control.dwFrameInterval = (UVC_INTERVAL(UVC_CAM_FPS_HS));
				video_Commit_Control.dwMaxPayloadTransferSize = USBD_VIDEO_EPIN_HS_BINTERVAL;
			}

			ux_utility_memory_copy(data, (VOID *)&video_Commit_Control, 26);

			status = ux_device_stack_transfer_request(
				transfer, UX_MIN(wLength, sizeof(USBD_VideoControlTypeDef)),
				UX_MIN(wLength, sizeof(USBD_VideoControlTypeDef)));

			break;

		default:
			break;
		}

		break;

	default:
		status = UX_ERROR;
		break;
	}

	return status;
}

/**
 * @brief  USBD_VIDEO_StreamGetMaxPayloadBufferSize
 *         Get video stream max payload buffer size.
 * @param  none
 * @retval max payload
 */
ULONG USBD_VIDEO_StreamGetMaxPayloadBufferSize(VOID)
{
	return USBD_VIDEO_EPIN_HS_MPS;
}

/**
 * @brief  video_write_payload
           Manage the UVC data packets.
 * @param  stream : Video class stream instance.
 * @retval none
 */
VOID video_write_payload(UX_DEVICE_CLASS_VIDEO_STREAM *stream)
{
	static ULONG	length;
	ULONG		timestamp = 100000;
	static uint8_t *Pcktdata  = NULL;

	switch (uvc_state)
	{
	case UVC_PLAY_STATUS_STREAMING:
		if ((new_frame_requested == 1) || (Pcktdata == NULL))
		{
			new_frame_requested = 0;
			do
			{
				usb_video_data_read(&Pcktdata, &length);
			} while (Pcktdata == NULL);
		}
		usb_sender_session_h264_send(stream, Pcktdata, length, timestamp, UX_TRUE);
		break;

	case UVC_PLAY_STATUS_STOP:
		break;
	default:
		return;
	}
}

UINT usb_sender_session_h264_send(UX_DEVICE_CLASS_VIDEO_STREAM *stream, UCHAR *frame_data, ULONG frame_data_size,
				   ULONG timestamp, UINT marker)
{
	(void)marker;

	static ULONG  nal_unit_size = 0;
	ULONG	      nal_video_xfer;
	static UCHAR *nal_unit_start;
	ULONG	      usbd_video_ep_mps = stream->ux_device_class_video_stream_endpoint->ux_slave_endpoint_descriptor
					.wMaxPacketSize;
	UCHAR  *buffer;
	ULONG	buffer_length;

	static UCHAR FID_Bit = FID_NEF_FALSE;

	if (nal_unit_start == NULL)
	{
		nal_unit_size  = frame_data_size;
		nal_unit_start = frame_data;
	}
	ux_device_class_video_write_payload_get(stream, &buffer, &buffer_length);

	if (nal_unit_size > (usbd_video_ep_mps - PAYLOAD_HEADER_SIZE))
	{
		nal_video_xfer = usbd_video_ep_mps - PAYLOAD_HEADER_SIZE;
		nal_unit_size -= nal_video_xfer;
		video_frame_buffer[0]			  = PAYLOAD_HEADER_SIZE;
		video_frame_buffer[1]			  = FID_Bit;
		*(uint32_t *)((video_frame_buffer) + 2)  = timestamp;
		*(uint32_t *)((video_frame_buffer) + 6)  = SOURCE_CLOCK_REF;

		ux_utility_memory_copy((video_frame_buffer + PAYLOAD_HEADER_SIZE), nal_unit_start, nal_video_xfer);
		nal_unit_start += nal_video_xfer;

		ux_utility_memory_copy(buffer, video_frame_buffer, nal_video_xfer + PAYLOAD_HEADER_SIZE);
		SCB_CleanDCache_by_Addr((uint32_t *)buffer, nal_video_xfer + PAYLOAD_HEADER_SIZE);
		ux_device_class_video_write_payload_commit(stream, nal_video_xfer + PAYLOAD_HEADER_SIZE);
	}
	else
	{
		nal_video_xfer	       = nal_unit_size;
		video_frame_buffer[0] = PAYLOAD_HEADER_SIZE;
		video_frame_buffer[1] = FID_Bit;
		if (FID_Bit == FID_EF_FALSE)
		{
			FID_Bit = FID_EF_TRUE;
		}
		else
		{
			FID_Bit = FID_EF_FALSE;
		}

		*(uint32_t *)((video_frame_buffer) + 2) = timestamp;
		*(uint32_t *)((video_frame_buffer) + 6) = SOURCE_CLOCK_REF;

		ux_utility_memory_copy((video_frame_buffer + PAYLOAD_HEADER_SIZE), nal_unit_start, nal_video_xfer);
		ux_utility_memory_copy(buffer, video_frame_buffer, nal_video_xfer + PAYLOAD_HEADER_SIZE);
		SCB_CleanDCache_by_Addr((uint32_t *)buffer, nal_video_xfer + PAYLOAD_HEADER_SIZE);
		ux_device_class_video_write_payload_commit(stream, nal_video_xfer + PAYLOAD_HEADER_SIZE);
		new_frame_requested = 1;
		nal_unit_start	     = NULL;
	}

	return (UX_SUCCESS);
}
