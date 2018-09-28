#ifndef _CAMENUM_H
#define _CAMENUM_H

#define		MAX_CAM_NUM		8
#define		MAX_ID			64

typedef struct 
{
	int		vid;
	int		pid;
} ID_TABLE_S;

// cam enumerator
class CCamEnum
{
public:
	CCamEnum();
	~CCamEnum();
	
	bool	Set_IDCheckTable(ID_TABLE_S table[MAX_ID], int num);
	bool 	Enum_Cam(struct usb_device* CamArray[MAX_CAM_NUM], int &nCamNum);
	
private:
	struct usb_device* 	m_Bag[MAX_CAM_NUM];
	int					m_nBagNum;
	ID_TABLE_S 			m_IDTable[MAX_ID];
	int					m_nIDNum;

	bool	is_valid_id(struct usb_device *pDev);
	void 	bus_find_cam(struct usb_device *pDev, int level);
};

#endif // _CAMENUM_H
