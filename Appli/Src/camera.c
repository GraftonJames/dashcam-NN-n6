#include "camera.h"
// HAL INCLDUES

// MIDDLEWARE INCLUDES
#include "cmw_camera.h"

// APP INCLUDES 


HAL_StatusTypeDef DCMIPP_initNN() {
	HAL_StatusTypeDef ret;
	CMW_DCMIPP_Conf_t dcmipp_conf = {0};

	dcmipp_conf.output_width = NN_WIDTH;
	dcmipp_conf.output_height = NN_HEIGHT;
	dcmipp_conf.output_height = DCMIPP_PIXEL_PACKER_FORMAT_YUV444_1;
	dcmipp_conf.output_bpp = NN_BPP;
	dcmipp_conf.mode = CMW_ASPECT_RATIO;

	uint32_t pitch = NN_PITCH;
	HAL_TRY(CMW_CAMERA_SetPipeConfig(DCMIPP_PIPE1, &dcmipp_conf, &pitch));

	return HAL_OK;
}

HAL_StatusTypeDef DCMIPP_initDisplay() {

	HAL_StatusTypeDef ret;
	CMW_DCMIPP_Conf_t dcmipp_conf = {0};

	dcmipp_conf.output_width = LCD_WIDTH;
	dcmipp_conf.output_height = LCD_HEIGHT;
	dcmipp_conf.output_height = DCMIPP_PIXEL_PACKER_FORMAT_RGB565_1;
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

	imx_conf.pixel_format = 0; //idk what this does yet TODO;

	adv_conf.selected_sensor = CMW_IMX335_Sensor;
	adv_conf.config_sensor.imx335_config = imx_conf;

	int cmw_ret = CMW_CAMERA_Init(&cam_conf, &adv_conf);
	// maybe add error checking here later

	HAL_TRY(DCMIPP_initDisplay());
	HAL_TRY(DCMIPP_initNN());

	return HAL_OK;
}

