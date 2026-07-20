/**
 ******************************************************************************
 * @file    venc_h264_config.c
 * @brief   VENC H264 encoder configuration - Phase 3 (UVC port plan).
 ******************************************************************************
 * Adapted from ST's VENC_USB reference (venc_h264_config.c +
 * venc_h264_config_480p_Frame.h), collapsed into one file since we're not
 * supporting multiple resolution presets. Two deliberate departures from the
 * reference, both driven by PSRAM not being confirmed present on this board
 * (see the UVC port plan's RAM budget analysis):
 *   - Buffers moved from PSRAM (IN_PSRAM) to our own .noncacheable section
 *     (internal SRAM3-6, grown into by the linker script this phase).
 *   - CAM_NB_BUFFERS reduced from 2 (double-buffered) to 1, to fit the
 *     ~1.75MB SRAM headroom alongside the encoder pool.
 * VENC_POOL_SIZE is scaled down from the reference's 8MB (sized for 1080p)
 * to roughly match 480p's share of that - this is an ESTIMATE, not a value
 * computed from the library's actual internal requirements (no public sizing
 * formula was found for this library version). H264EncInit() will report a
 * clean, diagnosable error if it's undersized rather than corrupting memory,
 * so this is safe to get wrong and adjust once tested on hardware.
 ******************************************************************************
 */
#include "app_conf.h"
#include "venc_h264_config.h"
#include "stm32n6xx_hal_dcmipp.h"

#define ALIGN_32	  __attribute__((aligned(32)))
#define VENC_NONCACHEABLE __attribute__((section(".noncacheable")))

#define DCMIPP_FORMAT		 DCMIPP_PIXEL_PACKER_FORMAT_YUV422_1
#define DCMIPP_BYTES_PER_PIXELS (2) /* YUV422: 16 bits per pixel */

/* Frame mode (not HW handshake/Slice mode): DCMIPP captures whole frames,
 * VENC reads/encodes them independently - no tight hardware-triggered
 * line-by-line coupling. Simpler and lower-risk for initial bring-up than
 * the HW handshake mode VENC_USB's DK reference defaults to. */
#define CAM_NB_BUFFERS	     (1)
#define INPUT_FRAME_SIZE     (VENC_WIDTH * VENC_HEIGHT * DCMIPP_BYTES_PER_PIXELS * CAM_NB_BUFFERS)
#define VIEW_MODE	     H264ENC_BASE_VIEW_SINGLE_BUFFER

/* See file header: scaled down from the reference's 8MB (1080p) estimate,
 * not computed from the library's real internal requirement. */
#define VENC_POOL_SIZE (1024UL * 1024UL)

/* Memory Pools - internal SRAM (.noncacheable), not PSRAM */
uint8_t ewl_pool[VENC_POOL_SIZE] ALIGN_32 VENC_NONCACHEABLE;

/* Input frame (camera capture) */
uint8_t input_frame[INPUT_FRAME_SIZE] ALIGN_32 VENC_NONCACHEABLE;

/* Sensor's native capture resolution (before DCMIPP downsizes to
 * VENC_WIDTH/HEIGHT) - matches CAMERA_WIDTH/HEIGHT, the same values
 * CMW_CAMERA_Init() already configures for Display/NN. */
cam_h264_cfg_t hCamH264Instance = {
	.width	= CAMERA_WIDTH,
	.height = CAMERA_HEIGHT,
};

dcmipp_h264_cfg_t hDcmippH264Instance = {
	.bytes_per_pixel = DCMIPP_BYTES_PER_PIXELS,
	.format		 = DCMIPP_FORMAT,
	.pitch		 = VENC_WIDTH * DCMIPP_BYTES_PER_PIXELS,
};

venc_h264_cfg_t hVencH264Instance = {
	.cfgH264Main.streamType	   = H264ENC_BYTE_STREAM, /* Byte stream / Plain NAL units */
	.cfgH264Main.viewMode	   = VIEW_MODE,
	.cfgH264Main.width	   = VENC_WIDTH,
	.cfgH264Main.height	   = VENC_HEIGHT,
	.cfgH264Main.level	   = H264ENC_LEVEL_4,
	.cfgH264Main.frameRateNum   = VENC_FRAMERATE,
	.cfgH264Main.frameRateDenom = 1,
	.cfgH264Main.scaledWidth   = 0,
	.cfgH264Main.scaledHeight  = 0,
	.cfgH264Main.refFrameAmount	= 1,
	.cfgH264Main.refFrameCompress	= 0,
	.cfgH264Main.rfcLumBufLimit	= 0,
	.cfgH264Main.rfcChrBufLimit	= 0,
	.cfgH264Main.svctLevel		= 0,
	.cfgH264Main.enableSvctPrefix	= 0,
	.cfgH264Preproc.origWidth	= VENC_WIDTH,
	.cfgH264Preproc.origHeight	= VENC_HEIGHT,
	.cfgH264Preproc.xOffset		= 0,
	.cfgH264Preproc.yOffset		= 0,
	.cfgH264Preproc.inputType	= H264ENC_YUV422_INTERLEAVED_YUYV,
	.cfgH264Preproc.rotation	= H264ENC_ROTATE_0,
	.cfgH264Preproc.videoStabilization = 0,
	.cfgH264Preproc.colorConversion	   = H264ENC_RGBTOYUV_BT601,
	.cfgH264Preproc.scaledOutput	   = 0,
	.cfgH264Preproc.interlacedFrame	   = 0,
	.cfgH264Coding.sliceSize		= 0,
	.cfgH264Coding.seiMessages		= 0,
	.cfgH264Coding.idrHeader		= 1,
	.cfgH264Coding.videoFullRange		= 0,
	.cfgH264Coding.constrainedIntraPrediction = 0,
	.cfgH264Coding.disableDeblockingFilter	   = 0,
	.cfgH264Coding.sampleAspectRatioWidth	   = 0,
	.cfgH264Coding.sampleAspectRatioHeight	   = 0,
	.cfgH264Coding.enableCabac		   = 1,
	.cfgH264Coding.cabacInitIdc		   = 0,
	.cfgH264Coding.transform8x8Mode	   = 1,
	.cfgH264Coding.quarterPixelMv		   = 0,
	.cfgH264Coding.cirStart			   = 0,
	.cfgH264Coding.cirInterval		   = 0,
	.cfgH264Coding.intraSliceMap1		   = 0,
	.cfgH264Coding.intraSliceMap2		   = 0,
	.cfgH264Coding.intraSliceMap3		   = 0,
	.cfgH264Coding.roi1DeltaQp		   = 0,
	.cfgH264Coding.adaptiveRoi		   = 0,
	.cfgH264Coding.adaptiveRoiColor		   = 0,
	.cfgH264Coding.roiMapEnable		   = 0,
	.cfgH264Coding.fieldOrder		   = 0,
	.cfgH264Coding.gdrDuration		   = 0,
	.cfgH264Coding.svctLevel		   = 0,
	.cfgH264Coding.noiseReductionEnable	   = 0,
	.cfgH264Coding.noiseLow			   = 1,
	.cfgH264Coding.noiseLevel		   = 1,
	.cfgH264Coding.inputLineBufEn		   = 0, /* Frame mode: no input line control signals */
	.cfgH264Coding.inputLineBufLoopBackEn	   = 0,
	.cfgH264Coding.inputLineBufDepth	   = 0,
	.cfgH264Coding.inputLineBufHwModeEn	   = 0, /* Frame mode: no HW handshake */
	.cfgH264Coding.nBaseLayerPID		   = 0,
	.cfgH264Coding.level			   = 0,
	.cfgH264Coding.enableSVC		   = 0,
	.cfgH264Rate.pictureRc		= 0,
	.cfgH264Rate.mbRc		= 0,
	.cfgH264Rate.pictureSkip	= 0,
	.cfgH264Rate.qpHdr		= 26,
	.cfgH264Rate.qpMin		= 20,
	.cfgH264Rate.qpMax		= 42,
	.cfgH264Rate.bitPerSecond	= 128000,
	.cfgH264Rate.hrd		= 0,
	.cfgH264Rate.hrdCpbSize		= 30000000,
	.cfgH264Rate.gopLen		= 30,
	.cfgH264Rate.intraQpDelta	= -3,
	.cfgH264Rate.fixedIntraQp	= 0,
	.cfgH264Rate.mbQpAdjustment	= 0,
	.cfgH264Rate.longTermPicRate	= 15,
	.cfgH264Rate.mbQpAutoBoost	= 0,
};

void EWLPoolChoiceCb(u8 **pool_ptr, size_t *size)
{
	*pool_ptr = ewl_pool;
	*size	  = VENC_POOL_SIZE;
}

void EWLPoolReleaseCb(u8 **pool_ptr)
{
	UNUSED(pool_ptr);
}

uint8_t *GetInputFrame(uint32_t *frameSize)
{
	if (frameSize)
	{
		*frameSize = INPUT_FRAME_SIZE;
	}
	return input_frame;
}
