#ifndef _FLYDVR_PARAMETER_H_
#define _FLYDVR_PARAMETER_H_

#define FLYDVR_STRING_MAX_LENGTH 		32
#define FLYDVR_USER_PROP	"persist.flydvr.user."
#define FLYDVR_FACTORY_PROP	"persist.flydvr.factory."

typedef struct _FLY_PARAM {
	
	//Group: SYSTEM

	UINT16 CardDetectPolarity;	//The polarity for card detection pin.
	UINT16 I2cWaitTimeout;		//The wait time for getting I2C response. 0: no wait, 1~0xFFFF.
	
	UINT16 FlipLR;				//Enable left/right flipping in both preview and capture modes. If enable, the captured image will be flipped.
	UINT16 FlipUD;				//Enable up/down flipping in both preview and capture modes. If enable, the captured image will be flipped.
	UINT16 SensorMaxVideoWidth;	//Get the maximum width in sensor preview setting.
	UINT16 SensorMaxVideoHeight;
	UINT16 SensorMaxStillWidth;	//Get the maximum width in sensor still capture setting.
	UINT16 SensorMaxStillHeight;//Get the maximum height in sensor still capture setting.
	UINT16 NightModeAE;			//Perform the night mode exposure in next captured frame.
	
	UINT16 CaptureMemoryCheck;	//Enable for checking if there has sufficient memory for next shot, if not sufficient, then the transition from capture 
           						//to view is delayed to for ensuring the capture completed.
	UINT32 FreeMemoryCheck;		//Get the size of available buffer. User can know how many free bytes can be allocated.
	
	UINT16 IgnoreAhlMessage;	//Set to 1 to ignore all AHL messages, instead of keeping the messages, those messages will be dropped.
	
	UINT16 AhlMsgUnblock;		//Unblock the blocked AHL messages. The input of this parameter is an AHL message ID and be blocked previously.

	UINT16 AhlMsgBlock;			//Block the assigned AHL message upon the input ID.
	
	UINT16 DebugPrintEnable;	//Enable/disable the debug Prints.
	
	UINT8  SDInfo[24];			//If SD card inserted and mounted, returns some information.
	
	FLY_BOOL DetectFlow;			//DetectFlow enable/disable while boots up.
	

	//Group: UI
	
	UINT16 OsdWidth;			//Get the Osd width in pixels (current layer).
	UINT16 OsdHeight;			//Get the Osd height in pixels (current layer).

	//Group: File & STORAGE
	UINT16 MountFlags;			//Configures the flags for media mount.
								//0- No flag
								//1- Remove all empty DCF objects (size=0) during mount.
								//2- Filter DCF files by date. System will only mount files from the date specified by FilterDcfFilesByDate.
								//3- Remove all files with the same file number during mount regardless file type.

	UINT16 SDCardType;			//This value is grabbed and stored during the SD/MMC mount. AHL can get current card type by this parameter. 0- MMC, 1- SD


	UINT16 MediaFatType;		//Set the desired FAT type before mount or read out current mounted FAT type.  FAT12/FAT16/FAT32
	UINT32 CallbackBadBlockEnable;	//If enabled, system will process the assigned callback function to let AHL know that there are too many bad blocks on current media.

	UINT32 CallbackUserDataErr;	//If enabled, system will process the assigned callback function to let AHL know that system discovered an error in user data.

	UINT32 CallbackSystemDataErr;//If enabled, system will process the assigned callback function to let AHL know that system discovered an error in system data. Such like root entry or FAT.

	UINT32 CallbackAccessTimeout;//If enabled, system will process the assigned callback function to let AHL handle the timeout during media access.

	
	//Group: Usb
	UINT16 UsbDescVid;			//Set the vendor ID for Usb device descriptor. Set to zero if wants to use the configuration inside resource file.

	UINT16 UsbDescPid;			//Set the product ID for Usb device descriptor. Set to zero if wants to use the configuration inside resource file.

	UINT16 UsbUfiCustomProductIDSel;		//Select the source of UFI custom ID. 0- From resource file.
											//1- From the value in UfiInquiryCustomProductID

	UINT8  UsbUfiInquiryCustomProductID[16];	//Set the product ID for Usb UFI INQUIRY operation
	UINT16 UsbUfiCustomVendorIDSel;			//Select the source of UFI custom vendor ID.
											//0 - From resource file.  1 - From the value in UfiInquiryCustomVendorID

	UINT8  UsbUfiInquiryCustomVendorID[16];	//Set the vendor ID for Usb UFI INQUIRY operation
	UINT16 UsbUfiCustomRevIDSel;			//Select the source of UFI custom revision ID.
											//0-From resource file.
											//1-From the value in UfiInquiryCustomRevID

	UINT8  UsbUfiInquiryCustomRevID[16];	//Set the revision ID for Usb UFI INQUIRY operation
	UINT16 UsbSerialDescriptorSel;			//Select the source of Usb serial descriptor string.
	UINT8  UsbSerialDescriptorString[FLYDVR_STRING_MAX_LENGTH];	//Set the Usb serial descriptor string.
	UINT8  UsbManufactoryDescriptorString[FLYDVR_STRING_MAX_LENGTH];	//Set the Usb manufactory descriptor string.
	UINT16 UsbManufactoryDescStringSel;		//Select the source of Usb manufactory descriptor string.
	UINT16 UsbProductDescStringSel;			//Select the source of Usb product descriptor string.
	UINT8  UsbProductDescriptorString[FLYDVR_STRING_MAX_LENGTH];	//Set the Usb product descriptor string.
	UINT16 UsbDescDevReleaseNum;			//Set the 'bcdDevice' value in the Usb standard device descriptor.

	UINT16 DpofCurrentStatus;				//Get the status (error code) of DPOF operation.
	UINT16 DpofExpectFileSize;				//Set the expected DPOF file size in KB
	UINT16 AudioOverUsb;					//Set the audio supported in Usb. 0- Not support
	UINT16 RegisterMSWriteCallback;			//Set a callback function to be triggered during data writing under Usb mass-storage mode.

	UINT16 UsbDeviceSpeed;		// Set or get the speed of Usb device. To read, Usb must be disconnected.
								//0- Usb FULL Speed
								//1- Usb HIGH Speed
										
	UINT16 UsbMsDetectReQ;		//Enable/disable detetion of device connected to Usb, if Devices connected to GPIO then a internal detection must be done.

	//Group: Sequential Still Capture
	UINT16 SeqCaptureProfile; 	//Decides the profiled sequence for capture.
	UINT16 SeqCaptureImages; 	//Number of images is going to be captured.

	//Group: JPEG
	UINT32 JpegDecodeWidth;
	UINT32 JpegDecodeHeight;
	
} FLY_PARAM;


typedef enum _FLYDVR_PARAM_ID {
	PARAM_ID_NONE = 0x0000,

	PARAM_ID_CARD_DETECT_POLARITY,
	PARAM_ID_FLIP_LR,
	PARAM_ID_FLIP_UD,
	PARAM_ID_SENSOR_MAX_VIDEO_WIDTH ,
	PARAM_ID_SENSOR_MAX_VIDEO_HEIGHT,
	PARAM_ID_SENSOR_MAX_STILL_WIDTH,
	PARAM_ID_SENSOR_MAX_STILL_HEIGHT,
	PARAM_ID_NIGHT_MODE_AE,
	PARAM_ID_CAPTURE_MEMORY_CHECK,
	PARAM_ID_FREE_MEMORY_CHECK,
	
	PARAM_ID_MAX = 0xFFFF
} FLYDVR_PARAM_ID;

typedef enum _FLYDVR_ACCESS_PROP{
	FLY_DEFAULT_USER = 0x01,
	FLY_DEFAULT_FACTORY	= 0x02,
}FLYDVR_ACCESS_PROP;

FLY_BOOL Flydvr_PARAM_Initialize(void);
FLY_BOOL Flydvr_PARAM_Menu_Write(FLYDVR_ACCESS_PROP accessProp);
FLY_BOOL Flydvr_PARAM_Menu_Read(UINT8* readback, FLYDVR_ACCESS_PROP accessProp);

#endif		//_FLYDVR_PARAMETER_H_