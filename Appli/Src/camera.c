#include "camera.h"
// HAL INCLDUES
#include <stdio.h>

// MIDDLEWARE INCLUDES
#include "cmw_camera.h"

// APP INCLUDES

/* Overrides cmw_camera.c's __weak default, which is literally `assert(0)` -
 * i.e. any transient DCMIPP pipe error (e.g. an AXI overrun under bus
 * contention) calls abort() FROM INSIDE THE DCMIPP ISR, parking the CPU in
 * newlib's _exit spin at interrupt priority: heartbeat LED dies, UART cuts
 * off mid-line, no fault registers set. Hit for real on 2026-07-22, the
 * first run where USBPD attach processing + VENC encoding + printf traffic
 * all ran concurrently (backtrace recovered off the MSP stack via ST-LINK:
 * DCMIPP_IRQHandler -> HAL_DCMIPP_PIPE_ErrorCallback -> assert -> abort).
 * For a dashcam the right policy is count-and-continue - a dropped/corrupt
 * frame is recoverable, a dead board is not. Counter is surfaced in
 * venc_app.c's periodic frame print; no printf here (ISR context). */
volatile uint32_t g_dcmipp_pipe_error_count = 0;
void CMW_CAMERA_PIPE_ErrorCallback(uint32_t pipe)
{
	(void)pipe;
	g_dcmipp_pipe_error_count++;
}

HAL_StatusTypeDef DCMIPP_initNN()
{
	HAL_StatusTypeDef ret;
	CMW_DCMIPP_Conf_t dcmipp_conf = {0};

	dcmipp_conf.output_width  = NN_WIDTH;
	dcmipp_conf.output_height = NN_HEIGHT;
	dcmipp_conf.output_format = DCMIPP_PIXEL_PACKER_FORMAT_YUV422_1;
	dcmipp_conf.output_bpp	  = NN_BPP;
	dcmipp_conf.mode	  = CMW_ASPECT_RATIO;

	uint32_t pitch; /* out-param: filled in by CMW_CAMERA_SetPipeConfig */
	/* PIPE2 (Ancillary): was incorrectly PIPE1 (colliding with Display) before
	 * Phase 3. Fixed, but this function is not currently called - NN stays
	 * disabled while VENC is being brought up, to isolate risk (Phase 3 plan). */
	HAL_TRY(CMW_CAMERA_SetPipeConfig(DCMIPP_PIPE2, &dcmipp_conf, &pitch));

	return HAL_OK;
}

HAL_StatusTypeDef DCMIPP_initVenc()
{
	HAL_StatusTypeDef ret;
	CMW_DCMIPP_Conf_t dcmipp_conf = {0};

	dcmipp_conf.output_width  = VENC_WIDTH;
	dcmipp_conf.output_height = VENC_HEIGHT;
	dcmipp_conf.output_format = DCMIPP_PIXEL_PACKER_FORMAT_YUV422_1;
	dcmipp_conf.output_bpp	  = 2;
	dcmipp_conf.mode	  = CMW_ASPECT_RATIO;

	uint32_t pitch = 0;
	/* PIPE1 (Main), not PIPE0 (Dump) or PIPE2 (Ancillary):
	 * Drivers/cmw_camera/cmw_camera.c's CMW_CAMERA_SetPipe() has an explicit
	 * vendor stub for PIPE0 ("specific case for pipe0 which is only a dump
	 * pipe... TODO: properly configure the dump pipe with decimation and
	 * crop") that ignores width/height/format entirely and never writes the
	 * pitch out-param. PIPE2 goes through the full config path but hits a
	 * real hardware constraint instead - "Only pipe 1 support YUV
	 * conversion" - so PIPE2 returns CMW_ERROR_FEATURE_NOT_SUPPORTED for our
	 * YUV422 format. PIPE1 is the only pipe that actually supports it, which
	 * is why Display (the other PIPE1 user, RGB565) is disabled above. */
	/* CMW_CAMERA_SetPipeConfig() actually returns an int32_t CMW_ERROR_* code
	 * (cmw_errno.h), not a HAL_StatusTypeDef - storing it in `ret` (whose
	 * enum has no negative enumerators, so this toolchain packs it into a
	 * single byte) truncated -11 into 245 the first time this was printed. */
	int32_t cmw_pipe_ret = CMW_CAMERA_SetPipeConfig(DCMIPP_PIPE1, &dcmipp_conf, &pitch);
	printf("DCMIPP_initVenc: CMW_CAMERA_SetPipeConfig(PIPE1) returned %ld, pitch=%lu (expect %lu)\r\n",
	       (long)cmw_pipe_ret, (unsigned long)pitch, (unsigned long)(VENC_WIDTH * dcmipp_conf.output_bpp));
	if (cmw_pipe_ret != CMW_ERROR_NONE)
		return HAL_ERROR;

#ifndef NDEBUG
	if (VENC_WIDTH * dcmipp_conf.output_bpp != pitch)
		return HAL_ERROR;
#endif

	return HAL_OK;
}

HAL_StatusTypeDef DCMIPP_initDisplay()
{

	HAL_StatusTypeDef ret;
	CMW_DCMIPP_Conf_t dcmipp_conf = {0};

	dcmipp_conf.output_width  = LCD_WIDTH;
	dcmipp_conf.output_height = LCD_HEIGHT;
	dcmipp_conf.output_format = DCMIPP_PIXEL_PACKER_FORMAT_RGB565_1;
	dcmipp_conf.output_bpp	  = 2;
	dcmipp_conf.mode	  = CMW_ASPECT_RATIO;

	uint32_t pitch;
	HAL_TRY(CMW_CAMERA_SetPipeConfig(DCMIPP_PIPE1, &dcmipp_conf, &pitch));

#ifndef NDEBUG
	if (LCD_WIDTH * dcmipp_conf.output_bpp != pitch)
		return HAL_ERROR;
#endif

	return HAL_OK;
}

HAL_StatusTypeDef CAMERA_init()
{

	HAL_StatusTypeDef     ret;
	CMW_CameraInit_t      cam_conf;
	CMW_Advanced_Config_t adv_conf;
	CMW_IMX335_config_t   imx_conf;

	cam_conf.width	     = CAMERA_WIDTH;
	cam_conf.height	     = CAMERA_HEIGHT;
	cam_conf.fps	     = CAMERA_FPS;
	cam_conf.mirror_flip = CAMERA_FLIP;

	imx_conf.pixel_format = CMW_PIXEL_FORMAT_DEFAULT; /* let cmw pick the sensor's default data type */

	adv_conf.selected_sensor	     = CMW_IMX335_Sensor;
	adv_conf.config_sensor.imx335_config = imx_conf;

	printf("CAMERA_init: calling CMW_CAMERA_Init\r\n");
	int32_t cmw_ret = CMW_CAMERA_Init(&cam_conf, &adv_conf);
	printf("CAMERA_init: CMW_CAMERA_Init returned %ld\r\n", (long)cmw_ret);
	if (cmw_ret != CMW_ERROR_NONE)
	{
		/* Sensor probe/config failed (e.g. camera unplugged) - camera_conf
		 * (width/height used by the crop math below) is only populated on a
		 * successful CMW_CAMERA_Init, so it's still zeroed here. Proceeding
		 * into pipe config would hit CMW_UTILS_get_crop_config's
		 * "assert(ratio >= 1)" with cam_width=0 regardless of any dimensions
		 * configured below - bail out instead. */
		return HAL_ERROR;
	}

	/* DCMIPP_initDisplay() deliberately not called: Drivers/cmw_camera/
	 * cmw_camera.c's CMW_CAMERA_SetPipe() hard-codes "Only pipe 1 support
	 * YUV conversion" (a real DCMIPP hardware constraint, confirmed via
	 * source + hardware trace - PIPE2 returned CMW_ERROR_FEATURE_NOT_SUPPORTED
	 * for YUV422). VENC's H264 encoder needs YUV422 input, so it needs PIPE1,
	 * which Display was using for RGB565 - same "isolate risk" tradeoff
	 * already applied to NN below, just discovered one layer deeper once
	 * boot got this far. Revisit once VENC is proven on hardware. */
	/* DCMIPP_initNN() deliberately not called: NN stays disabled while VENC is
	 * brought up on Phase 3's third pipe, to isolate risk (Phase 3 plan). */
	printf("CAMERA_init: calling DCMIPP_initVenc\r\n");
	HAL_TRY(DCMIPP_initVenc());
	printf("CAMERA_init: done\r\n");

	return HAL_OK;
}
