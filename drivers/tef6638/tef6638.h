#ifndef __TEF6638_H__
#define __TEF6638_H__

#include <asm/uaccess.h>
#include <linux/file.h>
#include <linux/syscalls.h>
//#include <asm/system.h>

#include <linux/workqueue.h>

#if 1
#define BYTE u8
#define UINT u32
#define UINT32 u32
#define BOOL bool
#define ULONG u32

#define FALSE 0
#define TRUE 1
#endif

#define LPC_CMD_6638RST_L  do{    \
		u8 buff[] = {0x02, 0x0a, 0x00};\
        SOC_LPC_Send(buff, SIZE_OF_ARRAY(buff));\
				}while(0)

#define LPC_CMD_6638RST_H  do{    \
		u8 buff[] = {0x02, 0x0a, 0x01};\
        SOC_LPC_Send(buff, SIZE_OF_ARRAY(buff));\
				}while(0)

#define LPC_CMD_7388STANDBY_L  do{    \
		u8 buff[] = {0x02, 0x0b, 0x00};\
        SOC_LPC_Send(buff, SIZE_OF_ARRAY(buff));\
				}while(0)

#define LPC_CMD_7388STANDBY_H  do{    \
		u8 buff[] = {0x02, 0x0b, 0x01};\
        SOC_LPC_Send(buff, SIZE_OF_ARRAY(buff));\
				}while(0)

#define LPC_CMD_7388MUTE_L  do{    \
		u8 buff[] = {0x02, 0x0c, 0x00};\
        SOC_LPC_Send(buff, SIZE_OF_ARRAY(buff));\
				}while(0)

#define LPC_CMD_7388MUTE_H  do{    \
		u8 buff[] = {0x02, 0x0c, 0x01};\
        SOC_LPC_Send(buff, SIZE_OF_ARRAY(buff));\
				}while(0)

#define LPC_CMD_4052A2_L  do{    \
		u8 buff[] = {0x02, 0x0f, 0x00};\
        SOC_LPC_Send(buff, SIZE_OF_ARRAY(buff));\
				}while(0)

#define LPC_CMD_4052A2_H  do{    \
		u8 buff[] = {0x02, 0x0f, 0x01};\
        SOC_LPC_Send(buff, SIZE_OF_ARRAY(buff));\
				}while(0)


#define I2C_API_XFER_MODE_SEND 1
#define I2C_API_XFER_MODE_RECV 2
#define I2C_API_XFER_MODE_RECV_SUBADDR_2BYTES 3

#define 	TEF6638_GPIO_RST          (LPC)  //no use

#define 	TEF6638_I2C_ADDR	      0x63

#define     TEF6638_ADDR_W	      0xC6
#define     TEF6638_ADDR_R		      0xC7


enum audio_source {Init = 0, MediaCD, CDC, RADIO, AUX, IPOD, TV, MediaMP3, SRADIO, A2DP, EXT_TEL, GR_AUDIO, MediaSystem, BACK, GPS, BT_RING, BT, DVR, VAP, FRONT_CAMERA, SYSTEM_RING, AUDIO_CHANNEL_MAX};

#define efIsExboxChannel(iChn)			(TV == iChn || DVR == iChn)

#define BALANCE_LEVEL_COUNT		21

#define FADER_LEVEL_COUNT		21

#define BASS_FREQ_COUNT		    3
#define BASS_LEVEL_COUNT	    11

#define MID_FREQ_COUNT		    3
#define MID_LEVEL_COUNT		    11

#define M_FREQ_500              0
#define M_FREQ_1000             1
#define M_FREQ_1500             2

#define TREB_FREQ_COUNT		    2
#define TREB_LEVEL_COUNT	    11

#define T_FREQ_10K              0
#define T_FREQ_12K              1
#define T_FREQ_15K              2
#define T_FREQ_17K5             3

//primary & sec input subaddr(sec don't have SineGen)
#define RADIO_SUBADDR	0x00
#define AIN0_SUBADDR		0x08
#define AIN1_SUBADDR		0x09
#define AIN2_3_SUBADDR	0x0A
#define I2S0_SUBADDR		0x10
#define I2S1_SUBADDR		0x11
#define I2S2_SUBADDR		0x12
#define HOST_I2S0_SUBADDR	0x13
#define HOST_I2S1_SUBADDR	0x14
#define SPDIF_SUBADDR		0x15
#define NOISEGEN_P_SUBADDR	0x1E
#define NOISEGEN_S_SUBADDR	0x1F
#define SINEGEN_SUBADDR		0x1F

#define NOTUSED_CASE		254	//or -1(suppress warning)


#if (defined SOC_rk3x88) || (defined SOC_imx6q) || (defined SOC_mt35x)
#define INIT_INPUT_ADDR		I2S1_SUBADDR
#define IIS1_INPUT_CASE		case IPOD:\
							case TV:\
							case VAP:\
							case MediaMP3:\
							case MediaSystem:\
							case SYSTEM_RING:\
							case BT_RING:
#define AIN0_INPUT_CASE		case AUX:
#define AIN1_INPUT_CASE		case A2DP:\
							case BT:
#define AIN2_3_INPUT_CASE	case EXT_TEL:\
							case GR_AUDIO:
#define SPDIF0_INPUT_CASE	case MediaCD:
#define RADIO_INPUT_CASE		case RADIO:
#endif

#ifdef SOC_msm8x26
#define INIT_INPUT_ADDR		AIN0_SUBADDR
#define IIS1_INPUT_CASE		case NOTUSED_CASE://not used
#define AIN0_INPUT_CASE		case IPOD:\
							case TV:\
							case VAP:\
							case MediaMP3:\
							case MediaSystem:\
							case SYSTEM_RING:\
							case BT_RING:
#define AIN1_INPUT_CASE		case A2DP:\
							case BT:
#define AIN2_3_INPUT_CASE	case EXT_TEL:\
							case GR_AUDIO:\
							case AUX:
#define SPDIF0_INPUT_CASE	case MediaCD:
#define RADIO_INPUT_CASE		case RADIO:
#endif

#endif  //__TEF6638_H__

