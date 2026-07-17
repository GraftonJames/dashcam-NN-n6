#include "camera.h"
// HAL INCLDUES
#include <stdio.h>

// MIDDLEWARE INCLUDES
#include "cmw_camera.h"

// APP INCLUDES


HAL_StatusTypeDef DCMIPP_initNN() {
	HAL_StatusTypeDef ret;
	CMW_DCMIPP_Conf_t dcmipp_conf = {0};

	dcmipp_conf.output_width = NN_WIDTH;
	dcmipp_conf.output_height = NN_HEIGHT;
	dcmipp_conf.output_format = DCMIPP_PIXEL_PACKER_FORMAT_YUV422_1;
	dcmipp_conf.output_bpp = NN_BPP;
	dcmipp_conf.mode = CMW_ASPECT_RATIO;

	uint32_t pitch;  /* out-param: filled in by CMW_CAMERA_SetPipeConfig */
	HAL_TRY(CMW_CAMERA_SetPipeConfig(DCMIPP_PIPE1, &dcmipp_conf, &pitch));

	return HAL_OK;
}

HAL_StatusTypeDef DCMIPP_initDisplay() {

	HAL_StatusTypeDef ret;
	CMW_DCMIPP_Conf_t dcmipp_conf = {0};

	dcmipp_conf.output_width = LCD_WIDTH;
	dcmipp_conf.output_height = LCD_HEIGHT;
	dcmipp_conf.output_format = DCMIPP_PIXEL_PACKER_FORMAT_RGB565_1;
	dcmipp_conf.output_bpp = 2;
	dcmipp_conf.mode = CMW_ASPECT_RATIO;


	uint32_t pitch;
	HAL_TRY(CMW_CAMERA_SetPipeConfig(DCMIPP_PIPE1, &dcmipp_conf, &pitch));

#ifndef NDEBUG
	if (LCD_WIDTH * dcmipp_conf.output_bpp != pitch) return HAL_ERROR;
#endif

	return HAL_OK;
}

HAL_StatusTypeDef CAMERA_init() {

	HAL_StatusTypeDef ret;
	CMW_CameraInit_t cam_conf;
	CMW_Advanced_Config_t adv_conf;
	CMW_IMX335_config_t imx_conf;

	cam_conf.width = CAMERA_WIDTH;
	cam_conf.height = CAMERA_HEIGHT;
	cam_conf.fps = CAMERA_FPS;
	cam_conf.mirror_flip = CAMERA_FLIP;

	imx_conf.pixel_format = CMW_PIXEL_FORMAT_DEFAULT;  /* let cmw pick the sensor's default data type */

	adv_conf.selected_sensor = CMW_IMX335_Sensor;
	adv_conf.config_sensor.imx335_config = imx_conf;

	printf("CAMERA_init: calling CMW_CAMERA_Init\r\n");
	int32_t cmw_ret = CMW_CAMERA_Init(&cam_conf, &adv_conf);
	printf("CAMERA_init: CMW_CAMERA_Init returned %ld\r\n", (long) cmw_ret);
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

	printf("CAMERA_init: calling DCMIPP_initDisplay\r\n");
	HAL_TRY(DCMIPP_initDisplay());
	printf("CAMERA_init: calling DCMIPP_initNN\r\n");
	HAL_TRY(DCMIPP_initNN());
	printf("CAMERA_init: done\r\n");

	return HAL_OK;
}

