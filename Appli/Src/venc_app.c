/**
 ******************************************************************************
 * @file    venc_app.c
 * @brief   VENC H264 encoder driving loop - Phase 3/4 (UVC port plan).
 ******************************************************************************
 * Phase 3 proved the encoder itself works (H264EncInit succeeds, steady frame
 * counter, valid-looking NAL output) using one reused output buffer. Phase 4
 * replaces that with a small block pool + ThreadX queue (modeled on VENC_USB
 * reference's venc_app.c), so a frame can sit queued for USB transmission
 * (ux_device_video.c's VENC_APP_GetData()) while the next one is being
 * encoded, instead of the two racing over one shared buffer.
 *
 * Block pool sizing: 4 blocks x 32KB = 128KB, the same total budget as
 * Phase 3's single 128KB buffer (see venc_h264_config.c for the full
 * .noncacheable budget accounting) - not a new allocation, just restructured.
 * At 128kbps/10fps the average frame is ~1.6KB; 32KB gives generous headroom
 * for I-frames. Same "safe to underestimate" caveat as the rest of Phase 3:
 * H264EncStrmEncode() reports a clean error if a block is too small.
 ******************************************************************************
 */
#include "main.h"
#include "camera.h"
#include "venc_h264_config.h"
#include "h264encapi.h"
#include "tx_api.h"
#include "venc_app.h"
#include <string.h>

#define VENC_OUTPUT_BLOCK_SIZE (32UL * 1024UL)
#define VENC_OUTPUT_BLOCK_NBR	4

#define ALIGNED(ptr, bytes) ((((ptr) + (bytes)-1) / (bytes)) * (bytes))

typedef struct
{
	uint32_t  size;
	uint32_t *block_addr;
	uint32_t *aligned_block_addr;
} venc_output_frame_t;

static uint8_t output_block_buffer[VENC_OUTPUT_BLOCK_NBR * VENC_OUTPUT_BLOCK_SIZE] __attribute__((aligned(32)))
__attribute__((section(".noncacheable")));

static TX_THREAD   venc_thread;
static uint8_t	    venc_thread_stack[2048];
static H264EncInst venc_encoder;
static H264EncIn   venc_enc_in;
static H264EncOut  venc_enc_out;

static TX_QUEUE	     enc_frame_queue;
static TX_BLOCK_POOL venc_block_pool;
static venc_output_frame_t queue_buf[VENC_OUTPUT_BLOCK_NBR];

static int venc_encoder_prepare(void)
{
	H264EncRet ret;

	ret = H264EncInit(&hVencH264Instance.cfgH264Main, &venc_encoder);
	printf("VENC: H264EncInit returned %d\r\n", (int)ret);
	if (ret != H264ENC_OK)
	{
		return -1;
	}

	ret = H264EncSetPreProcessing(venc_encoder, &hVencH264Instance.cfgH264Preproc);
	if (ret != H264ENC_OK)
	{
		printf("VENC: H264EncSetPreProcessing FAILED, ret=%d\r\n", (int)ret);
		return -1;
	}

	ret = H264EncSetCodingCtrl(venc_encoder, &hVencH264Instance.cfgH264Coding);
	if (ret != H264ENC_OK)
	{
		printf("VENC: H264EncSetCodingCtrl FAILED, ret=%d\r\n", (int)ret);
		return -1;
	}

	ret = H264EncSetRateCtrl(venc_encoder, &hVencH264Instance.cfgH264Rate);
	if (ret != H264ENC_OK)
	{
		printf("VENC: H264EncSetRateCtrl FAILED, ret=%d\r\n", (int)ret);
		return -1;
	}

	return 0;
}

static void venc_thread_entry(ULONG initial_input)
{
	H264EncRet	     ret;
	uint32_t	     frame_nb = 0;
	venc_output_frame_t frame_buffer;

	(void)initial_input;

	if (tx_queue_create(&enc_frame_queue, "ENC frame queue", sizeof(venc_output_frame_t) / 4, &queue_buf,
			     sizeof(queue_buf)) != TX_SUCCESS)
	{
		printf("VENC: tx_queue_create FAILED, venc_thread exiting\r\n");
		return;
	}

	if (tx_block_pool_create(&venc_block_pool, "venc output block pool", VENC_OUTPUT_BLOCK_SIZE,
				  output_block_buffer, sizeof(output_block_buffer)) != TX_SUCCESS)
	{
		printf("VENC: tx_block_pool_create FAILED, venc_thread exiting\r\n");
		return;
	}

	if (venc_encoder_prepare() != 0)
	{
		printf("VENC: encoder_prepare FAILED, venc_thread exiting\r\n");
		return;
	}

	/* Frame mode: capture straight into input_frame, continuously. No HW
	 * handshake with the encoder - DCMIPP just keeps input_frame current.
	 * PIPE1 (Main), matching DCMIPP_initVenc() in camera.c - the only pipe
	 * that supports YUV422 conversion in hardware (PIPE0 is additionally an
	 * incomplete vendor stub in cmw_camera.c) - hardware-verified 2026-07-22. */
	if (CMW_CAMERA_Start(DCMIPP_PIPE1, GetInputFrame(NULL), CMW_MODE_CONTINUOUS) != CMW_ERROR_NONE)
	{
		printf("VENC: CMW_CAMERA_Start FAILED, venc_thread exiting\r\n");
		return;
	}

	memset(&frame_buffer, 0, sizeof(frame_buffer));
	if (tx_block_allocate(&venc_block_pool, (void **)&frame_buffer.block_addr, TX_NO_WAIT) != TX_SUCCESS)
	{
		printf("VENC: initial tx_block_allocate FAILED, venc_thread exiting\r\n");
		return;
	}
	frame_buffer.aligned_block_addr = (uint32_t *)ALIGNED((uint32_t)frame_buffer.block_addr, 8);

	venc_enc_in.busLuma	= (ptr_t)GetInputFrame(NULL);
	venc_enc_in.pOutBuf	= frame_buffer.aligned_block_addr;
	venc_enc_in.busOutBuf	= (size_t)frame_buffer.aligned_block_addr;
	venc_enc_in.outBufSize = VENC_OUTPUT_BLOCK_SIZE - 8;
	venc_enc_in.codingType	= H264ENC_INTRA_FRAME;
	venc_enc_in.ipf	= H264ENC_REFERENCE_AND_REFRESH;
	venc_enc_in.ltrf	= H264ENC_REFERENCE;

	ret = H264EncStrmStart(venc_encoder, &venc_enc_in, &venc_enc_out);
	printf("VENC: H264EncStrmStart returned %d\r\n", (int)ret);
	if (ret != H264ENC_OK)
	{
		tx_block_release(frame_buffer.block_addr);
		return;
	}
	frame_buffer.size = venc_enc_out.streamSize;
	if (tx_queue_send(&enc_frame_queue, (void *)&frame_buffer, TX_NO_WAIT) != TX_SUCCESS)
	{
		tx_block_release(frame_buffer.block_addr);
	}

	while (1)
	{
		if (!(frame_nb % 30))
		{
			venc_enc_in.codingType = H264ENC_INTRA_FRAME;
		}
		else
		{
			venc_enc_in.timeIncrement = 1;
			venc_enc_in.codingType	  = H264ENC_PREDICTED_FRAME;
		}

		if (tx_block_allocate(&venc_block_pool, (void **)&frame_buffer.block_addr, TX_WAIT_FOREVER) !=
		    TX_SUCCESS)
		{
			printf("VENC: failed to allocate output block\r\n");
			continue;
		}
		frame_buffer.aligned_block_addr = (uint32_t *)ALIGNED((uint32_t)frame_buffer.block_addr, 8);
		venc_enc_in.pOutBuf		 = frame_buffer.aligned_block_addr;
		venc_enc_in.busOutBuf		 = (size_t)frame_buffer.aligned_block_addr;
		venc_enc_in.outBufSize		 = VENC_OUTPUT_BLOCK_SIZE - 8;

		ret = H264EncStrmEncode(venc_encoder, &venc_enc_in, &venc_enc_out, NULL, NULL, NULL);
		if (ret == H264ENC_FRAME_READY && venc_enc_out.streamSize != 0)
		{
			frame_buffer.size = venc_enc_out.streamSize;
			if (tx_queue_send(&enc_frame_queue, (void *)&frame_buffer, TX_NO_WAIT) != TX_SUCCESS)
			{
				tx_block_release(frame_buffer.block_addr);
			}
			if (!(frame_nb % 30))
			{
				printf("VENC: frame=%lu size=%lu bytes\r\n", (unsigned long)frame_nb,
				       (unsigned long)venc_enc_out.streamSize);
			}
			frame_nb++;
		}
		else
		{
			tx_block_release(frame_buffer.block_addr);
			printf("VENC: H264EncStrmEncode returned %d (frame=%lu)\r\n", (int)ret,
			       (unsigned long)frame_nb);
		}

		/* Pace to VENC_FRAMERATE (app_conf.h): TX_TIMER_TICKS_PER_SECOND is
		 * 100 (10ms/tick, ThreadX default - tx_user.h leaves it unset). */
		tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND / VENC_FRAMERATE);
	}
}

void VENC_APP_ThreadCreate(void)
{
	tx_thread_create(&venc_thread, "venc_thread", venc_thread_entry, 0, venc_thread_stack,
			  sizeof(venc_thread_stack), 20, 20, TX_NO_TIME_SLICE, TX_AUTO_START);
}

/**
 * @brief  VENC_APP_GetData - called from ux_device_video.c's
 *         usb_video_data_read() to pull the next encoded frame for USB
 *         transmission. Non-blocking: returns immediately if nothing is
 *         queued yet (video_write_payload() in ux_device_video.c retries).
 */
UINT VENC_APP_GetData(UCHAR **data, ULONG *size)
{
	static uint32_t *curr_block = NULL;

	if (curr_block)
	{
		tx_block_release(curr_block);
		curr_block = NULL;
	}

	venc_output_frame_t frame_block;
	if (tx_queue_receive(&enc_frame_queue, (void *)&frame_block, TX_NO_WAIT) != TX_SUCCESS)
	{
		*data = NULL;
		*size = 0;
		return (UINT)-1;
	}

	*data	   = (UCHAR *)frame_block.aligned_block_addr;
	*size	   = frame_block.size;
	curr_block = frame_block.block_addr;
	return 0;
}
