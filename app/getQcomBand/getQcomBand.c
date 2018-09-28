
#include "lidbg_servicer.h"

#include <utils/Log.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#ifdef ANDROID
#include <cutils/properties.h>
#endif

#include "qmi.h"
#include "qmi_client.h"
#include "network_access_service_v01.h"
#define TAG "fuband:"

int getQcomBand(int para)
{
    qmi_client_error_type qmi_client_err;
    qmi_client_type my_qmi_client_handle;
    nas_get_rf_band_info_resp_msg_v01 resp_msg;
    qmi_idl_service_object_type nas_service;
    int i = 0;
    int band = -1;

    /*Initialize the qmi handle*/
    int qmi_handle = qmi_init(NULL, NULL);

    if( qmi_handle < 0 )
    {
        lidbg(TAG"qmi_init() failure: %d\n", qmi_handle);
        return -1;
    }

    nas_service = nas_get_service_object_v01();

    qmi_client_err = qmi_client_init(QMI_PORT_RMNET_0, nas_service, NULL, NULL, &my_qmi_client_handle);

    if(qmi_client_err != QMI_NO_ERR)
    {
        lidbg(TAG"Error: while Initializing qmi_client_init : %d\n", qmi_client_err);
        if(my_qmi_client_handle != NULL)
        {
            free(my_qmi_client_handle);
        }
        return -1;
    }

    qmi_client_err = qmi_client_send_msg_sync(my_qmi_client_handle,
                     QMI_NAS_GET_RF_BAND_INFO_RESP_MSG_V01, NULL, 0, &resp_msg, sizeof(resp_msg), 500/*TIMEOUT*/);

    if(qmi_client_err != QMI_NO_ERR)
    {
        lidbg(TAG"Error: while getting the QMI_NAS_GET_RF_BAND_INFO: %d\n", qmi_client_err);
        return -1;
    }

    //Got the band info
    if(resp_msg.rf_band_info_list_len > 1)
    {
        lidbg(TAG"Error: can't determine if more than one band is serving! Is this possible!?\n");
        return -1;
    }
    for ( i = 0; i < (int)resp_msg.rf_band_info_list_len; i++ )
    {
        if(para)
            lidbg(TAG"%d/%d: radio_if: %d  active_band: %d\n",
                  i, ((int)resp_msg.rf_band_info_list_len), resp_msg.rf_band_info_list[i].radio_if, resp_msg.rf_band_info_list[i].active_band);
        if(i == 0)
            band = resp_msg.rf_band_info_list[i].active_band;
    }

    //Cleauo QMI stuff
    qmi_client_err = qmi_client_release(my_qmi_client_handle);
    if(qmi_client_err != QMI_NO_ERR)
    {
        lidbg(TAG"Error: while releasing qmi_client : %d\n", qmi_client_err);
        //Ignore cleanup error
    }

    qmi_handle = qmi_release(qmi_handle);
    if( qmi_handle < 0 )
    {
        lidbg(TAG"Error: while releasing qmi %d\n", qmi_handle);
        //Ignore cleanup errors
    }
    return band;
}
static int band_old = -1 ;
static int loop = 0;
int main( void )
{
    while(1)
    {
        int band = getQcomBand(loop % 10 == 0);
        loop++;
        if(band != band_old && band != -1)
        {
            char cmd[64];
            band_old = band;
            lidbg(TAG"current band: %d\n", band);
            sprintf(cmd, "ws toast current_band:%d 1 ", band);
            LIDBG_WRITE("/dev/lidbg_pm0", cmd);
        }
        sleep (1);
    }
    return 0;
}

