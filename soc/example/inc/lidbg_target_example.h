#ifndef  __TARGET_MT3360__DEFINE_
#define __TARGET_MT3360__DEFINE_

#ifdef PLATFORM_mt3360
#define UART_SEND_DATA_TO_LPC
#define UART_SEND_DATA "/system/bin/lidbg_uart_send_data"
#define UART_BAUD_REATE "115200"
#define LPC_UART_PORT "/dev/ttyMT0"
#define SEND_DATA_WITH_UART
#endif

#define  SOC_IO_SUSPEND
#define  SOC_IO_RESUME

#define  MCU_WP_GPIO_ON  do{check_gpio(g_hw.gpio_mcu_wp);SOC_IO_Output(0, g_hw.gpio_mcu_wp, 1);lidbg("===== gpio wp = 1\n");}while(0)
#define  MCU_WP_GPIO_OFF  do{check_gpio(g_hw.gpio_mcu_wp);SOC_IO_Output(0, g_hw.gpio_mcu_wp, 0);lidbg("===== gpio wp = 0\n");}while(0)

#define  MCU_APP_GPIO_ON  do{check_gpio(g_hw.gpio_mcu_app);SOC_IO_Output(0, g_hw.gpio_mcu_app, 1);lidbg("===== gpio app = 1\n");}while(0)
#define  MCU_APP_GPIO_OFF  do{check_gpio(g_hw.gpio_mcu_app);SOC_IO_Output(0, g_hw.gpio_mcu_app, 0);lidbg("===== gpio app = 0\n");}while(0)


//lcd

#define LCD_RESET do{\
			check_gpio(g_hw.gpio_lcd_reset);\
			SOC_IO_Output(0, g_hw.gpio_lcd_reset, 0);\
			msleep(20);\
			SOC_IO_Output(0, g_hw.gpio_lcd_reset, 1);\
}while(0)\
 
//usb
#define USB_SWITCH_CONNECT  do{\
			check_gpio(g_hw.gpio_usb_switch);\
			SOC_IO_Output(0, g_hw.gpio_usb_switch, 0);\
	}while(0)


#define USB_SWITCH_DISCONNECT do{\
			check_gpio(g_hw.gpio_usb_switch);\
			SOC_IO_Output(0, g_hw.gpio_usb_switch, 1);\
	}while(0)


#define USB_POWER_ENABLE do{\
		LPC_CMD_USB5V_ON;\
		if(g_hw.gpio_usb_power == -1 )\
			break ;\
			if(g_var.hw_info.hw_version == 1)\
				SOC_IO_Output(0, g_hw.gpio_usb_power, 0);\
			else\
				SOC_IO_Output(0, g_hw.gpio_usb_power, 1);\
	}while(0)


#define USB_POWER_DISABLE do{\
			LPC_CMD_USB5V_OFF;\
			check_gpio(g_hw.gpio_usb_power);\
			if(g_var.hw_info.hw_version == 1)\
				SOC_IO_Output(0, g_hw.gpio_usb_power, 1);\
			else\
				SOC_IO_Output(0, g_hw.gpio_usb_power, 0);\
	}while(0)



#define USB_ID_LOW_HOST do{\
			check_gpio(g_hw.gpio_usb_id);\
			SOC_IO_Output(0, g_hw.gpio_usb_id, 0);\
	}while(0)

#define USB_ID_HIGH_DEV do{\
			check_gpio(g_hw.gpio_usb_id);\
			SOC_IO_Output(0, g_hw.gpio_usb_id, 1);\
	}while(0)

#define SET_USB_ID_SUSPEND do{\
				check_gpio(g_hw.gpio_usb_id);\
		}while(0)

#define USB_WORK_ENABLE do{\
				lidbg("USB_WORK_ENABLE\n");\
				USB_SWITCH_CONNECT;\
    			USB_POWER_ENABLE;\
    			USB_ID_LOW_HOST;\
			}while(0)
#define USB_WORK_DISENABLE  do{\
			lidbg("USB_WORK_DISENABLE\n");\
			USB_SWITCH_DISCONNECT;\
			USB_POWER_DISABLE;\
			USB_ID_HIGH_DEV;\
			}while(0)
//ad
#define AD_KEY_PORT_L   (35)//(g_hw.ap_key_left)
#define AD_KEY_PORT_R   (37)//(g_hw.ap_key_right)


#define FLY_GPS_SO  "gps.msm8226.so"


#endif
