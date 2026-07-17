// Hardware Selection
#define STM32N657xx 1
#define STM32N6570_NUCLEO_REV 1

#define GAMMA_CORRECTION 0

#define CMW_ASPECT_RATIO CMW_Aspect_ratio_crop
// Camera Conf
#define CAMERA_WIDTH 2592
#define CAMERA_HEIGHT 1944
#define CAMERA_FLIP 1
#define CAMERA_FPS 30

// Pipe1 LCD/record DCMIPP Init
#define LCD_HEIGHT 480
#define LCD_WIDTH 640

// Pipe2 NN DCMIPP Init
#define NN_HEIGHT 240
#define NN_WIDTH 320
#define NN_BPP 2 

// HAL TRY
#ifndef NDEBUG
#define HAL_TRY(FUNC) \
	ret = FUNC; \
	if (ret != HAL_OK) return ret
#else
#define HAL_TRY(FUNC) FUNC
#endif
