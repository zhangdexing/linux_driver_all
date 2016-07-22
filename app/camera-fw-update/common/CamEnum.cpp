#include <stdio.h>
#include "usb.h"
#include "CamEnum.h"
#include "debug.h"
#include "..//BurnerApLib//BurnerApLib.h"	// shawn 2009/11/06 add
#include "../../inc/lidbg_servicer.h"
#include "../../inc/lidbg_flycam_app.h"

CCamEnum::CCamEnum()
{
	
}

CCamEnum::~CCamEnum()
{
	
}

bool CCamEnum::is_valid_id(struct usb_device *pDev)
{
	usb_dev_handle 	*udev;	
	bool	bRet;
	int		i;
	char	szRomVer[LEN_ROM_VER];
	
	bRet = false;
	udev = usb_open(pDev);
	if (udev == NULL)
	{
		DBG_Print("is_valid_id : usb_open() Fail!\n");
		goto exit;
	}

	if (!init_xfer(udev, MODE_VENDOR_COMMAND))
	{
		DBG_Print("is_valid_id : init_xfer() Fail!\n");
		goto exit;
	}
	
	if (!get_rom_version(szRomVer))
	{
		DBG_Print("is_valid_id : get_rom_version() - Fail!\n");
	}
	else
		bRet = true;
	
	uninit_xfer();

exit:	
	usb_close(udev);
	return bRet;
}

void CCamEnum::bus_find_cam(struct usb_device *pDev, int level)
{
	int	i;
	FILE*	fp = NULL;
	char devnum[10], hub_path[256];
	memset(hub_path, 0, sizeof(hub_path));
	sprintf(hub_path, "/sys/bus/usb/drivers/usb/%s/devnum", UDISK_NODE);
	fp = fopen(hub_path, "r");
	if(fp)
	{
		fread(devnum, sizeof(char), 10, fp);
		fclose(fp);
	}

	if(atoi(pDev->filename) == atoi(devnum))
		return;

	sprintf(hub_path, "/sys/bus/usb/drivers/usb/%s/devnum", BACK_NODE);
	fp = fopen(hub_path, "r");
	if(fp)
	{
		fread(devnum, sizeof(char), 10, fp);
		fclose(fp);
	}

	if(atoi(pDev->filename) == atoi(devnum))
		return;

	//LIDBG_PRINT("devnum = %d, filename = %d\n",atoi(devnum), atoi(pDev->filename));

	if (is_valid_id(pDev))
	{
		m_Bag[m_nBagNum] = pDev;
		m_nBagNum++;
	}
	
	for (i=0; i < pDev->num_children; i++)
	{
		bus_find_cam(pDev->children[i], level+1);
	}

	return;
}

bool CCamEnum::Set_IDCheckTable(ID_TABLE_S table[MAX_ID], int num)
{
	int		i;
	
	m_nIDNum = num;
	for (i=0; i<num; i++)
	{
		m_IDTable[i].vid = table[i].vid;
		m_IDTable[i].pid = table[i].pid;
	}
	return true;	
}

bool CCamEnum::Enum_Cam(struct usb_device* CamArray[MAX_CAM_NUM], int &nCamNum)
{
	struct usb_bus		*bus;
	struct usb_device 	*pDev;
	int  i;

	//sleep(2); // carol 2013/12/16 mark
	usb_init();
	usb_find_busses();
	usb_find_devices();
	
	m_nBagNum = 0;
	for (bus = usb_busses; bus; bus = bus->next) 
	{
		if (bus->root_dev)
		{
			bus_find_cam(bus->root_dev, 0);
		}
		else 
		{
			for (pDev = bus->devices; pDev; pDev = pDev->next)
			{
				bus_find_cam(pDev, 0);
			}
		}
	}

	for (i=0; i<m_nBagNum; i++)
		CamArray[i] = m_Bag[i];	
	nCamNum = m_nBagNum;

	return true;
}
