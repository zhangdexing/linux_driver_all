#ifndef  __TARGET_MT35X__DEFINE_
#define __TARGET_MT35X___DEFINE_

#define  SOC_IO_SUSPEND  do{soc_io_suspend();}while(0)
#define  SOC_IO_RESUME  do{soc_io_resume();}while(0)

#define  MCU_WP_GPIO_ON  do{check_gpio(g_hw.gpio_mcu_wp);SOC_IO_Output(0, g_hw.gpio_mcu_wp, 0);lidbg("MCU_WP_GPIO_0\n");}while(0)
#define  MCU_WP_GPIO_OFF  do{check_gpio(g_hw.gpio_mcu_wp);SOC_IO_Output(0, g_hw.gpio_mcu_wp, 1);lidbg("MCU_WP_GPIO_1\n");}while(0)
#define  MCU_SET_WP_GPIO_SUSPEND  do{check_gpio(g_hw.gpio_mcu_wp);SOC_IO_Suspend_Config(g_hw.gpio_mcu_wp,GPIOMUX_OUT_HIGH,GPIO_CFG_NO_PULL,GPIOMUX_DRV_2MA);}while(0)

#ifdef SUSPEND_ONLINE
#define  MCU_APP_GPIO_ON
#define  MCU_APP_GPIO_OFF
#define  MCU_SET_APP_GPIO_SUSPEND
#else
#define  MCU_APP_GPIO_ON  do{check_gpio(g_hw.gpio_mcu_app);SOC_IO_Output(0, g_hw.gpio_mcu_app, 0);lidbg("MCU_APP_GPIO_0\n");}while(0)
#define  MCU_APP_GPIO_OFF  do{check_gpio(g_hw.gpio_mcu_app);SOC_IO_Output(0, g_hw.gpio_mcu_app, 1);lidbg("MCU_APP_GPIO_1\n");}while(0)
#define  MCU_SET_APP_GPIO_SUSPEND  do{check_gpio(g_hw.gpio_mcu_app);SOC_IO_Suspend_Config(g_hw.gpio_mcu_app,GPIOMUX_OUT_LOW,GPIO_CFG_NO_PULL,GPIOMUX_DRV_2MA);}while(0)
#endif
//gps

#ifdef BOARD_V1
#define GPS_POWER_ON  do{SOC_IO_Output(0, g_hw.gpio_gps_lna_en, 1);SOC_IO_Output(0, g_hw.gpio_gps_ant_power, 1);}while(0)
#define GPS_POWER_OFF do{SOC_IO_Output(0, g_hw.gpio_gps_lna_en, 0);SOC_IO_Output(0, g_hw.gpio_gps_ant_power, 0);}while(0)

#define DEVICE3_3_POWER_ON do{}while(0)
#define DEVICE3_3_POWER_OFF do{}while(0)
#else
#define GPS_POWER_ON  do{SOC_IO_Output(0, g_hw.gpio_gps_lna_en, 1);}while(0)
#define GPS_POWER_OFF do{SOC_IO_Output(0, g_hw.gpio_gps_lna_en, 0);}while(0)

#define DEVICE3_3_POWER_ON    do{SOC_IO_Output(0, g_hw.gpio_gps_ant_power, 1);}while(0)
#define DEVICE3_3_POWER_OFF  do{SOC_IO_Output(0, g_hw.gpio_gps_ant_power, 0);}while(0)

#endif
#define MSM_DSI83_POWER_ON do{}while(0)
#define MSM_DSI83_POWER_OFF do{}while(0)
#define MSM_ACCEL_POWER_ON do{}while(0)
#define MSM_ACCEL_POWER_OFF do{}while(0)



//lcd
#define LCD_RESET do{\
			check_gpio(g_hw.gpio_lcd_reset);\
			SOC_IO_Output(0, g_hw.gpio_lcd_reset, 0);\
			msleep(20);\
			SOC_IO_Output(0, g_hw.gpio_lcd_reset, 1);\
}while(0)
 

//t123
#define T123_RESET do{\
		check_gpio(g_hw.gpio_t123_reset);\
		SOC_IO_Output(0, g_hw.gpio_t123_reset, 0);\
		msleep(300);\
		SOC_IO_Output(0, g_hw.gpio_t123_reset, 1);\
		msleep(20);\
	}while(0)


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
			check_gpio(g_hw.gpio_usb_power);\
			SOC_IO_Output(0, g_hw.gpio_usb_power, 1);\
	}while(0)


#define USB_VBUS_POWER_ENABLE do{\
			check_gpio(g_hw.gpio_usb_vbus_en);\
			SOC_IO_Output(0, g_hw.gpio_usb_vbus_en, 1);\
	}while(0)

#define USB_VBUS_POWER_DISABLE do{\
			check_gpio(g_hw.gpio_usb_vbus_en);\
			SOC_IO_Output(0, g_hw.gpio_usb_vbus_en, 0);\
	}while(0)

#define USB_POWER_DISABLE do{\
			LPC_CMD_USB5V_OFF;\
			g_var.usb_status = 0;\
			check_gpio(g_hw.gpio_usb_power);\
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
				SOC_IO_Suspend_Config(g_hw.gpio_usb_id,GPIOMUX_OUT_HIGH,GPIO_CFG_NO_PULL,GPIOMUX_DRV_2MA);\
		}while(0)

#define USB_POWER_FRONT_ENABLE do{\
			lidbg("gpio_usb_front_en\n");\
			check_gpio(g_hw.gpio_usb_front_en);\
			SOC_IO_Output(0, g_hw.gpio_usb_front_en, 1);\
	}while(0)

#define USB_POWER_FRONT_DISABLE do{\
			check_gpio(g_hw.gpio_usb_front_en);\
			SOC_IO_Output(0, g_hw.gpio_usb_front_en, 0);\
	}while(0)

#define USB_POWER_BACK_ENABLE do{\
			lidbg("gpio_usb_backcam_en\n");\
			check_gpio(g_hw.gpio_usb_backcam_en);\
			SOC_IO_Output(0, g_hw.gpio_usb_backcam_en, 1);\
	}while(0)

#define USB_POWER_BACK_DISABLE do{\
			check_gpio(g_hw.gpio_usb_backcam_en);\
			SOC_IO_Output(0, g_hw.gpio_usb_backcam_en, 0);\
	}while(0)

#define USB_POWER_UDISK_ENABLE do{\
			lidbg("gpio_usb_udisk_en\n");\
			check_gpio(g_hw.gpio_usb_udisk_en);\
			SOC_IO_Output(0, g_hw.gpio_usb_udisk_en, 1);\
	}while(0)

#define USB_POWER_UDISK_DISABLE do{\
			check_gpio(g_hw.gpio_usb_udisk_en);\
			SOC_IO_Output(0, g_hw.gpio_usb_udisk_en, 0);\
	}while(0)

#define USB_WORK_ENABLE do{\
			 lidbg("USB_WORK_ENABLE\n");\
			 USB_ID_LOW_HOST;\
			 USB_POWER_ENABLE;\
			 USB_POWER_BACK_ENABLE;\
			 USB_POWER_FRONT_ENABLE;\
			 msleep(1000);\
			 USB_POWER_UDISK_ENABLE;\
			}while(0)

#define USB_WORK_DISENABLE  do{\
			lidbg("USB_WORK_DISENABLE\n");\
			USB_POWER_DISABLE;\
			USB_POWER_UDISK_DISABLE;\
			USB_POWER_FRONT_DISABLE;\
			USB_POWER_BACK_DISABLE;\
			msleep(500);\
			USB_ID_HIGH_DEV;\
			}while(0)


#define USB_FRONT_WORK_ENABLE

#define MSM_DSI83_DISABLE

#define FLY_GPS_SO  "gps.mt3561.so"


#define WAKEUP_MCU_BEGIN  do{check_gpio(g_hw.gpio_mcu_i2c_wakeup);SOC_IO_Output(0, g_hw.gpio_mcu_i2c_wakeup, 1); }while(0)
#define WAKEUP_MCU_END  do{check_gpio(g_hw.gpio_mcu_i2c_wakeup);SOC_IO_Output(0, g_hw.gpio_mcu_i2c_wakeup, 0); }while(0)

#define GPIO_IS_READY   do{check_gpio(g_hw.gpio_ready);SOC_IO_Output(0, g_hw.gpio_ready, 1); }while(0)
#define GPIO_NOT_READY   do{check_gpio(g_hw.gpio_ready);SOC_IO_Output(0, g_hw.gpio_ready, 0); }while(0)
#define  SET_GPIO_READY_SUSPEND  do{check_gpio(g_hw.gpio_ready);SOC_IO_Suspend_Config(g_hw.gpio_ready,GPIOMUX_OUT_LOW,GPIO_CFG_NO_PULL,GPIOMUX_DRV_2MA);}while(0)

#define HAL_IS_READY   do{check_gpio(g_hw.gpio_mcu_app);SOC_IO_Output(0, g_hw.gpio_mcu_app, 0); }while(0)
#define HAL_NOT_READY   do{check_gpio(g_hw.gpio_mcu_app);SOC_IO_Output(0, g_hw.gpio_mcu_app, 1); }while(0)
#define  SET_HAL_READY_SUSPEND  do{check_gpio(g_hw.gpio_mcu_app);SOC_IO_Suspend_Config(g_hw.gpio_mcu_app,GPIOMUX_OUT_HIGH,GPIO_CFG_NO_PULL,GPIOMUX_DRV_2MA);}while(0)

#endif
