/**
 ******************************************************************************
 * @file    venc_app.c
 * @brief   VENC H264 encoder driving loop - Phase 3 (UVC port plan).
 ******************************************************************************
 * Deliberately minimal compared to VENC_USB's reference venc_app.c: this only
 * proves the encoder itself works (H264EncInit succeeds, steady frame
 * counter, valid-looking NAL output over UART) per the Phase 3 plan's
 * verification goal. No USB/UVC output queue, no ThreadX block-pool frame
 * handoff, no dynamic bitrate/error recovery - that plumbing belongs to
 * Phase 4, once this half is confirmed working on real hardware.
 ******************************************************************************
 */
#include "main.h"
#include "camera.h"
#include "venc_h264_config.h"
#include "h264encapi.h"
#include "tx_api.h"

/* Compressed output rarely exceeds a small fraction of the raw frame size at
 * this bitrate/resolution; sized to fit the remaining .noncacheable budget
 * after ewl_pool + input_frame (see venc_h264_config.c for the full budget
 * accounting). Same "safe to underestimate" caveat: H264EncStrmEncode()
 * reports a clean error if this is too small, not a corruption. */
#define VENC_OUTPUT_BUF_SIZE (128UL * 1024UL)
static uint8_t venc_output_buf[VENC_OUTPUT_BUF_SIZE] __attribute__((aligned(32))) __attribute__((section(".noncacheable")));

static TX_THREAD      venc_thread;
static uint8_t	      venc_thread_stack[2048];
static H264EncInst    venc_encoder;
static H264EncIn      venc_enc_in;
static H264EncOut     venc_enc_out;

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
	H264EncRet ret;
	uint32_t   frame_nb = 0;

	(void)initial_input;

	if (venc_encoder_prepare() != 0)
	{
		printf("VENC: encoder_prepare FAILED, venc_thread exiting\r\n");
		return;
	}

	/* Frame mode: capture straight into input_frame, continuously. No HW
	 * handshake with the encoder - DCMIPP just keeps input_frame current.
	 * PIPE1 (Main), matching DCMIPP_initVenc() in camera.c - the only pipe
	 * that supports YUV422 conversion in hardware, see the comment there. */
	if (CMW_CAMERA_Start(DCMIPP_PIPE1, GetInputFrame(NULL), CMW_MODE_CONTINUOUS) != CMW_ERROR_NONE)
	{
		printf("VENC: CMW_CAMERA_Start FAILED, venc_thread exiting\r\n");
		return;
	}

	venc_enc_in.busLuma	= (ptr_t)GetInputFrame(NULL);
	venc_enc_in.pOutBuf	= (u32 *)venc_output_buf;
	venc_enc_in.busOutBuf	= (size_t)venc_output_buf;
	venc_enc_in.outBufSize = VENC_OUTPUT_BUF_SIZE;
	venc_enc_in.codingType	= H264ENC_INTRA_FRAME;
	venc_enc_in.ipf		= H264ENC_REFERENCE_AND_REFRESH;
	venc_enc_in.ltrf	= H264ENC_REFERENCE;

	ret = H264EncStrmStart(venc_encoder, &venc_enc_in, &venc_enc_out);
	printf("VENC: H264EncStrmStart returned %d\r\n", (int)ret);
	if (ret != H264ENC_OK)
	{
		return;
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

		ret = H264EncStrmEncode(venc_encoder, &venc_enc_in, &venc_enc_out, NULL, NULL, NULL);
		if (ret == H264ENC_FRAME_READY)
		{
			extern volatile uint32_t g_dcmipp_pipe_error_count; /* camera.c */
			printf("VENC: frame=%lu size=%lu bytes, first=%02X %02X %02X %02X, pipe_errs=%lu\r\n",
			       (unsigned long)frame_nb, (unsigned long)venc_enc_out.streamSize,
			       venc_output_buf[0], venc_output_buf[1], venc_output_buf[2], venc_output_buf[3],
			       (unsigned long)g_dcmipp_pipe_error_count);
			frame_nb++;
		}
		else
		{
			printf("VENC: H264EncStrmEncode returned %d (frame=%lu)\r\n", (int)ret, (unsigned long)frame_nb);
		}

		tx_thread_sleep(100); /* ~1s at 100Hz kernel tick; well under VENC_FRAMERATE's pace, just enough to see steady progress over UART */
	}
}

void VENC_APP_ThreadCreate(void)
{
	tx_thread_create(&venc_thread, "venc_thread", venc_thread_entry, 0,
			 venc_thread_stack, sizeof(venc_thread_stack),
			 20, 20, TX_NO_TIME_SLICE, TX_AUTO_START);
}
