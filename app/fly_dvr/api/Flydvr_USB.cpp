#include "Flydvr_USB.h"

static FLY_BOOL isFrontCameraConnect;
static FLY_BOOL isRearCameraConnect;

/**
 @brief Check the connection of USB device

 This function
 Parameters:
 @param[in]: void
 @retval AHC_TRUE:Connect AHC_FALSE: NO connection.
*/
FLY_BOOL Flydvr_IsFrontCameraConnect(void)
{
    return isFrontCameraConnect;
}

void Flydvr_SetFrontCameraConnect(FLY_BOOL isConnect)
{
    isFrontCameraConnect = isConnect;
}


/**
 @brief Check the connection of USB device

 This function
 Parameters:
 @param[in]: void
 @retval AHC_TRUE:Connect AHC_FALSE: NO connection.
*/
FLY_BOOL Flydvr_IsRearCameraConnect(void)
{
    return isRearCameraConnect;
}

void Flydvr_SetRearCameraConnect(FLY_BOOL isConnect)
{
    isRearCameraConnect = isConnect;
}


