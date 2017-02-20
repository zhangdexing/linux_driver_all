UINT32 KeyParser_VideoRecEvent(UINT32 ulMsgId, UINT32 ulEvent, UINT32 ulParam )
{
    switch( ulEvent )
    {
        //Device
        case BUTTON_USB_DETECTED   :     return EVENT_USB_DETECT;
        case BUTTON_USB_REMOVED    :     return EVENT_USB_REMOVED;
        case BUTTON_USB_B_DEVICE_DETECTED   : return EVENT_USB_B_DEVICE_DETECT;
        case BUTTON_USB_B_DEVICE_REMOVED    : return EVENT_USB_B_DEVICE_REMOVED;

        case BUTTON_SD_DETECTED    :     return EVENT_SD_DETECT;
        case BUTTON_SD_REMOVED     :     return EVENT_SD_REMOVED;
        
        case BUTTON_CUS_SW1_ON     :	 return EVENT_CUS_SW1_ON;
        case BUTTON_CUS_SW1_OFF    :	 return EVENT_CUS_SW1_OFF;

        #if (EMERGENCY_RECODE_FLOW_TYPE == EMERGENCY_RECODE_SWITCH_FILE)
        case BUTTON_LOCK_FILE_G    :     return EVENT_VR_EMERGENT;
        #else
        case BUTTON_LOCK_FILE_G    :     return EVENT_LOCK_FILE_G;
        #endif

        //Record Callback
        case BUTTON_VRCB_MEDIA_FULL:     return EVENT_VRCB_MEDIA_FULL;
        case BUTTON_VRCB_FILE_FULL :     return EVENT_VRCB_FILE_FULL;
        case BUTTON_VRCB_MEDIA_SLOW:     return EVENT_VRCB_MEDIA_SLOW;
        case BUTTON_VRCB_MEDIA_ERROR:    return EVENT_VRCB_MEDIA_ERROR;
        case BUTTON_VRCB_SEAM_LESS :     return EVENT_VRCB_SEAM_LESS;
        case BUTTON_VRCB_MOTION_DTC:     return EVENT_VRCB_MOTION_DTC;
        case BUTTON_VRCB_VR_START  :	 return EVENT_VR_START;
        case BUTTON_VRCB_VR_STOP   :	 return EVENT_VR_STOP;
        case BUTTON_VRCB_VR_POSTPROC:    return EVENT_VR_WRITEINFO;
        case BUTTON_VRCB_EMER_DONE	:	 return EVENT_VRCB_EMER_DONE;
        case BUTTON_CLEAR_WMSG     :     return EVENT_CLEAR_WMSG;
        case BUTTON_VRCB_RECDSTOP_CARDSLOW:     return EVENT_VRCB_RECDSTOP_CARDSLOW;

        default                    :     return EVENT_NONE;
    }
}

