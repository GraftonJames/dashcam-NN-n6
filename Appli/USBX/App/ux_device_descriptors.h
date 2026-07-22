/**
 ******************************************************************************
 * @file    ux_device_descriptors.h
 * @brief   USBX device descriptor header - Phase 4 (UVC port plan).
 ******************************************************************************
 * Adapted from ST's VENC_USB reference (Appli/USBX/App/ux_device_descriptors.h).
 * The .c file's descriptor builder is entirely driven by the macros below (no
 * board-specific code in the .c itself), so it's copied unmodified; this
 * header is the only thing that needed adapting - resolution/framerate now
 * come from app_conf.h (VENC_WIDTH/VENC_HEIGHT/VENC_FRAMERATE) instead of the
 * reference's hardcoded 320x236 @ 25fps, to match Phase 3's encoder config.
 ******************************************************************************
 */
#ifndef __UX_DEVICE_DESCRIPTORS_H__
#define __UX_DEVICE_DESCRIPTORS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ux_api.h"
#include "ux_stm32_config.h"
#include "ux_device_class_video.h"
#include "app_conf.h"

#define USBD_MAX_NUM_CONFIGURATION 1U
#define USBD_MAX_SUPPORTED_CLASS   3U
#define USBD_MAX_CLASS_ENDPOINTS   9U
#define USBD_MAX_CLASS_INTERFACES  12U

#define USBD_VIDEO_CLASS_ACTIVATED 1U

#define USBD_CONFIG_MAXPOWER                  25U
#define USBD_COMPOSITE_USE_IAD                1U
#define USBD_DEVICE_FRAMEWORK_BUILDER_ENABLED 1U

#define USBD_FRAMEWORK_MAX_DESC_SZ       500U
#define USBD_UVC_USE_FRAME_BASE_H264     1U
#define USBD_UVC_USE_H264                0U
#define USBD_UVC_USE_CAMERA              1U
#define USBD_DEVICE_VIDEO_PC_PROTOCOL_15 0U

typedef enum
{
	CLASS_TYPE_NONE	    = 0,
	CLASS_TYPE_HID	    = 1,
	CLASS_TYPE_CDC_ACM  = 2,
	CLASS_TYPE_MSC	    = 3,
	CLASS_TYPE_CDC_ECM  = 4,
	CLASS_TYPE_DFU	    = 5,
	CLASS_TYPE_VIDEO    = 6,
	CLASS_TYPE_PIMA_MTP = 7,
	CLASS_TYPE_CCID	    = 8,
	CLASS_TYPE_PRINTER  = 9,
	CLASS_TYPE_RNDIS    = 10,
} USBD_CompositeClassTypeDef;

typedef struct
{
	uint32_t status;
	uint32_t total_length;
	uint32_t rem_length;
	uint32_t maxpacket;
	uint16_t is_used;
	uint16_t bInterval;
} USBD_EndpointTypeDef;

typedef struct
{
	uint8_t	 add;
	uint8_t	 type;
	uint16_t size;
	uint8_t	 is_used;
} USBD_EPTypeDef;

typedef struct
{
	USBD_CompositeClassTypeDef ClassType;
	uint32_t		    ClassId;
	uint8_t			    InterfaceType;
	uint32_t		    Active;
	uint32_t		    NumEps;
	uint32_t		    NumIf;
	USBD_EPTypeDef		    Eps[USBD_MAX_CLASS_ENDPOINTS];
	uint8_t			    Ifs[USBD_MAX_CLASS_INTERFACES];
} USBD_CompositeElementTypeDef;

typedef struct _USBD_DevClassHandleTypeDef
{
	uint8_t			      Speed;
	uint32_t		      classId;
	uint32_t		      NumClasses;
	USBD_CompositeElementTypeDef tclasslist[USBD_MAX_SUPPORTED_CLASS];
	uint32_t		      CurrDevDescSz;
	uint32_t		      CurrConfDescSz;
} USBD_DevClassHandleTypeDef;

typedef enum
{
	OUT = 0x00,
	IN  = 0x80,
} USBD_EPDirectionTypeDef;

typedef struct
{
	uint8_t	 bLength;
	uint8_t	 bDescriptorType;
	uint16_t bcdUSB;
	uint8_t	 bDeviceClass;
	uint8_t	 bDeviceSubClass;
	uint8_t	 bDeviceProtocol;
	uint8_t	 bMaxPacketSize;
	uint16_t idVendor;
	uint16_t idProduct;
	uint16_t bcdDevice;
	uint8_t	 iManufacturer;
	uint8_t	 iProduct;
	uint8_t	 iSerialNumber;
	uint8_t	 bNumConfigurations;
} __PACKED USBD_DeviceDescTypedef;

typedef struct
{
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bFirstInterface;
	uint8_t bInterfaceCount;
	uint8_t bFunctionClass;
	uint8_t bFunctionSubClass;
	uint8_t bFunctionProtocol;
	uint8_t iFunction;
} __PACKED USBD_IadDescTypedef;

typedef struct
{
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bInterfaceNumber;
	uint8_t bAlternateSetting;
	uint8_t bNumEndpoints;
	uint8_t bInterfaceClass;
	uint8_t bInterfaceSubClass;
	uint8_t bInterfaceProtocol;
	uint8_t iInterface;
} __PACKED USBD_IfDescTypedef;

typedef struct
{
	uint8_t	 bLength;
	uint8_t	 bDescriptorType;
	uint8_t	 bEndpointAddress;
	uint8_t	 bmAttributes;
	uint16_t wMaxPacketSize;
	uint8_t	 bInterval;
} __PACKED USBD_EpDescTypedef;

typedef struct
{
	uint8_t	 bLength;
	uint8_t	 bDescriptorType;
	uint16_t wDescriptorLength;
	uint8_t	 bNumInterfaces;
	uint8_t	 bConfigurationValue;
	uint8_t	 iConfiguration;
	uint8_t	 bmAttributes;
	uint8_t	 bMaxPower;
} __PACKED USBD_ConfigDescTypedef;

typedef struct
{
	uint8_t	 bLength;
	uint8_t	 bDescriptorType;
	uint16_t bcdDevice;
	uint8_t	 Class;
	uint8_t	 SubClass;
	uint8_t	 Protocol;
	uint8_t	 bMaxPacketSize;
	uint8_t	 bNumConfigurations;
	uint8_t	 bReserved;
} __PACKED USBD_DevQualiDescTypedef;

#if USBD_VIDEO_CLASS_ACTIVATED == 1

typedef struct
{
	uint8_t	 bLength;
	uint8_t	 bDescriptorType;
	uint8_t	 bDescriptorSubtype;
	uint16_t bcdUVC;
	uint16_t wTotalLength;
	uint32_t dwClockFrequency;
	uint8_t	 bInCollection;
	uint8_t	 aInterfaceNr;
} __PACKED USBD_VIDEOCSVCIfDescTypeDef;

typedef struct
{
	uint8_t	 bLength;
	uint8_t	 bDescriptorType;
	uint8_t	 bDescriptorSubtype;
	uint8_t	 bTerminalID;
	uint16_t wTerminalType;
	uint8_t	 bAssocTerminal;
	uint8_t	 iTerminal;
#if (USBD_UVC_USE_CAMERA == 1U)
	uint16_t wObjectiveFocalLengthMin;
	uint16_t wObjectiveFocalLengthMax;
	uint16_t wOcularFocalLength;
	uint8_t	 bControlSize;
	uint8_t	 bmControls[3];
#endif
} __PACKED USBD_VIDEOInputTerminalDescTypeDef;

typedef struct
{
	uint8_t	 bLength;
	uint8_t	 bDescriptorType;
	uint8_t	 bDescriptorSubtype;
	uint8_t	 bTerminalID;
	uint16_t wTerminalType;
	uint8_t	 bAssocTerminal;
	uint8_t	 bSourceID;
	uint8_t	 iTerminal;
} __PACKED USBD_VIDEOOutputTerminalDescTypeDef;

typedef struct
{
	uint8_t	 bLength;
	uint8_t	 bDescriptorType;
	uint8_t	 bDescriptorSubtype;
	uint8_t	 bNumFormats;
	uint16_t wTotalLength;
	uint8_t	 bEndpointAddress;
	uint8_t	 bmInfo;
	uint8_t	 bTerminalLink;
	uint8_t	 bStillCaptureMethod;
	uint8_t	 bTriggerSupport;
	uint8_t	 bTriggerUsage;
	uint8_t	 bControlSize;
	uint8_t	 bmaControls;
} __PACKED USBD_VIDEOVSHeaderDescTypeDef;

typedef struct
{
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubType;
	uint8_t bFormatIndex;
	uint8_t bNumFrameDescriptors;
#if (USBD_UVC_USE_H264 == 1U)
	uint8_t	 bDefaultFrameIndex;
	uint8_t	 bMaxCodecConfigDelay;
	uint8_t	 bmSupportedSliceModes;
	uint8_t	 bmSupportedSyncFrameTypes;
	uint8_t	 bResolutionScaling;
	uint8_t	 Reserved;
	uint8_t	 bmSupportedRateControlModes;
	uint16_t wMaxMBperSecOneResNoScal;
	uint16_t wMaxMBperSecTwoResNoScal;
	uint16_t wMaxMBperSecThreeResNoScal;
	uint16_t wMaxMBperSecFourResNoScal;
	uint16_t wMaxMBperSecOneResTemporalScal;
	uint16_t wMaxMBperSecTwoResTemporalScal;
	uint16_t wMaxMBperSecThreeResTemporalScal;
	uint16_t wMaxMBperSecFourResTemporalScal;
	uint16_t wMaxMBperSecOneResTemporalQualityScal;
	uint16_t wMaxMBperSecTwoResTemporalQualityScal;
	uint16_t wMaxMBperSecThreeResTemporalQualityScal;
	uint16_t wMaxMBperSecFourRessTemporalQualityScal;
	uint16_t wMaxMBperSecOneResTemporalSpatialScal;
	uint16_t wMaxMBperSecTwoResTemporalSpatialScal;
	uint16_t wMaxMBperSecThreeResTemporalSpatialScal;
	uint16_t wMaxMBperSecFourResTemporalSpatialScal;
	uint16_t wMaxMBperSecOneResFullScal;
	uint16_t wMaxMBperSecTwoResFullScal;
	uint16_t wMaxMBperSecThreeRessFullScal;
	uint16_t wMaxMBperSecFourResFullScal;
#elif (USBD_UVC_USE_FRAME_BASE_H264 == 1)
	uint8_t pGiudFormat[16];
	uint8_t bBitsPerPixel;
	uint8_t bDefaultFrameIndex;
	uint8_t bAspectRatioX;
	uint8_t bAspectRatioY;
	uint8_t bmInterlaceFlag;
	uint8_t bCopyProtect;
	uint8_t bVariableSize;
#else
	uint8_t bmFlags;
	uint8_t bDefaultFrameIndex;
	uint8_t bAspectRatioX;
	uint8_t bAspectRatioY;
	uint8_t bmInterlaceFlag;
	uint8_t bCopyProtect;
#endif
} __PACKED USBD_VIDEOPayloadFormatDescTypeDef;

typedef struct
{
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubType;
	uint8_t bColorPrimarie;
	uint8_t bTransferCharacteristics;
	uint8_t bMatrixCoefficients;
} __PACKED USBD_ColorMatchingDescTypeDef;

typedef struct
{
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubType;
	uint8_t bFrameIndex;
#if (USBD_UVC_USE_H264 == 1U)
	uint16_t wWidth;
	uint16_t wHeight;
	uint16_t wSARwidth;
	uint16_t wSARheight;
	uint16_t wProfile;
	uint8_t	 bLevelIDC;
	uint16_t wConstrainedToolset;
	uint32_t bmSupportedUsages;
	uint16_t bmCapabilities;
	uint32_t bmSVCCapabilities;
	uint32_t bmMVCCapabilities;
	uint32_t dwMinBitRate;
	uint32_t dwMaxBitRate;
	uint32_t dwDefaultFrameInterval;
	uint8_t	 bNumFrameIntervals;
	uint32_t dwFrameInterval;
#elif (USBD_UVC_USE_FRAME_BASE_H264 == 1U)
	uint8_t	 bmCapabilities;
	uint16_t wWidth;
	uint16_t wHeight;
	uint32_t dwMinBitRate;
	uint32_t dwMaxBitRate;
	uint32_t dwDefaultFrameInterval;
	uint8_t	 bFrameIntervalType;
	uint32_t dwBytesPerLine;
	uint32_t dwFrameInterval;
#else
	uint8_t	 bmCapabilities;
	uint16_t wWidth;
	uint16_t wHeight;
	uint32_t dwMinBitRate;
	uint32_t dwMaxBitRate;
	uint32_t dwMaxVideoFrameBufferSize;
	uint32_t dwDefaultFrameInterval;
	uint8_t	 bFrameIntervalType;
	uint32_t dwFrameInterval;
#endif
} __PACKED USBD_VIDEOFrameDescTypeDef;

#endif /* USBD_VIDEO_CLASS_ACTIVATED */

uint8_t *USBD_Get_Device_Framework_Speed(uint8_t Speed, ULONG *Length);
uint8_t *USBD_Get_String_Framework(ULONG *Length);
uint8_t *USBD_Get_Language_Id_Framework(ULONG *Length);
uint16_t USBD_Get_Interface_Number(uint8_t class_type, uint8_t interface_type);
uint16_t USBD_Get_Configuration_Number(uint8_t class_type, uint8_t interface_type);

#define USBD_VID	   0x0483
#define USBD_PID	   0x7753
#define USBD_LANGID_STRING 1033
#define USBD_MANUFACTURER_STRING    "STMicroelectronics"
#define USBD_PRODUCT_STRING	     "N6 Dashcam UVC"
#define USBD_SERIAL_NUMBER	     "DASHCAM01"
#define USBD_UVC_INTERFACE_STRING   "N6 Dashcam H264 video Streaming"

#define USB_DESC_TYPE_INTERFACE     0x04U
#define USB_DESC_TYPE_ENDPOINT	     0x05U
#define USB_DESC_TYPE_CONFIGURATION 0x02U
#define USB_DESC_TYPE_IAD	     0x0BU

#define USBD_EP_TYPE_CTRL 0x00U
#define USBD_EP_TYPE_ISOC 0x01U
#define USBD_EP_TYPE_BULK 0x02U
#define USBD_EP_TYPE_INTR 0x03U

#define USBD_EP_ATTR_ISOC_NOSYNC 0x00U
#define USBD_EP_ATTR_ISOC_ASYNC  0x04U
#define USBD_EP_ATTR_ISOC_ADAPT  0x08U
#define USBD_EP_ATTR_ISOC_SYNC	  0x0CU

#define USBD_FULL_SPEED 0x00U
#define USBD_HIGH_SPEED 0x01U

#define USB_BCDUSB	    0x0200U
#define LANGUAGE_ID_MAX_LENGTH 2U

#define USBD_IDX_MFC_STR	    0x01U
#define USBD_IDX_PRODUCT_STR	    0x02U
#define USBD_IDX_SERIAL_STR	    0x03U
#define USBD_IDX_UVC_INTERFACE_STR 0x04U

#define USBD_MAX_EP0_SIZE		 64U
#define USBD_DEVICE_QUALIFIER_DESC_SIZE 0x0AU

#define USBD_STRING_FRAMEWORK_MAX_LENGTH 256U

/* Device VIDEO Class */
#define USBD_VIDEO_EPIN_ADDR	      0x81U
#define USBD_VIDEO_EPIN_FS_MPS	      1023U
#define USBD_VIDEO_EPIN_HS_MPS	      1024U
#define USBD_VIDEO_EPIN_FS_BINTERVAL 1U
#define USBD_VIDEO_EPIN_HS_BINTERVAL 1U

#define UVC_GUID_YUY2 0x32595559U
#define UVC_GUID_NV12 0x3231564EU
#define UVC_GUID_H264 0x34363248U

/* Resolution/framerate come from app_conf.h (Phase 3's encoder config) rather
 * than being redefined here, so the two can never drift apart. */
#define UVC_FRAME_WIDTH	 VENC_WIDTH
#define UVC_FRAME_HEIGHT VENC_HEIGHT
#define UVC_CAM_FPS_FS	 VENC_FRAMERATE
#define UVC_CAM_FPS_HS	 VENC_FRAMERATE

#define UVC_MIN_BIT_RATE(n) (UVC_FRAME_WIDTH * UVC_FRAME_HEIGHT * 16U * (n))
#define UVC_MAX_BIT_RATE(n) (UVC_FRAME_WIDTH * UVC_FRAME_HEIGHT * 16U * (n))
#define UVC_INTERVAL(n)	     (10000000U / (n))
#define UVC_MAX_FRAME_SIZE  (UVC_FRAME_WIDTH * UVC_FRAME_HEIGHT * 16U / 2U)

#define VS_FORMAT_DESC_SIZE 0x0BU
#define VC_HEADER_SIZE                                                                          \
	(sizeof(USBD_VIDEOCSVCIfDescTypeDef) + sizeof(USBD_VIDEOInputTerminalDescTypeDef) +      \
	 sizeof(USBD_VIDEOOutputTerminalDescTypeDef))
#define VS_HEADER_SIZE                                                                          \
	(sizeof(USBD_VIDEOVSHeaderDescTypeDef) + sizeof(USBD_VIDEOPayloadFormatDescTypeDef) +    \
	 sizeof(USBD_VIDEOFrameDescTypeDef))

#ifndef WBVAL
#define WBVAL(x) ((x)&0xFFU), (((x) >> 8) & 0xFFU)
#endif
#ifndef DBVAL
#define DBVAL(x) ((x)&0xFFU), (((x) >> 8) & 0xFFU), (((x) >> 16) & 0xFFU), (((x) >> 24) & 0xFFU)
#endif

#ifndef USBD_CONFIG_STR_DESC_IDX
#define USBD_CONFIG_STR_DESC_IDX 0U
#endif

#ifndef USBD_CONFIG_BMATTRIBUTES
#define USBD_CONFIG_BMATTRIBUTES 0xC0U
#endif

#define __USBD_FRAMEWORK_SET_EP(epadd, eptype, epsize, HSinterval, FSinterval)                  \
	do                                                                                        \
	{                                                                                         \
		pEpDesc			  = ((USBD_EpDescTypedef *)((uint32_t)pConf + *Sze));    \
		pEpDesc->bLength	  = (uint8_t)sizeof(USBD_EpDescTypedef);                  \
		pEpDesc->bDescriptorType  = USB_DESC_TYPE_ENDPOINT;                               \
		pEpDesc->bEndpointAddress = (epadd);                                              \
		pEpDesc->bmAttributes	  = (eptype);                                              \
		pEpDesc->wMaxPacketSize	  = (epsize);                                              \
		if (pdev->Speed == USBD_HIGH_SPEED)                                               \
		{                                                                                  \
			pEpDesc->bInterval = (HSinterval);                                        \
		}                                                                                  \
		else                                                                               \
		{                                                                                  \
			pEpDesc->bInterval = (FSinterval);                                        \
		}                                                                                  \
		*Sze += (uint32_t)sizeof(USBD_EpDescTypedef);                                     \
	} while (0)

#define __USBD_FRAMEWORK_SET_IF(ifnum, alt, eps, class, subclass, protocol, istring)             \
	do                                                                                        \
	{                                                                                         \
		pIfDesc			   = ((USBD_IfDescTypedef *)((uint32_t)pConf + *Sze));    \
		pIfDesc->bLength	   = (uint8_t)sizeof(USBD_IfDescTypedef);                 \
		pIfDesc->bDescriptorType   = USB_DESC_TYPE_INTERFACE;                             \
		pIfDesc->bInterfaceNumber  = (ifnum);                                             \
		pIfDesc->bAlternateSetting = (alt);                                               \
		pIfDesc->bNumEndpoints	   = (eps);                                                \
		pIfDesc->bInterfaceClass   = (class);                                             \
		pIfDesc->bInterfaceSubClass = (subclass);                                         \
		pIfDesc->bInterfaceProtocol = (protocol);                                         \
		pIfDesc->iInterface	    = (istring);                                           \
		*Sze += (uint32_t)sizeof(USBD_IfDescTypedef);                                     \
	} while (0)

#ifdef __cplusplus
}
#endif
#endif /* __UX_DEVICE_DESCRIPTORS_H__ */
