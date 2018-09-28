#ifndef __TARGET_IMX6Q__DEFINE_
#define __TARGET_IMX6Q__DEFINE_

//#ifdef PLATFORM_mt3360
/*#define UART_SEND_DATA_TO_LPC
#define UART_SEND_DATA "/system/bin/lidbg_uart_send_data"
#define UART_BAUD_REATE "115200"
#define LPC_UART_PORT "/dev/ttyMT0"
#define SEND_DATA_WITH_UART*/
//add for build erro
#define WAKEUP_MCU_BEGIN  do{	}while(0)
#define WAKEUP_MCU_END  do{   }while(0)
#define USB_POWER_FRONT_DISABLE do{	}while(0)
#define USB_POWER_FRONT_ENABLE do{}while(0)
#define USB_VBUS_POWER_DISABLE do{}while(0)
#define USB_VBUS_POWER_ENABLE do{}while(0)
#define USB_POWER_BACK_DISABLE do{}while(0)
#define USB_POWER_BACK_ENABLE do{}while(0)
#define LEVEL_CONVERSION_ENABLE do{}while(0)
#define USB_BACK_WORK_ENABLE do{}while(0)
#define USB_POWER_UDISK_ENABLE do{}while(0)

#define  SOC_IO_SUSPEND  do{soc_io_suspend();}while(0)
#define  SOC_IO_RESUME	 do{soc_io_resume();}while(0)

#define  MCU_WP_GPIO_ON  do{check_gpio(g_hw.gpio_mcu_wp);SOC_IO_Output(0, g_hw.gpio_mcu_wp, 0);lidbg("===== gpio wp = 0\n");}while(0)
#define  MCU_WP_GPIO_OFF  do{check_gpio(g_hw.gpio_mcu_wp);SOC_IO_Output(0, g_hw.gpio_mcu_wp, 1);lidbg("===== gpio wp = 1\n");}while(0)
#define  MCU_SET_WP_GPIO_SUSPEND  do{check_gpio(g_hw.gpio_mcu_wp);SOC_IO_Suspend_Config(g_hw.gpio_mcu_wp,GPIOMUX_OUT_HIGH,GPIO_CFG_NO_PULL,GPIOMUX_DRV_2MA);}while(0)


#define  MCU_APP_GPIO_ON  do{check_gpio(g_hw.gpio_mcu_app);SOC_IO_Output(0, g_hw.gpio_mcu_app, 0);lidbg("===== gpio app = 0\n");}while(0)
#define  MCU_APP_GPIO_OFF  do{check_gpio(g_hw.gpio_mcu_app);SOC_IO_Output(0, g_hw.gpio_mcu_app, 1);lidbg("===== gpio app = 1\n");}while(0)

#define  GPS_POWER_ON  do{check_gpio(g_hw.gpio_gps_rst);SOC_IO_Output(0, g_hw.gpio_gps_rst, 1);}while(0)
#define  GPS_POWER_OFF
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
		g_var.usb_status = 1;\
		if(g_hw.gpio_usb_power == -1 )\
			break ;\
			if(g_var.hw_info.hw_version == 1)\
				SOC_IO_Output(0, g_hw.gpio_usb_power, 0);\
			else\
				SOC_IO_Output(0, g_hw.gpio_usb_power, 1);\
	}while(0)


#define USB_POWER_DISABLE do{\
			LPC_CMD_USB5V_OFF;\
			g_var.usb_status = 0;\
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
#define USB_FRONT_WORK_ENABLE  do{\
}while(0)
//ad
#define AD_KEY_PORT_L   (35)//(g_hw.ap_key_left)
#define AD_KEY_PORT_R   (37)//(g_hw.ap_key_right)


#define MSM_DSI83_DISABLE do{\
	}while(0)


#define MSM_ACCEL_POWER_ON do{\
}while(0)
 
#define MSM_ACCEL_POWER_OFF do{\
}while(0)
#define FLY_GPS_SO  "gps.sabresd_6dq.so"

#define DEVICE3_3_POWER_ON
#define DEVICE3_3_POWER_OFF
#endif
