#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <assert.h>
#include "..//common//my_type.h"
#include "..//common//debug.h"
#include "..//common//misc.h"
#include "FW_File.h"
#include "..//BurnerApLib//BurnerApLib.h"	// shawn 2009/11/02 for 232 and 275

#define	ISP_BASE_ADDR	0x8800
#define	ISP_BASE_ADDR_232	0x4800
#define	ISP_BASE_ADDR_275	0x6000

// shawn 2009/11/03 for 232 and 275 +++++
extern BYTE m_abyNumOfFmt[SPEEDNUM];
extern BYTE byActualXUNum;
extern GUID aGUIDXU[XUNUM];
extern unsigned short awXUCTRL[XUNUM];
extern BYTE m_abyNumOfResFmtStill[SPEEDNUM][FMTNUM];
extern BYTE m_abyNumOfResFmtVideo[SPEEDNUM][FMTNUM];
extern BYTE m_abyFmtIdx[SPEEDNUM][FMTNUM];
extern BYTE m_abyNumOfFRFmtVideo[SPEEDNUM][FMTNUM][RESNUM];
extern BYTE m_abyStillCfgIdx[SPEEDNUM][FMTNUM][RESNUM];
extern ConfigData aCfgData[CFGNUM];
extern FRData m_aFRData_FmtVideo[SPEEDNUM][FMTNUM][RESNUM][FRNUM];
bool bIsNew276;				// shawn 2010/09/20 add
// shawn 2009/11/03 for 232 and 275 -----

int	_file_len(FILE *fp);
bool	isp_process(BYTE *szSrc, int nSrcLen, BYTE *pOutBuf, int nBufLen, int *pnOutLen, int nRomID);	// shawn 2009/11/03 modify for 232 and 275
bool 	isp_remove_comments(BYTE *srcBuf);
void	isp_init_tokenizer(BYTE *str);
bool	isp_get_next_token(BYTE szTok[32], int *pnLen);
bool	isp_get_next_token2(BYTE szTok[32], int *pnLen);	// shawn 2009/11/03 for 232 and 275
bool	isp_is_legal_num(BYTE szNum[32], int nLen);
bool	isp_is_dmt_char(BYTE ch);
BYTE	isp_to_octal(BYTE szNum[32], int nLen);


static int		g_idx;
static BYTE		*g_str;
static BYTE		g_dmt[4];

int	_file_len(FILE *fp)
{
	int	pos_old, len;
	
	assert(fp != NULL);
	
	pos_old = ftell(fp);
	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, pos_old, SEEK_SET);
	
	return len;
}

void	isp_init_tokenizer(BYTE *str)
{
	g_dmt[0] = ',';
	g_dmt[1] = ' ';
	g_dmt[2] = 0x0D;
	g_dmt[3] = 0x0A;
	g_idx = 0;
	g_str = str;
}

// assumption : the szNum is Legal
BYTE	isp_to_octal(BYTE szNum[32], int nLen)
{
	int		i;
	BYTE	n = 0;
	
	for (i=2; i<nLen; i++)
	{
		if (szNum[i] >= '0' && szNum[i] <= '9')
		{
			n = (n << 4) + (szNum[i] - '0');
		}
		else if (szNum[i] >= 'a' && szNum[i] <= 'f')
		{
			n = (n << 4) + (szNum[i] - 'a') + 10;
		}
		else if (szNum[i] >= 'A' && szNum[i] <= 'F')
		{
			n = (n << 4) + (szNum[i] - 'A') + 10;
		}
		else
		{
			DBG_Print("Error! - isp_to_octal : reach impossible program state!\n");
			assert(0);
		}
	}
	return n;
}

bool	isp_is_dmt_char(BYTE ch)
{
	if (ch == g_dmt[0] || 
		ch == g_dmt[1] ||
		ch == g_dmt[2] || 
		ch == g_dmt[3])
		return true;
	return false;
}

bool	isp_get_next_token2(BYTE szTok[32], int *pnLen)	// shawn 2009/11/03 recover for 232 and 275
{
	bool	bFound = false;
	int		n;
	int		nState;		// 0: idx is on Left of token
						// 1: idx is "in" the token
						// 2: idx is on Right of token
	
	n = 0;
	nState = 0;
	memset(szTok, 0x00, 32);
	while (g_str[g_idx] != '\0')
	{
		if (nState == 0)
		{
			if (g_str[g_idx] == '0' && g_str[g_idx+1] == 'x')
			{
				bFound = true;
				nState = 1;
				szTok[n++] = '0';
				szTok[n++] = 'x';
				g_idx += 2;
				continue;
			}
			else
			{
				g_idx++;
				continue;
			}
		}
		else if (nState == 1)
		{
			if (isp_is_dmt_char(g_str[g_idx]))
			{
				nState = 2;
				break;
			}
			else
			{
				szTok[n++] = g_str[g_idx++];
				continue;
			}
		}
		else
		{
			DBG_Print("Error! - isp_get_next_token : reach impossible program state #1!\n");
			return false;
		}
		DBG_Print("Error! - isp_get_next_token : reach impossible program state #2!\n");
		return false;
	}
	(*pnLen) = n;
	
	return bFound;
}

bool	isp_get_next_token(BYTE szTok[32], int *pnLen)
{
	bool	bFound = false;

	memset(szTok, 0x00, 32);
	while (g_str[g_idx] != '\0')
	{
		if (g_str[g_idx] == '0' && g_str[g_idx+1] == 'x')
		{
			szTok[0] = g_str[g_idx];
			szTok[1] = g_str[g_idx+1];
			szTok[2] = g_str[g_idx+2];
			szTok[3] = g_str[g_idx+3];
			g_idx += 4;
			bFound = true;
			break;
		}
		else
		{
			g_idx++;
			continue;
		}
		DBG_Print("isp_get_next_token : reach impossible program state! - Error!!\n");
		return false;
	}
	(*pnLen) = 4;
	
	return bFound;
}

bool	isp_is_legal_num(BYTE szNum[32], int nLen)
{
	int		i;
	
	if (nLen < 2)
		return false;
	
	for (i=2; i<nLen; i++)
	{
		if ((szNum[i] >= '0' && szNum[i] <= '9') || 
		    (szNum[i] >= 'a' && szNum[i] <= 'f') || 
		    (szNum[i] >= 'A' && szNum[i] <= 'F'))
		    continue;
		else
			return false;		
	}
	return true;
}

bool 	isp_remove_comments(BYTE *srcBuf)
{
	int i=0, j=0;

	for (i = 0; srcBuf[i]!='\0'; i++)
	{
		if (srcBuf[i] == ';' || srcBuf[i] == '£»' || (srcBuf[i] == '/' && srcBuf[i+1] == '/') || (srcBuf[i] == '#' && srcBuf[i+2] != 'X' && srcBuf[i+3] != 'U'))	// shawn 2009/11/03 modify for 232 and 275
		{
			for (;srcBuf[i]!='\n'; i++)
			{
				if (srcBuf[i]=='\0')
					break;
				srcBuf[i]=' ';
			}
		}
		else if(srcBuf[i] == '/' && srcBuf[i+1] == '*')
		{
			j=i+2;
			while ((srcBuf[j] != '*' || srcBuf[j+1] != '/') &&
					(srcBuf[j-1] != '*' || srcBuf[j] != '/') )
			{
				if (srcBuf[j] == '\0' || srcBuf[j+1] == '\0')
				{
					DBG_Print("Syntax error:miss */");
					return false;
				}
				j+=2;
			}

			for(;i<=j+1;i++)
			{
				srcBuf[i]=' ';
			}

			i--;
		}
		else if(srcBuf[i] == '*' && srcBuf[i+1] == '/')
		{
			DBG_Print("Syntax error:miss /*");
			return false;
		}
		else if(_memicmp(srcBuf+i, "# XU Information", 16) == 0)	// shawn 2009/11/03 for 232 and 275
		{
			for(;srcBuf[i]!='\0';i++)
				srcBuf[i]=' ';
		}
		else
			;//address or data

	}

	return true;
}

bool	isp_process(BYTE *szSrc, int nSrcLen, BYTE *pOutBuf, int nBufLen, int *pnOutLen, int nRomID)	// shawn 2009/11/03 modify for 232 and 275
{
	BYTE 	*str, *buf;
	BYTE	szNum[32];
	int		i, n, len, i_prv, sum;
	BYTE	nSec;
	int		aSecStart[64], aSecLen[64];
	bool	bRet = true;
	unsigned short val;
	str = (BYTE *)malloc(nSrcLen+64);
	assert(str != NULL);	
	buf = (BYTE *)malloc(nSrcLen+64);
	assert(buf != NULL);	

	memcpy(str, szSrc, nSrcLen);
	str[nSrcLen+1] = '\0';
	isp_remove_comments(str);
	
	// store & convert the string token into buf array
	n = 0;
	memset(buf, 0x00, nSrcLen+1);
	isp_init_tokenizer(str);
	while (isp_get_next_token(szNum, &len))
	{
		if (isp_is_legal_num(szNum, len))
		{
			buf[n++] = isp_to_octal(szNum, len);
		}
		else
		{
			bRet = false;
			DBG_Print("isp_process : isp_is_legal_num, szNum = %s, len = %d - Fail!\n",	szNum, len);			
			DBG_Print("'0' = 0x%.2x, '9' = 0x%.2x, 'a' = 0x%.2x, 'f' = 0x%.2x, 'z' = 0x%.2x\n",
					'0', '9', 'a', 'f', 'z');
			for (i=0; i<len; i++)
				DBG_Print("[ %d ] : 0x%.2x\n", i, szNum[i]);
			goto exit;
		}
	}
	
	// shawn 2009/11/03 for 232 adn 275 +++++
	if (nRomID == SD50232A_2005 || 
			nRomID == ST50275A_3005 || 
			nRomID == ST50276A_4006 )	// shawn 2010/07/13 add 276
	{
		if (!LoadDataAction(nRomID, 
												buf, 
												szSrc, 
												true, 
									      true,
												m_abyNumOfFmt,
												&byActualXUNum,
												aGUIDXU,
												awXUCTRL,
												m_abyNumOfResFmtStill,
												m_abyNumOfResFmtVideo,
												m_abyFmtIdx,
												m_abyNumOfFRFmtVideo,
												m_abyStillCfgIdx,
												aCfgData,
												m_aFRData_FmtVideo))
		{
			DBG_Print("CBurnMgr::Load_Source_Data : LoadDataAction() - Fail!\n");
			return false;
		}
	}
	// shawn 2009/11/03 for 232 adn 275 -----
		
	// checkpoint : buf
	// for (i=0; i<n; i++)		printf("0x%.2x\n", buf[i]);
		
	// count # of sections & mark section start in the buf & the length of section
	nSec = 0;
	memset(aSecStart, 0x00, sizeof(aSecStart));
	memset(aSecLen, 0x00, sizeof(aSecLen));
	i_prv = 0;
	i = 0;	
	while (i<=n-4)
	{
		// note : there MUST be 4 0xff in the end of file !
		if (buf[i] == 0xff && buf[i+1] == 0xff && buf[i+2] == 0xff && buf[i+3] == 0xff)
		{
			aSecStart[nSec]	= i_prv;
			aSecLen[nSec]	= i - i_prv;
			nSec++;
			i_prv = i+4;
			i+=4;			     
		}
		else
			i++;
	}
	// checkpoint : 
	// for (i=0; i<nSec; i++) printf("%d : %d - %d\n", i, aSecStart[i], aSecLen[i]);
	
	// store to suitable format to OutBuf
	sum = 0;
	len = 0;
	// [0] : amount of sections
	if (nBufLen < 1)	goto overflow;
	pOutBuf[len] = nSec; 	len++;
	
	for (i=0; i<nSec; i++)
	{
		// shawn 2009/11/03 for 232 and 275 +++++
		if (nRomID == SD50232A_2005)
			val = ISP_BASE_ADDR_232 + 1 + 2*nSec + 2*1 + sum;
		else if (nRomID == ST50275A_3005 || nRomID == ST50276A_4006)	// shawn 2010/07/13 add 276
			val = ISP_BASE_ADDR_275 + 1 + 2*nSec + 2*1 + sum;
		else
			val = ISP_BASE_ADDR + 1 + 2*nSec + 2*1 + sum;
		// shawn 2009/11/03 for 232 and 275 -----
		
		pOutBuf[len] 	= (BYTE)(val >> 8);		len++;
		if (nBufLen < len)	goto overflow;
		
		pOutBuf[len] 	= (BYTE)(val);			len++;
		if (nBufLen < len)	goto overflow;
		
		sum += aSecLen[i];
	}
	// save end address of the last section
	// shawn 2009/11/03 for 232 and 275 +++++
	if (nRomID == SD50232A_2005)
		val = ISP_BASE_ADDR_232 + 1 + 2*nSec + 2*1 + sum;
	else if (nRomID == ST50275A_3005 || nRomID == ST50276A_4006)	// shawn 2010/07/13 add 276
		val = ISP_BASE_ADDR_275 + 1 + 2*nSec + 2*1 + sum;
	else
		val = ISP_BASE_ADDR + 1 + 2*nSec + 2*1 + sum;
	
	pOutBuf[len] 	= (BYTE)(val >> 8);		len++;
	if (nBufLen < len)	goto overflow;
	
	pOutBuf[len] 	= (BYTE)(val);			len++;
	if (nBufLen < len)	goto overflow;
	
	// copy real data of each section
	for (i=0; i<nSec; i++)
	{
		if (nBufLen < len+aSecLen[i])	goto overflow;		
		memcpy(pOutBuf+len, buf+aSecStart[i], aSecLen[i]);
		len += aSecLen[i];
	}
	// checkpoint : pOutBuf
	// for (i=0; i<len; i++)	printf("0x%.2x\n", pOutBuf[i]);	
	(*pnOutLen) = len;
	goto exit;	

overflow:
	bRet = false;
	DBG_Print("isp_process : go into overflow - Error!\n");
	
exit:
	free(str);
	free(buf);

	return bRet;
}

int	 get_file_len(const char *szFile)
{
	FILE	*fp;
	int		len;

	fp = fopen(szFile, "rb");
	if (fp == NULL)
	{
		DBG_Print("get_file_len : fopen() - Fail!\n");
		return -1;
	}
	
	len = _file_len(fp);
	fclose(fp);
	return len;
}

bool read_file_rom(const char *szFile, unsigned long ulBypass, 
						BYTE *pBuf, int nBufLen, int *pnOutLen)
{
	FILE		*fp;
	int		len, n;

	(*pnOutLen) = 0;
	fp = fopen(szFile, "rb");
	if (fp == NULL)
	{
		DBG_Print("read_file_rom : fopen() Fail!\n");
		return false;
	}
	
	len = _file_len(fp) - ulBypass;
	if (len > nBufLen)
	{
		DBG_Print("read_file_rom : len (= %d) > nBufLen (=%d)\n", len, nBufLen);
		fclose(fp);
		return false;
	}
	
	memset(pBuf, 0xff, len);
	fseek(fp, ulBypass, SEEK_SET);
	n = fread(pBuf, 1, len, fp);
	if (n != len)
	{
		DBG_Print("read_file_rom : n != uiLength\n");
		fclose(fp);
		return false;
	}
	
	(*pnOutLen) = len;
	fclose(fp);
	return true;
}

bool read_file_des(const char *szFile, BYTE *pBuf, int nBufLen, int *pnOutLen)
{
	FILE	*fp;
	int		len, n;

	(*pnOutLen) = 0;
	fp = fopen(szFile, "rb");
	if (fp == NULL)
	{
		DBG_Print("read_file_des : fopen() Fail!\n");
		return false;
	}

	len = _file_len(fp);
	len = (nBufLen > len ? len : nBufLen);
		
	memset(pBuf, 0xff, len);
	fseek(fp, 0, SEEK_SET);
	n = fread(pBuf, 1, len, fp);
	if (n != len)
	{
		DBG_Print("read_file_des : n (=%d) != len (=%d)\n", n, len);
		fclose(fp);
		return false;
	}
	
	(*pnOutLen) = len;
	fclose(fp);
	return true;
}

bool read_file_param(const char *szFile, BYTE *pBuf, int nBufLen, int *pnOutLen)
{
	FILE	*fp;
	int		len, n;

	(*pnOutLen) = 0;
	fp = fopen(szFile, "rb");
	if (fp == NULL)
	{
		DBG_Print("read_file_param : fopen() Fail!\n");
		return false;
	}

	len = _file_len(fp);
	len = (nBufLen > len ? len : nBufLen);
		
	memset(pBuf, 0xff, len);
	fseek(fp, 0, SEEK_SET);
	n = fread(pBuf, 1, len, fp);
	if (n != len)
	{
		DBG_Print("read_file_param : n (=%d) != len (=%d)\n", n, len);
		fclose(fp);
		return false;
	}
	
	(*pnOutLen) = len;
	fclose(fp);
	return true;
}

bool read_file_ISP(const char *szFile, BYTE *pBuf, int nBufLen, int *pnOutLen, int nRomID)	// shawn 2009/11/03 modify for 232 and 275
{
	FILE	*fp;
	BYTE	*pTmpBuf;
	int		len, n;
	bool	bRet = true;
	
	fp = fopen(szFile, "r");
	if (fp == NULL)
	{
		DBG_Print("read_file_ISP : fopen() Fail!\n");
		return false;
	}
	
	len = _file_len(fp);
	pTmpBuf = (BYTE *)malloc(len);
	assert(pTmpBuf != NULL);
	
	// read all characters
	memset(pTmpBuf, 0xff, len);
	fseek(fp, 0, SEEK_SET);
	n = (int)fread(pTmpBuf, 1, len, fp);
	if (n != len)
	{
		DBG_Print("read_file_ISP : n (=%d) != len (=%d)\n", n, len);
		bRet = false;
		goto fail;
	}
	
	if (!isp_process(pTmpBuf, len, pBuf, nBufLen, pnOutLen, nRomID))	// shawn 2009/11/03 modify for 232 and 275
	{
		DBG_Print("read_file_ISP : isp_process() - Fail!\n", n, len);
		bRet = false;
	}
	
fail:	
	free(pTmpBuf);	
	fclose(fp);
	return bRet;	
}

// shawn 2009/08/14 for 233
bool read_file_64k(const char *szFile, BYTE *pBuf, int nBufLen)
{
	FILE	*fp;
	int		len, n;

	fp = fopen(szFile, "rb");
	if (fp == NULL)
	{
		DBG_Print("read_file_64k : fopen() Fail!\n");
		return false;
	}

	len = _file_len(fp);
	len = (nBufLen > len ? len : nBufLen);

	memset(pBuf, 0xff, len);
	fseek(fp, 0, SEEK_SET);
	n = fread(pBuf, 1, len, fp);
	if (n != len)
	{
		DBG_Print("read_file_64k : n (=%d) != len (=%d)\n", n, len);
		fclose(fp);
		return false;
	}
	
	fclose(fp);
	return true;
}

// shawn 2009/11/03 for 232 and 275
bool GetSectionPos(BYTE *pReadTmp, unsigned long dwStartPos, unsigned long &dwSectionPos, bool bDontParse)	// shawn 2010/09/13 modify
{
	int i = 0;
	int j = 0;
	int iTmpCnt = 0;
	// shawn 2009/12/08 +++++
	int iDataLen = 0;
	bool bIsData = false;
	bool bIsEnd = false;
	// shawn 2009/12/08 -----
	bool bIsL232TableData = false;	// shawn 2010/05/31 add

	if (pReadTmp == NULL)
	{
		dwSectionPos = 0;
		return true;
	}
	
	for (i = dwStartPos; pReadTmp[i] != '\0'; i++)
	{
		if (pReadTmp[i] == ';' || pReadTmp[i] == '£»'  || (pReadTmp[i] == '/' && pReadTmp[i+1] == '/') || pReadTmp[i] == '#')
		{
			for (; pReadTmp[i] != '\n'; i++)
			{
				if (pReadTmp[i] == '\0')
					break;
			}
		}
		else if (pReadTmp[i] == '/' && pReadTmp[i+1] == '*')
		{
			j=i+2;
			
			while ( (pReadTmp[j] != '*' || pReadTmp[j+1] != '/') && 
					(pReadTmp[j-1] != '*' || pReadTmp[j] != '/') )
			{
				if (pReadTmp[j] == '\0' || pReadTmp[j+1] == '\0')
				{
					DBG_Print("Syntax error:miss */");
					return false;
				}

				j+=2;
			}

			for (; i <= j+1; i++)
				;

			i--;
		}
		else if (pReadTmp[i] == '*' && pReadTmp[i+1] == '/')
		{
			DBG_Print("Syntax error:miss /*");
			return false;
		}
		// shawn 2009/12/08 modify +++++
		else if ( (pReadTmp[i]    == '0') && 
				  (pReadTmp[i+1]  == 'x') )
		{
			if (bDontParse)	// shawn 2010/09/13 modify
			{
				if ( ((pReadTmp[i+2] == 'f') || (pReadTmp[i+2] == 'F')) && 
				     ((pReadTmp[i+3] == 'f') || (pReadTmp[i+3] == 'F')) ) 
				{
					iTmpCnt++;
					i += 3;

					if (iTmpCnt == 4)
					{
						dwSectionPos = i;
						break;
					}
				}
				else
				{
					iTmpCnt = 0;
					i += 3;
				}
			}
			else
			{
				if (!bIsData)
				{
					// shawn 2010/05/31 modify +++++
					if ((pReadTmp[i+2] >= '0') && (pReadTmp[i+2] <= '1'))
					{
						if (bIsL232TableData)
							iDataLen = 1;
						else
							iDataLen = 4;
						
						iTmpCnt = 0;
					}
					else if ( ((pReadTmp[i+2] == 'a') || (pReadTmp[i+2] == 'A')) && 
							  ((pReadTmp[i+3] == 'a') || (pReadTmp[i+3] == 'A')) )
					{
						iDataLen = 1;
						iTmpCnt = 0;
					}
					else if ( ((pReadTmp[i+2] == 'b') || (pReadTmp[i+2] == 'B')) && 
							  ((pReadTmp[i+3] == 'b') || (pReadTmp[i+3] == 'B')) )
					{
						iDataLen = 3;
						iTmpCnt = 0;
					}
					else if ( ((pReadTmp[i+2] == 'f') || (pReadTmp[i+2] == 'F')) && 
							  ((pReadTmp[i+3] == 'f') || (pReadTmp[i+3] == 'F')) )
					{
						//iDataLen = 4;
						//bIsEnd = true;
						iDataLen = 1;
						iTmpCnt++;

						if (iTmpCnt == 4)
							bIsEnd = true;

						if (iTmpCnt == 1)
							bIsL232TableData = true;
						else
							bIsL232TableData = false;
					}
					else if (pReadTmp[i+2] == '9')
					{
						if ( (pReadTmp[i+3] >= '0') && (pReadTmp[i+3] <= '9') )
							iDataLen = ((int)(pReadTmp[i+3] - '0')) * 3 + 1;
						else if ( (pReadTmp[i+3] >= 'a') && (pReadTmp[i+3] <= 'f') )
							iDataLen = ((int)(pReadTmp[i+3] - 'a' + 10 )) * 3 + 1;
						else if ( (pReadTmp[i+3] >= 'A') && (pReadTmp[i+3] <= 'F') )
							iDataLen = ((int)(pReadTmp[i+3] - 'A' + 10)) * 3 + 1;

						iTmpCnt = 0;
					}
					else if (pReadTmp[i+2] == '8')
					{
						// shawn 2010/05/31 modify +++++
						if ( (pReadTmp[i+3] >= '0') && (pReadTmp[i+3] <= '9') )
							iDataLen = ((int)(pReadTmp[i+3] - '0'));
						else if ( (pReadTmp[i+3] >= 'a') && (pReadTmp[i+3] <= 'f') )
							iDataLen = ((int)(pReadTmp[i+3] - 'a' + 10 ));
						else if ( (pReadTmp[i+3] >= 'A') && (pReadTmp[i+3] <= 'F') )
							iDataLen = ((int)(pReadTmp[i+3] - 'A' + 10));

						iDataLen &= 0x7;
						iDataLen = iDataLen + 1;
						iTmpCnt = 0;
						// shawn 2010/05/31 modify -----
					}
					/*else if ((pReadTmp[i+2] == '3') && (pReadTmp[i+3] == '2'))	// Rom Code Version String	// shawn 2010/08/31 remove
					{
						iDataLen = 8;
						iTmpCnt = 0;
					}*/
					else if ((pReadTmp[i+2] == 'a' || pReadTmp[i+2] == 'A') && (pReadTmp[i+3] == '0'))	// shawn 2010/03/18 add
					{
						iDataLen = 4;
						iTmpCnt = 0;
					}
					else	// shawn 2010/05/31 for L232
					{
						iDataLen = 1;
						iTmpCnt = 0;
					}
					// shawn 2010/05/31 modify -----

					bIsData = true;
				}

				TRACE("iDataLen = %d", iDataLen);
				iDataLen--;
			
				if (iDataLen == 0)
				{
					bIsData = false;

					if (bIsEnd)
					{
						dwSectionPos = i + 3;
						break;
					}
				}
				
				//iTmpCnt = 0;
				i += 3;
			}
		}
		/*else if ( (pReadTmp[i]    == '0') && 
				  (pReadTmp[i+1]  == 'x') && 
				  ((pReadTmp[i+2] == 'f') || (pReadTmp[i+2] == 'F')) && 
				  ((pReadTmp[i+3] == 'f') || (pReadTmp[i+3] == 'F')) ) 
		{
			iTmpCnt++;
			i += 3;

			if (iTmpCnt == 4)
			{
				dwSectionPos = i;
				break;
			}
		}*/
		// shawn 2009/12/08 modify -----
	}

	return true;
}

// shawn 2009/11/02 for 232 and 275
bool LoadDataAction(int m_iRomVer, 
										BYTE *pReadTmp, 
										BYTE *pReadTmp2, 
										bool bIsFromUnproccessedFile, 
										bool bIsCheckProc,
										BYTE m_abyNumOfFmt[SPEEDNUM],
										BYTE *byActualXUNum,
										GUID aGUIDXU[XUNUM],
										unsigned short awXUCTRL[XUNUM],
										BYTE m_abyNumOfResFmtStill[SPEEDNUM][FMTNUM],
										BYTE m_abyNumOfResFmtVideo[SPEEDNUM][FMTNUM],
										BYTE m_abyFmtIdx[SPEEDNUM][FMTNUM],
										BYTE m_abyNumOfFRFmtVideo[SPEEDNUM][FMTNUM][RESNUM],
										BYTE m_abyStillCfgIdx[SPEEDNUM][FMTNUM][RESNUM],
										ConfigData aCfgData[CFGNUM],
										FRData m_aFRData_FmtVideo[SPEEDNUM][FMTNUM][RESNUM][FRNUM])
{
	int i = 0;
	int j = 0;
	int k = 0;
	int l = 0;
	int iTmp = 0;
	int iTmpCfgIdx = 0;
	int iTmpInitIdx = 0;
	int iTmpFRIdx = 0;
	int iTmpIQIdx = 0;			// shawn 2009/06/26 for 275
	int iTmpAreaCnt = 0;
	//int iResult = 0;			// shawn 2009/09/23 for XU
	int iTmpCnt = 0;			// shawn 2010/05/28 add
	short *pwTmpAreaAddr = NULL;
	//CString szTmp;
	//CString szTmpResult;		// shawn 2009/09/23 for XU
	bool bIsDiff = false;
	bool bIsWrongFile = false;	// shawn 2009/08/19 for check timing table file
	unsigned short m_wVer = 0;
	
	// shawn 2009/09/15 for saving comment +++++
	unsigned long dwSectionPosStr = 0;
	unsigned long dwSectionPosEnd = 0;
	unsigned long dwTmpPos = 0;
	//unsigned long dwTmpPos2 = 0;
	bool bIsData = false;
	// shawn 2009/09/15 for saving comment -----
	
	// Handle Data From SF +++++
	if (!bIsFromUnproccessedFile)
	{
		iTmpAreaCnt = pReadTmp[0];
		pwTmpAreaAddr = (short *)malloc(iTmpAreaCnt+1);

		for (i = 0, j = 1; i <= iTmpAreaCnt; i++, j += 2)
		{
			pwTmpAreaAddr[i] = (pReadTmp[j] << 8) | pReadTmp[j+1];
			
			// shawn 2009/07/10 for 275 +++++
			if (m_iRomVer == SD50232A_2005)
				pwTmpAreaAddr[i] = pwTmpAreaAddr[i] - 0x4800;
			else if (m_iRomVer == ST50275A_3005 || m_iRomVer == ST50276A_4006)	// shawn 2010/07/13 add 276
				pwTmpAreaAddr[i] = pwTmpAreaAddr[i] - 0x6000;
			// shawn 2009/07/10 for 275 -----
		}

		i = pwTmpAreaAddr[0];
	}
	// Handle Data From SF -----
	
	// shawn 2009/08/25 for Check File +++++
	if ( ((pReadTmp[i+6] < 0) || (pReadTmp[i+6] > 3)) || 
		 ((pReadTmp[i+7] < 0) || (pReadTmp[i+7] > 3)) || 
		 ((pReadTmp[i+8] < 0) || (pReadTmp[i+8] > 3)) || 
		 ((pReadTmp[i+9] < 0) || (pReadTmp[i+9] > 3)) || 
		 ((pReadTmp[i+10] < 0) || (pReadTmp[i+10] > 3)) || 
		 ((pReadTmp[i+11] < 0) || (pReadTmp[i+11] > 3)) || 
		 ((pReadTmp[i+12] < 0) || (pReadTmp[i+12] > 3)) || 
		 ((pReadTmp[i+13] < 0) || (pReadTmp[i+13] > 3)) || 
		 (((pReadTmp[i+14]<<8) | (pReadTmp[i+15])) != TABLEFIXLEN) )
	{
		DBG_Print("This File Is Not Timing Table Structure !!");

		if (pwTmpAreaAddr)
		{
				free(pwTmpAreaAddr);
				pwTmpAreaAddr = NULL;
		}

		return false;
	}
	// shawn 2009/08/25 for Check File -----
	
	// Initialize Data +++++
	/*InitMemberData();
	InitOtherData();
	m_CASICDlg->m_bIsModified = false;
	m_CXUDlg->m_bIsModified = false;	// shawn 2009/09/22 for XU
	m_CConfigDlg->m_bIsModified = false;
	m_CConfigDlg->m_CInitTableDlg->m_bIsModified = false;
	m_CConfigDlg->m_CFRTableDlg->m_bIsModified = false;*/
	// Initialize Data -----

	// Header Len +++++
	//m_wHeaderLen = (pReadTmp[i]<<8) | (pReadTmp[i+1]);
	i += 2;
	// Header Len -----

	// Header Version +++++
	m_wVer = (pReadTmp[i]<<8) | (pReadTmp[i+1]);
	
	// shawn 2009/06/23 for 275 +++++
	if (m_wVer == 2)
		m_wVer = HEADERVER;
	// shawn 2009/06/23 for 275 -----
	
	i += 2;

	//szTmp.Format("%02x", m_wVer);
	//GetDlgItem(IDC_EDIT_VER)->SetWindowText(szTmp);
	// Header Version -----

	// Timing Table Len +++++
	//m_wTableLen = (pReadTmp[i]<<8) | (pReadTmp[i+1]);
	i += 2;
	// Timing Table Len -----

	// Number of Format HS +++++
	m_abyNumOfFmt[HIGH_SPEED] = pReadTmp[i];
	i++;
	// Number of Format HS -----

	// Format Index HS +++++
	for (j = 0; j < FMTNUM; i++, j++)
		m_abyFmtIdx[HIGH_SPEED][j] = pReadTmp[i];
	// Format Index HS -----

	// Number of Format FS +++++
	m_abyNumOfFmt[FULL_SPEED] = pReadTmp[i];
	i++;
	// Number of Format FS -----

	// Format Index FS +++++
	for (j = 0; j < FMTNUM; i++, j++)
		m_abyFmtIdx[FULL_SPEED][j] = pReadTmp[i];
	// Format Index FS -----
		
	// Offset +++++
	for (j = 0; j < FMTNUM; i += 2, j++)
		;//m_awOffFmtVideo[HIGH_SPEED][j] = (pReadTmp[i]<<8) | (pReadTmp[i+1]);

	for (j = 0; j < FMTNUM; i += 2, j++)
		;//m_awOffFmtStill[HIGH_SPEED][j] = (pReadTmp[i]<<8) | (pReadTmp[i+1]);

	for (j = 0; j < FMTNUM; i += 2, j++)
		;//m_awOffFmtVideo[FULL_SPEED][j] = (pReadTmp[i]<<8) | (pReadTmp[i+1]);

	for (j = 0; j < FMTNUM; i += 2, j++)
		;//m_awOffFmtStill[FULL_SPEED][j] = (pReadTmp[i]<<8) | (pReadTmp[i+1]);
	// Offset -----

	// Format Video HS +++++
	for (j = 0; j < FMTNUM; j++)
	{
		m_abyNumOfResFmtVideo[HIGH_SPEED][j] = pReadTmp[i];
		i++;
			
		for (k = 0; k < m_abyNumOfResFmtVideo[HIGH_SPEED][j]; k++)
		{
			m_abyNumOfFRFmtVideo[HIGH_SPEED][j][k] = pReadTmp[i];
			i++;
				
			for (l = 0; l < m_abyNumOfFRFmtVideo[HIGH_SPEED][j][k]; l++)
			{
				m_aFRData_FmtVideo[HIGH_SPEED][j][k][l].wFRInt = (pReadTmp[i]<<8) | (pReadTmp[i+1]);
				i += 2;

				m_aFRData_FmtVideo[HIGH_SPEED][j][k][l].byCfgIdx = pReadTmp[i];
				//bIsHighSpeed[pReadTmp[i] - 1] = true;
				i++;

				if (m_aFRData_FmtVideo[HIGH_SPEED][j][k][l].byCfgIdx > iTmpCfgIdx)
					iTmpCfgIdx = m_aFRData_FmtVideo[HIGH_SPEED][j][k][l].byCfgIdx;
			}
		}
	}
	// Format Video HS -----

	// Format Still HS +++++
	for (j = 0; j < FMTNUM; j++)
	{
		m_abyNumOfResFmtStill[HIGH_SPEED][j] = pReadTmp[i];
		i++;
		
		for (k = 0; k < m_abyNumOfResFmtStill[HIGH_SPEED][j]; k++)
		{
			m_abyStillCfgIdx[HIGH_SPEED][j][k] = pReadTmp[i];
			//bIsHighSpeed[pReadTmp[i] - 1] = true;
			i++;

			if (m_abyStillCfgIdx[HIGH_SPEED][j][k] > iTmpCfgIdx)
				iTmpCfgIdx = m_abyStillCfgIdx[HIGH_SPEED][j][k];
		}
	}
	// Format Still HS -----

	// Format Video FS +++++
	for (j = 0; j < FMTNUM; j++)
	{
		m_abyNumOfResFmtVideo[FULL_SPEED][j] = pReadTmp[i];
		i++;
			
		for (k = 0; k < m_abyNumOfResFmtVideo[FULL_SPEED][j]; k++)
		{
			m_abyNumOfFRFmtVideo[FULL_SPEED][j][k] = pReadTmp[i];
			i++;
				
			for (l = 0; l < m_abyNumOfFRFmtVideo[FULL_SPEED][j][k]; l++)
			{
				m_aFRData_FmtVideo[FULL_SPEED][j][k][l].wFRInt = (pReadTmp[i]<<8) | (pReadTmp[i+1]);
				i += 2;

				m_aFRData_FmtVideo[FULL_SPEED][j][k][l].byCfgIdx = pReadTmp[i];
				//bIsHighSpeed[pReadTmp[i] - 1] = false;
				i++;

				if (m_aFRData_FmtVideo[FULL_SPEED][j][k][l].byCfgIdx > iTmpCfgIdx)
					iTmpCfgIdx = m_aFRData_FmtVideo[FULL_SPEED][j][k][l].byCfgIdx;
			}
		}
	}
	// Format Video HS -----

	// Format Still HS +++++
	for (j = 0; j < FMTNUM; j++)
	{
		m_abyNumOfResFmtStill[FULL_SPEED][j] = pReadTmp[i];
		i++;
			
		for (k = 0; k < m_abyNumOfResFmtStill[FULL_SPEED][j]; k++)
		{
			m_abyStillCfgIdx[FULL_SPEED][j][k] = pReadTmp[i];
			//bIsHighSpeed[pReadTmp[i] - 1] = false;
			i++;

			if (m_abyStillCfgIdx[FULL_SPEED][j][k] > iTmpCfgIdx)
				iTmpCfgIdx = m_abyStillCfgIdx[FULL_SPEED][j][k];
		}
	}
	// Format Still HS -----

	// Configs +++++
	for (j = 0; j < CFGNUM; j++)
	{
		aCfgData[j].byFmtIdx = pReadTmp[i];
		i++;
		
		aCfgData[j].byResIdx = pReadTmp[i];
		i++;

		aCfgData[j].wFRInt = (pReadTmp[i]<<8) | (pReadTmp[i+1]);
		i += 2;

		aCfgData[j].wOutWinWid = (pReadTmp[i]<<8) | (pReadTmp[i+1]);
		i += 2;

		aCfgData[j].wOutWinHei = (pReadTmp[i]<<8) | (pReadTmp[i+1]);
		i += 2;

		aCfgData[j].wOutWinHStr = (pReadTmp[i]<<8) | (pReadTmp[i+1]);
		i += 2;

		aCfgData[j].wOutWinVStr = (pReadTmp[i]<<8) | (pReadTmp[i+1]);
		i += 2;

		aCfgData[j].wInWinWid = (pReadTmp[i]<<8) | (pReadTmp[i+1]);
		i += 2;

		aCfgData[j].wInWinHei = (pReadTmp[i]<<8) | (pReadTmp[i+1]);
		i += 2;

		aCfgData[j].wInWinHStr = (pReadTmp[i]<<8) | (pReadTmp[i+1]);
		i += 2;

		aCfgData[j].wInWinVStr = (pReadTmp[i]<<8) | (pReadTmp[i+1]);
		i += 2;

		aCfgData[j].byInitTableIdx = pReadTmp[i] - 9;
		i++;

		aCfgData[j].byFRTableIdx = pReadTmp[i] - 9;
		i++;

		aCfgData[j].bySCL = pReadTmp[i];
		i++;

		aCfgData[j].dwPCK = (pReadTmp[i]<<24) | (pReadTmp[i+1]<<16) | (pReadTmp[i+2]<<8) | (pReadTmp[i+3]);
		i += 4;

		aCfgData[j].wPCKPerLine = (pReadTmp[i]<<8) | (pReadTmp[i+1]);
		i += 2;

		aCfgData[j].wPayLoadSzHS = (pReadTmp[i]<<8) | (pReadTmp[i+1]);
		i += 2;

		aCfgData[j].wPayLoadSzFS = (pReadTmp[i]<<8) | (pReadTmp[i+1]);
		i += 2;

		aCfgData[j].dwFRSzHS = (pReadTmp[i]<<24) | (pReadTmp[i+1]<<16) | (pReadTmp[i+2]<<8) | (pReadTmp[i+3]);
		i += 4;

		aCfgData[j].dwFRSzFS = (pReadTmp[i]<<24) | (pReadTmp[i+1]<<16) | (pReadTmp[i+2]<<8) | (pReadTmp[i+3]);
		i += 4;

		// shawn 2009/06/23 for 275 +++++
		if (m_wVer == HEADERVER)
		{
			aCfgData[j].byIQTableIdx = pReadTmp[i];
			i++;

			if (m_iRomVer == ST50275A_3005 || m_iRomVer == ST50276A_4006)	// shawn 2010/07/13 add 276
				aCfgData[j].byIQTableIdx -= 9;

			for (k = 0; k < CFGRESLEN; k++)
				aCfgData[j].abyReserved[k] = pReadTmp[i+k];

			i += CFGRESLEN;
		}
		// shawn 2009/06/23 for 275 -----

		if ( (aCfgData[j].byInitTableIdx > iTmpInitIdx) && ((aCfgData[j].byInitTableIdx != 0xf5) && (aCfgData[j].byInitTableIdx != 0xf6)) )
			iTmpInitIdx = aCfgData[j].byInitTableIdx;
			
		if ( (aCfgData[j].byFRTableIdx > iTmpFRIdx) && ((aCfgData[j].byFRTableIdx != 0xf5) && (aCfgData[j].byFRTableIdx != 0xf6)) )
			iTmpFRIdx = aCfgData[j].byFRTableIdx;

		// shawn 2009/06/26 for 275 +++++
		if (m_iRomVer == ST50275A_3005 || m_iRomVer == ST50276A_4006)	// shawn 2010/07/13 add 276
		{
			if ( (aCfgData[j].byIQTableIdx > iTmpIQIdx) && ((aCfgData[j].byIQTableIdx != 0xf5) && (aCfgData[j].byIQTableIdx != 0xf6)) )
				iTmpIQIdx = aCfgData[j].byIQTableIdx;
		}
		// shawn 2009/06/26 for 275 -----
			
		if (!bIsFromUnproccessedFile)
		{
			if (i == pwTmpAreaAddr[1])
				break;
		}
		else
		{
			if ( (pReadTmp[i] == 0xff) && 
				 (pReadTmp[i+1] == 0xff) && 
				 (pReadTmp[i+2] == 0xff) && 
				 (pReadTmp[i+3] == 0xff) )
			{
				i += 4;
				break;
			}
		}
	}

	if (iTmpFRIdx)
		iTmpFRIdx -= iTmpInitIdx;
	else
		iTmpFRIdx = 0;

	// shawn 2009/06/26 for 275 +++++
	if (m_iRomVer == ST50275A_3005 || m_iRomVer == ST50276A_4006)	// shawn 2010/07/13 add 276
	{
		if (iTmpIQIdx)
			iTmpIQIdx = iTmpIQIdx - iTmpInitIdx - iTmpFRIdx;
		else
			iTmpIQIdx = 0;
	}
	// shawn 2009/06/26 for 275 -----
		
	for (k = 0; k <= j; k++)
	{
		if ( (aCfgData[k].byInitTableIdx == 0xf5) || (aCfgData[k].byInitTableIdx == 0xf6) )
			aCfgData[k].byInitTableIdx += 9;
		
		if ( (aCfgData[k].byFRTableIdx != 0xf5) && (aCfgData[k].byFRTableIdx != 0xf6) )
			aCfgData[k].byFRTableIdx -= iTmpInitIdx;
		else
			aCfgData[k].byFRTableIdx += 9;

		// shawn 2009/06/26 for 275 +++++
		if (m_iRomVer == ST50275A_3005 || m_iRomVer == ST50276A_4006)	// shawn 2010/07/13 add 276
		{
			if ( (aCfgData[k].byIQTableIdx != 0xf5) && (aCfgData[k].byIQTableIdx != 0xf6) )
				aCfgData[k].byIQTableIdx = aCfgData[k].byIQTableIdx - iTmpInitIdx - iTmpFRIdx;
			else
			{
				// if initial table index is not empty, sync the initial table index and IQ table index
				if ( (aCfgData[k].byInitTableIdx != 0xfe) && (aCfgData[k].byInitTableIdx != 0xff) )
				{
					aCfgData[k].byIQTableIdx = aCfgData[k].byInitTableIdx;

					if (aCfgData[k].byIQTableIdx > iTmpIQIdx)
						iTmpIQIdx = aCfgData[k].byIQTableIdx;
				}
				else
					aCfgData[k].byIQTableIdx += 9;
			}
		}
		// shawn 2009/06/26 for 275 -----
	}

	/*m_CConfigDlg->m_pCCBox_CFGNO->ResetContent();
		
	for (j = 1; j <= iTmpCfgIdx; j++)
	{
		szTmp.Format("%d", j);
		m_CConfigDlg->m_pCCBox_CFGNO->AddString(szTmp);
	}*/
	// Configs -----

	// ASIC Settings +++++
//#if !defined (_DEBUG)
	if (m_iRomVer == SD50232A_2005)
		iTmp = STRVDO232;
	else if (m_iRomVer == ST50275A_3005)
		iTmp = STRVDO275;
	else if (m_iRomVer == ST50276A_4006)	// shawn 2010/07/13 add 276
	{
		// shawn 2010/09/20 modify +++++
		if ( (pReadTmp[i]   == 0x10) && 
			 (pReadTmp[i+1] == 0x61) && 
			 (pReadTmp[i+2] == 0x01) && 
			 (pReadTmp[i+3] == 0x01) && 
			 (pReadTmp[i+4] == 0x11) && 
			 (pReadTmp[i+5] == 0x01) && 
			 (pReadTmp[i+6] == 0x01) && 
			 (pReadTmp[i+7] == 0x00) )
		{
			iTmp = STRVDO276OLD;
			bIsNew276 = false;

			/*BYTE byStr[] = {0x10, 0x61, 0x01, 0x01,
							0x11, 0x01, 0x01, 0x00,
							0x10, 0x61, 0x02, 0x00,
							0x11, 0x01, 0x02, 0x00,
							0x10, 0x01, 0x02, 0x00,
							0x10, 0x01, 0x10, 0x00,
							0x10, 0x0E, 0x01, 0x00,
							0x10, 0x0E, 0x02, 0x00,
							0x10, 0x0E, 0x04, 0x00,
							0x10, 0x0E, 0x10, 0x00,
							0x10, 0x0F, 0x01, 0x00,
							0x10, 0x0F, 0x02, 0x00,
							0x10, 0x0F, 0x04, 0x00,
							0x10, 0x0F, 0x08, 0x00,
							0x10, 0x0F, 0x10, 0x00,
							0x10, 0x0F, 0x20, 0x00,
							0x10, 0xD1, 0x7F, 0,	// user input
							0x11, 0x83, 0x80, 0x00,
							0x10, 0xD9, 0x01, 0x00,
							0x10, 0xD8, 0x01, 0x01,
							0x10, 0xD9, 0x02, 0,	// user input
							0x11, 0x8A, 0x7F, 0x40,
							0x11, 0x8B, 0x3F, 0x30,
							0x10, 0x00, 0x01, 0x00,
							0x10, 0x00, 0x80, 0x00,
							0x11, 0x8C, 0x18, 0x00,
							0x11, 0x8C, 0x02, 0,	// user input
							0x11, 0x8C, 0x04, 0,	// user input
							0x11, 0x81, 0x3F, 0,	// user input
							0x11, 0x80, 0x3C, 0,	// user input
							0x11, 0x80, 0x02, 0,	// user input
							0x10, 0x15, 0x1F, 0,	// user input
							0x11, 0x82, 0x01, 0x00,
							0x11, 0x82, 0x02, 0x00,
							0x11, 0x82, 0x04, 0x00,
							0x11, 0x82, 0x08, 0x00,
							0x11, 0x83, 0x08, 0,	// user input
							0x11, 0x83, 0x01, 0,	// user input
							0x11, 0x83, 0x02, 0,	// user input
							0x11, 0x83, 0x04, 0,	// user input
							0x11, 0x82, 0x10, 0,	// user input
							0x11, 0x82, 0x20, 0,	// user input
							0x11, 0x8C, 0x80, 0,	// user input
							0x11, 0x82, 0x80, 0,	// user input
							0x11, 0x8C, 0x01, 0x00,
							0x11, 0x8C, 0x40, 0x00,
							0x11, 0x80, 0x40, 0x40,
							0x10, 0x67, 0xFF, 0x40,
							0x10, 0x68, 0xFF, 0x30,
							0x10, 0x69, 0xFF, 0x20,
							0x10, 0x6A, 0xFF, 0x10,
							0x10, 0x6B, 0xFF, 0x08};

			for (k = 0, l = 0; k < iTmp; k++, l += 4)
			{
				aASICData[0][k].byAddrH = byStr[l];
				aASICData[0][k].byAddrL = byStr[l+1];
				aASICData[0][k].byMask  = byStr[l+2];
				aASICData[0][k].byValue = byStr[l+3];
			}*/
		}
		else
		{
			iTmp = STRVDO276;
			bIsNew276 = true;
		}
		// shawn 2010/09/20 modify -----
	}
	
	for (j = 0; j < iTmp; j++)
	{
		// shawn 2009/08/19 for using text file of different backend +++++
		if (pReadTmp[i] > 0x1f)
			break;
		
		if (!bIsFromUnproccessedFile)
		{
			if (i == pwTmpAreaAddr[2])
			{
				i--;
				break;
			}
		}
		else
		{
			if ( (pReadTmp[i] == 0xff) && 
				 (pReadTmp[i+1] == 0xff) && 
				 (pReadTmp[i+2] == 0xff) && 
				 (pReadTmp[i+3] == 0xff) )
			{
				i--;
				break;
			}
		}
		// shawn 2009/08/19 for using text file of different backend -----
		
		//aASICData[0][j].byAddrH = pReadTmp[i];
		//aASICData[0][j].byAddrL = pReadTmp[i+1];
		//aASICData[0][j].byMask  = pReadTmp[i+2];
		//aASICData[0][j].byValue = pReadTmp[i+3];
		i += 4;
	}
//#endif
	
	// shawn 2009/09/15 for saving comment +++++
	if (bIsFromUnproccessedFile && !bIsCheckProc)
	{
		GetSectionPos(pReadTmp2, dwSectionPosStr, dwSectionPosEnd, true);	// shawn 2010/09/13 modify
		dwSectionPosStr = dwSectionPosEnd + 1;

		if (pReadTmp2[dwSectionPosStr]   == ',' && 
			pReadTmp2[dwSectionPosStr+1] == ' ' && 
			pReadTmp2[dwSectionPosStr+2] == '\r' && 
			pReadTmp2[dwSectionPosStr+3] == '\n' )
			dwSectionPosStr += 4;
		else if ( (pReadTmp2[dwSectionPosStr] == ',' || pReadTmp2[dwSectionPosStr] == ' ') && 
				 pReadTmp2[dwSectionPosStr+1] == '\r' && 
				 pReadTmp2[dwSectionPosStr+2] == '\n' )
			dwSectionPosStr += 3;
		else if (pReadTmp2[dwSectionPosStr]   == '\r' && 
				 pReadTmp2[dwSectionPosStr+1] == '\n' )
			dwSectionPosStr += 2;
		else if ( (pReadTmp2[dwSectionPosStr] == ',' && pReadTmp2[dwSectionPosStr+1] == ' ') || 
				  (pReadTmp2[dwSectionPosStr] == ' ' && pReadTmp2[dwSectionPosStr+1] == ',') )
			dwSectionPosStr += 2;
		else if (pReadTmp2[dwSectionPosStr] == ',' || pReadTmp2[dwSectionPosStr] == ' ')
			dwSectionPosStr += 1;

		GetSectionPos(pReadTmp2, dwSectionPosStr, dwSectionPosEnd, false);	// shawn 2010/09/13 modify
			
		for (dwTmpPos = dwSectionPosStr; dwTmpPos < dwSectionPosEnd + 1; dwTmpPos++)
		{
	//#if !defined (_DEBUG)
			if (_memicmp(pReadTmp2+dwTmpPos, "# start of sequence", 19) == 0)
			{
				dwTmpPos += 21;
				bIsData = true;
			}
	/*#else
			bIsData = true;
	#endif
			
			if (bIsData)
			{
				szTmp.Format("%c", pReadTmp2[dwTmpPos]);
				aszSenPower[0].Insert(dwTmpPos, szTmp);
			}*/
		}
		
		dwSectionPosStr = dwSectionPosEnd + 1;
	}
	// shawn 2009/09/15 for saving comment -----
	
	for (j = 0; j < SENPOWERLEN; j++)
	{
		//abySenPower[0][j] = pReadTmp[i];
		i++;

		if (!bIsFromUnproccessedFile)
		{
			if (i == pwTmpAreaAddr[2])
			{
				/*abySenPower[0][j+1] = 0xff;
				abySenPower[0][j+2] = 0xff;
				abySenPower[0][j+3] = 0xff;
				abySenPower[0][j+4] = 0xff;*/
				
				break;
			}
		}
		else
		{
			if ( (pReadTmp[i] == 0xff) && 
				 (pReadTmp[i+1] == 0xff) && 
				 (pReadTmp[i+2] == 0xff) && 
				 (pReadTmp[i+3] == 0xff) )
			{
				/*abySenPower[0][j+1] = pReadTmp[i];
				abySenPower[0][j+2] = pReadTmp[i+1];
				abySenPower[0][j+3] = pReadTmp[i+2];
				abySenPower[0][j+4] = pReadTmp[i+3];*/

				i += 4;
				break;
			}
		}
	}
	// ASIC Settings -----

	// Command Head +++++
	// shawn 2009/09/15 for saving comment +++++
	if (bIsFromUnproccessedFile && !bIsCheckProc)
	{
		if (pReadTmp2[dwSectionPosStr]   == ',' && 
			pReadTmp2[dwSectionPosStr+1] == ' ' && 
			pReadTmp2[dwSectionPosStr+2] == '\r' && 
			pReadTmp2[dwSectionPosStr+3] == '\n' )
			dwSectionPosStr += 4;
		else if ( (pReadTmp2[dwSectionPosStr] == ',' || pReadTmp2[dwSectionPosStr] == ' ') && 
				 pReadTmp2[dwSectionPosStr+1] == '\r' && 
				 pReadTmp2[dwSectionPosStr+2] == '\n' )
			dwSectionPosStr += 3;
		else if (pReadTmp2[dwSectionPosStr]   == '\r' && 
				 pReadTmp2[dwSectionPosStr+1] == '\n' )
			dwSectionPosStr += 2;
		else if ( (pReadTmp2[dwSectionPosStr] == ',' && pReadTmp2[dwSectionPosStr+1] == ' ') || 
				  (pReadTmp2[dwSectionPosStr] == ' ' && pReadTmp2[dwSectionPosStr+1] == ',') )
			dwSectionPosStr += 2;
		else if (pReadTmp2[dwSectionPosStr] == ',' || pReadTmp2[dwSectionPosStr] == ' ')
			dwSectionPosStr += 1;

		GetSectionPos(pReadTmp2, dwSectionPosStr, dwSectionPosEnd, false);	// shawn 2010/09/13 modify
				
		for (dwTmpPos = dwSectionPosStr; dwTmpPos < dwSectionPosEnd + 1; dwTmpPos++)
		{
			//szTmp.Format("%c", pReadTmp2[dwTmpPos]);
			//szHead.Insert(dwTmpPos, szTmp);
		}
		
		dwSectionPosStr = dwSectionPosEnd + 1;
	}
	// shawn 2009/09/15 for saving comment -----

	for (j = 0; j < HEADTAILLEN; j++)
	{
		//abyHead[j] = pReadTmp[i];
		i++;

		if (!bIsFromUnproccessedFile)
		{
			if (i == pwTmpAreaAddr[3])
			{
				/*abyHead[j+1] = 0xff;
				abyHead[j+2] = 0xff;
				abyHead[j+3] = 0xff;
				abyHead[j+4] = 0xff;*/
				
				break;
			}
		}
		else
		{
			if ( (pReadTmp[i] == 0xff) && 
				 (pReadTmp[i+1] == 0xff) && 
				 (pReadTmp[i+2] == 0xff) && 
				 (pReadTmp[i+3] == 0xff) )
			{
				/*abyHead[j+1] = pReadTmp[i];
				abyHead[j+2] = pReadTmp[i+1];
				abyHead[j+3] = pReadTmp[i+2];
				abyHead[j+4] = pReadTmp[i+3];*/
				
				i += 4;
				break;
			}
		}
	}
	// Command Head -----

	// Command Tail +++++
	// shawn 2009/09/15 for saving comment +++++
	if (bIsFromUnproccessedFile && !bIsCheckProc)
	{
		if (pReadTmp2[dwSectionPosStr]   == ',' && 
			pReadTmp2[dwSectionPosStr+1] == ' ' && 
			pReadTmp2[dwSectionPosStr+2] == '\r' && 
			pReadTmp2[dwSectionPosStr+3] == '\n' )
			dwSectionPosStr += 4;
		else if ( (pReadTmp2[dwSectionPosStr] == ',' || pReadTmp2[dwSectionPosStr] == ' ') && 
				 pReadTmp2[dwSectionPosStr+1] == '\r' && 
				 pReadTmp2[dwSectionPosStr+2] == '\n' )
			dwSectionPosStr += 3;
		else if (pReadTmp2[dwSectionPosStr]   == '\r' && 
				 pReadTmp2[dwSectionPosStr+1] == '\n' )
			dwSectionPosStr += 2;
		else if ( (pReadTmp2[dwSectionPosStr] == ',' && pReadTmp2[dwSectionPosStr+1] == ' ') || 
				  (pReadTmp2[dwSectionPosStr] == ' ' && pReadTmp2[dwSectionPosStr+1] == ',') )
			dwSectionPosStr += 2;
		else if (pReadTmp2[dwSectionPosStr] == ',' || pReadTmp2[dwSectionPosStr] == ' ')
			dwSectionPosStr += 1;

		GetSectionPos(pReadTmp2, dwSectionPosStr, dwSectionPosEnd, false);	// shawn 2010/09/13 modify
				
		for (dwTmpPos = dwSectionPosStr; dwTmpPos < dwSectionPosEnd + 1; dwTmpPos++)
		{
			//szTmp.Format("%c", pReadTmp2[dwTmpPos]);
			//szTail.Insert(dwTmpPos, szTmp);
		}
		
		dwSectionPosStr = dwSectionPosEnd + 1;
	}
	// shawn 2009/09/15 for saving comment -----

	for (j = 0; j < HEADTAILLEN; j++)
	{
		//abyTail[j] = pReadTmp[i];
		i++;
			
		if (!bIsFromUnproccessedFile)
		{
			if (i == pwTmpAreaAddr[4])
			{
				/*abyTail[j+1] = 0xff;
				abyTail[j+2] = 0xff;
				abyTail[j+3] = 0xff;
				abyTail[j+4] = 0xff;*/
				
				break;
			}
		}
		else
		{
			if ( (pReadTmp[i] == 0xff) && 
				 (pReadTmp[i+1] == 0xff) && 
				 (pReadTmp[i+2] == 0xff) && 
				 (pReadTmp[i+3] == 0xff) )
			{
				/*abyTail[j+1] = pReadTmp[i];
				abyTail[j+2] = pReadTmp[i+1];
				abyTail[j+3] = pReadTmp[i+2];
				abyTail[j+4] = pReadTmp[i+3];*/
					
				i += 4;
				break;
			}
		}
	}
	// Command Tail -----

	// ASIC Settings 2 +++++
	// shawn 2009/09/15 for saving comment +++++
	if (bIsFromUnproccessedFile && !bIsCheckProc)
	{
		if (pReadTmp2[dwSectionPosStr]   == ',' && 
			pReadTmp2[dwSectionPosStr+1] == ' ' && 
			pReadTmp2[dwSectionPosStr+2] == '\r' && 
			pReadTmp2[dwSectionPosStr+3] == '\n' )
			dwSectionPosStr += 4;
		else if ( (pReadTmp2[dwSectionPosStr] == ',' || pReadTmp2[dwSectionPosStr] == ' ') && 
				 pReadTmp2[dwSectionPosStr+1] == '\r' && 
				 pReadTmp2[dwSectionPosStr+2] == '\n' )
			dwSectionPosStr += 3;
		else if (pReadTmp2[dwSectionPosStr]   == '\r' && 
				 pReadTmp2[dwSectionPosStr+1] == '\n' )
			dwSectionPosStr += 2;
		else if ( (pReadTmp2[dwSectionPosStr] == ',' && pReadTmp2[dwSectionPosStr+1] == ' ') || 
				  (pReadTmp2[dwSectionPosStr] == ' ' && pReadTmp2[dwSectionPosStr+1] == ',') )
			dwSectionPosStr += 2;
		else if (pReadTmp2[dwSectionPosStr] == ',' || pReadTmp2[dwSectionPosStr] == ' ')
			dwSectionPosStr += 1;

		GetSectionPos(pReadTmp2, dwSectionPosStr, dwSectionPosEnd, false);	// shawn 2010/09/13 modify
				
		for (dwTmpPos = dwSectionPosStr; dwTmpPos < dwSectionPosEnd + 1; dwTmpPos++)
		{
	//#if !defined (_DEBUG)
			if (_memicmp(pReadTmp2+dwTmpPos, "# end of sequence", 17) == 0)
			{
				//aszSenPower[1].Insert(dwTmpPos, "0xff, 0xff, 0xff, 0xff");
				break;
			}
	//#endif

			//szTmp.Format("%c", pReadTmp2[dwTmpPos]);
			//aszSenPower[1].Insert(dwTmpPos, szTmp);
		}
		
		dwSectionPosStr = dwSectionPosEnd + 1;
	}
	// shawn 2009/09/15 for saving comment -----

	for (j = 0; j < SENPOWERLEN; j++)
	{
		//abySenPower[1][j] = pReadTmp[i];
		i++;

//#if !defined (_DEBUG)
		// shawn 2009/08/19 for using text file of different backend +++++
		if (!bIsFromUnproccessedFile)
		{
			if (i == pwTmpAreaAddr[5])
			{
				bIsDiff = true;
				break;
			}
		}
		else
		{
			if ( (pReadTmp[i] == 0xff) && 
				 (pReadTmp[i+1] == 0xff) && 
				 (pReadTmp[i+2] == 0xff) && 
				 (pReadTmp[i+3] == 0xff) )
			{
				bIsDiff = true;
				break;
			}
		}
		// shawn 2009/08/19 for using text file of different backend -----
		
		if (m_iRomVer == SD50232A_2005)
		{
			if ( (pReadTmp[i])	 == 0x11 && 
				 (pReadTmp[i+1]) == 0x81 && 
				 (pReadTmp[i+2]) == 0xff && 
				 (pReadTmp[i+4]) == 0x10 && 
				 (pReadTmp[i+5]) == 0x00 && 
				 (pReadTmp[i+6]) == 0x80 )
			{
				/*abySenPower[1][j+1] = 0xff;
				abySenPower[1][j+2] = 0xff;
				abySenPower[1][j+3] = 0xff;
				abySenPower[1][j+4] = 0xff;*/
				break;
			}
		}
		else if (m_iRomVer == ST50275A_3005 || m_iRomVer == ST50276A_4006)	// shawn 2010/07/13 add 276
		{
			if ( (pReadTmp[i])	 == 0x11 && 
				 (pReadTmp[i+1]) == 0x83 && 
				 (pReadTmp[i+2]) == 0x80 && 
				 (pReadTmp[i+4]) == 0x10 && 
				 (pReadTmp[i+5]) == 0x00 && 
				 (pReadTmp[i+6]) == 0x01 )
			{
				/*abySenPower[1][j+1] = 0xff;
				abySenPower[1][j+2] = 0xff;
				abySenPower[1][j+3] = 0xff;
				abySenPower[1][j+4] = 0xff;*/
				break;
			}
		}
/*#else
		if (!bIsFromUnproccessedFile)
		{
			if (i == pwTmpAreaAddr[5])
			{
				abySenPower[1][j+1] = 0xff;
				abySenPower[1][j+2] = 0xff;
				abySenPower[1][j+3] = 0xff;
				abySenPower[1][j+4] = 0xff;
				
				break;
			}
		}
		else
		{
			if ( (pReadTmp[i] == 0xff) && 
				 (pReadTmp[i+1] == 0xff) && 
				 (pReadTmp[i+2] == 0xff) && 
				 (pReadTmp[i+3] == 0xff) )
			{
				abySenPower[1][j+1] = pReadTmp[i];
				abySenPower[1][j+2] = pReadTmp[i+1];
				abySenPower[1][j+3] = pReadTmp[i+2];
				abySenPower[1][j+4] = pReadTmp[i+3];

				i += 4;
				break;
			}
		}
#endif*/
	}

//#if !defined (_DEBUG)
	if (m_iRomVer == SD50232A_2005)
		iTmp = STOPVDO232;
	else if (m_iRomVer == ST50275A_3005)
		iTmp = STOPVDO275;
	else if (m_iRomVer == ST50276A_4006)	// shawn 2010/07/13 add 276
	{
		// shawn 2010/09/20 modify +++++
		if (bIsNew276)
			iTmp = 0;
		else
			iTmp = STOPVDO276;
		// shawn 2010/09/20 modify -----
	}
	
	for (j = 0; j < iTmp; j++)
	{
		// shawn 2009/08/19 for using text file of different backend +++++
		if (bIsDiff)
			break;
		// shawn 2009/08/19 for using text file of different backend -----
		
		//aASICData[1][j].byAddrH = pReadTmp[i];
		//aASICData[1][j].byAddrL = pReadTmp[i+1];
		//aASICData[1][j].byMask  = pReadTmp[i+2];
		//aASICData[1][j].byValue = pReadTmp[i+3];
		i += 4;
	}
//#endif

	if (!bIsFromUnproccessedFile)
	{
		// shawn 2010/05/28 modify +++++
		//i += 4;		// shawn 2009/08/19 for using text file of different backend
		while (i != pwTmpAreaAddr[9])
			i++;
		// shawn 2010/05/28 modify -----
	}
	else
	{
//#if !defined (_DEBUG)
		while (iTmpCnt != 5)
		{
			if ( (pReadTmp[i] == 0xff) && 
				 (pReadTmp[i+1] == 0xff) && 
				 (pReadTmp[i+2] == 0xff) && 
				 (pReadTmp[i+3] == 0xff) )
			{
				i += 4;
				iTmpCnt++;
			}
			else
				i++;
		}
		// shawn 2010/05/28 modify -----
/*#else
		// shawn 2010/05/28 modify +++++
		//i += 20;	// shawn 2009/08/19 for using text file of different backend
		while (iTmpCnt != 4)
		{
			if ( (pReadTmp[i] == 0xff) && 
				 (pReadTmp[i+1] == 0xff) && 
				 (pReadTmp[i+2] == 0xff) && 
				 (pReadTmp[i+3] == 0xff) )
			{
				i += 4;
				iTmpCnt++;

				// shawn 2010/06/23 for FW version +++++
				if (iTmpCnt == 1)
				{
					TRACE("pReadTmp[%d] = %x", i, pReadTmp[i]);
					TRACE("pReadTmp[%d] = %x", i+1, pReadTmp[i+1]);
					TRACE("pReadTmp[%d] = %x", i+2, pReadTmp[i+2]);
					TRACE("pReadTmp[%d] = %x", i+3, pReadTmp[i+3]);
					TRACE("pReadTmp[%d] = %x", i+4, pReadTmp[i+4]);
					TRACE("pReadTmp[%d] = %x", i+5, pReadTmp[i+5]);
					TRACE("pReadTmp[%d] = %x", i+6, pReadTmp[i+6]);
					TRACE("pReadTmp[%d] = %x", i+7, pReadTmp[i+7]);
					TRACE("pReadTmp[%d] = %x", i+8, pReadTmp[i+8]);
					TRACE("pReadTmp[%d] = %x", i+9, pReadTmp[i+9]);
					TRACE("pReadTmp[%d] = %x", i+10, pReadTmp[i+10]);
					TRACE("pReadTmp[%d] = %x", i+11, pReadTmp[i+11]);
					TRACE("pReadTmp[%d] = %x", i+12, pReadTmp[i+12]);
					TRACE("pReadTmp[%d] = %x", i+13, pReadTmp[i+13]);
					TRACE("pReadTmp[%d] = %x", i+14, pReadTmp[i+14]);
					TRACE("pReadTmp[%d] = %x", i+15, pReadTmp[i+15]);
				}
				// shawn 2010/06/23 for FW version -----
			}
			else
				i++;
		}
		// shawn 2010/05/28 modify -----
#endif*/
	}

	// shawn 2009/08/19 for using text file of different backend +++++
	if (pReadTmp[i] == 0xaa)
	{
		if (bIsCheckProc)
			DBG_Print("The File Does Not Have Backend Info. !!");
		else
			DBG_Print("The File Does Not Have Backend Info. !! The ASIC Settings Will Be Cleared !!");

		//pASICDlg->m_bIsCleared = true;
		bIsWrongFile = true;
		
		if (!bIsFromUnproccessedFile)
			i += 1;
		else
			i += 5;
	}
	else
	{
		if (pReadTmp[i]   == 0x32 && 
			pReadTmp[i+1] == 0x33 && 
			pReadTmp[i+2] == 0x32 && 
			pReadTmp[i+3] == 0x52 && 
			pReadTmp[i+4] == 0x30 && 
			pReadTmp[i+5] == 0x01 && 
			pReadTmp[i+6] == 0x20 && 
			pReadTmp[i+7] == 0x05)
		{
			if (m_iRomVer != SD50232A_2005)
			{
				if (bIsCheckProc)
					DBG_Print("The File Is Not For This Backend !!");
				else
					DBG_Print("The File Is Not For This Backend !! The ASIC Settings Will Be Cleared !!");
				
				//pASICDlg->m_bIsCleared = true;
				bIsWrongFile = true;
			}
		}
		else if (pReadTmp[i]   == 0x32 && 
				 pReadTmp[i+1] == 0x37 && 
				 pReadTmp[i+2] == 0x35 && 
				 pReadTmp[i+3] == 0x52 && 
				 pReadTmp[i+4] == 0x30 && 
				 pReadTmp[i+5] == 0x01 && 
				 pReadTmp[i+6] == 0x30 && 
				 pReadTmp[i+7] == 0x05)
		{
			if (m_iRomVer != ST50275A_3005)
			{
				if (bIsCheckProc)
					DBG_Print("The File Is Not For This Backend !!");
				else
					DBG_Print("The File Is Not For This Backend !! The ASIC Settings Will Be Cleared !!");

				//pASICDlg->m_bIsCleared = true;
				bIsWrongFile = true;
			}
		}
		else if (pReadTmp[i]   == 0x32 && 	// shawn 2010/07/13 for 276
				 pReadTmp[i+1] == 0x37 && 
				 pReadTmp[i+2] == 0x36 && 
				 pReadTmp[i+3] == 0x52 && 
				 pReadTmp[i+4] == 0x30 && 
				 pReadTmp[i+5] == 0x01 && 
				 pReadTmp[i+6] == 0x40 && 
				 pReadTmp[i+7] == 0x06)
		{
			if (m_iRomVer != ST50276A_4006)
			{
				if (bIsCheckProc)
					DBG_Print("The File Is Not For This Backend !!");
				else
					DBG_Print("The File Is Not For This Backend !! The ASIC Settings Will Be Cleared !!");

				//pASICDlg->m_bIsCleared = true;
				bIsWrongFile = true;
			}
		}
		else	// shawn 2010/05/31 add
		{
			if (bIsCheckProc)
				DBG_Print("The File Is Not For This Backend !!");
			else
				DBG_Print("The File Is Not For This Backend !! The ASIC Settings Will Be Cleared !!");
				
			//pASICDlg->m_bIsCleared = true;
			bIsWrongFile = true;
		}

		if (!bIsFromUnproccessedFile)
		{
			// shawn 2010/05/31 modify +++++
			//i += 64;
			while (i != pwTmpAreaAddr[10])
				i++;
			// shawn 2010/05/31 modify -----
		}
		else
		{
			// shawn 2010/05/31 modify +++++
			//i += 68;
			iTmpCnt = 0;
			
			while (iTmpCnt != 4)
			{
				if ( (pReadTmp[i] == 0xff) && 
					 (pReadTmp[i+1] == 0xff) && 
					 (pReadTmp[i+2] == 0xff) && 
					 (pReadTmp[i+3] == 0xff) )
				{
					i += 4;
					iTmpCnt++;
				}
				else
					i++;
			}
			// shawn 2010/05/31 modify -----
		}
	}
	// shawn 2009/08/19 for using text file of different backend -----
	// ASIC Settings 2 -----

	// Sensor Init Table (I2C Settings) +++++
	// shawn 2009/09/15 for saving comment +++++
	if (bIsFromUnproccessedFile && !bIsCheckProc)
	{
		GetSectionPos(pReadTmp2, dwSectionPosStr, dwSectionPosEnd, true);	// shawn 2010/09/13 modify
		dwSectionPosStr = dwSectionPosEnd + 1;
		GetSectionPos(pReadTmp2, dwSectionPosStr, dwSectionPosEnd, false);	// shawn 2010/09/13 modify
		dwSectionPosStr = dwSectionPosEnd + 1;
		GetSectionPos(pReadTmp2, dwSectionPosStr, dwSectionPosEnd, false);	// shawn 2010/09/13 modify
		dwSectionPosStr = dwSectionPosEnd + 1;
		GetSectionPos(pReadTmp2, dwSectionPosStr, dwSectionPosEnd, false);	// shawn 2010/09/13 modify
		dwSectionPosStr = dwSectionPosEnd + 1;
		GetSectionPos(pReadTmp2, dwSectionPosStr, dwSectionPosEnd, true);	// shawn 2010/09/13 modify
		dwSectionPosStr = dwSectionPosEnd + 1;		
		
		for (j = 0; j < iTmpInitIdx; j++)
		{
			if (pReadTmp2[dwSectionPosStr]   == ',' && 
				pReadTmp2[dwSectionPosStr+1] == ' ' && 
				pReadTmp2[dwSectionPosStr+2] == '\r' && 
				pReadTmp2[dwSectionPosStr+3] == '\n' )
				dwSectionPosStr += 4;
			else if ( (pReadTmp2[dwSectionPosStr] == ',' || pReadTmp2[dwSectionPosStr] == ' ') && 
					 pReadTmp2[dwSectionPosStr+1] == '\r' && 
					 pReadTmp2[dwSectionPosStr+2] == '\n' )
				dwSectionPosStr += 3;
			else if (pReadTmp2[dwSectionPosStr]   == '\r' && 
					 pReadTmp2[dwSectionPosStr+1] == '\n' )
				dwSectionPosStr += 2;
			else if ( (pReadTmp2[dwSectionPosStr] == ',' && pReadTmp2[dwSectionPosStr+1] == ' ') || 
					  (pReadTmp2[dwSectionPosStr] == ' ' && pReadTmp2[dwSectionPosStr+1] == ',') )
				dwSectionPosStr += 2;
			else if (pReadTmp2[dwSectionPosStr] == ',' || pReadTmp2[dwSectionPosStr] == ' ')
				dwSectionPosStr += 1;
			
			GetSectionPos(pReadTmp2, dwSectionPosStr, dwSectionPosEnd, false);	// shawn 2010/09/13 modify

			for (dwTmpPos = dwSectionPosStr; dwTmpPos < dwSectionPosEnd + 1; dwTmpPos++)
			{
				//szTmp.Format("%c", pReadTmp2[dwTmpPos]);
				//aszInit[j].Insert(dwTmpPos, szTmp);
			}
			
			dwSectionPosStr = dwSectionPosEnd + 1;
		}
	}
	// shawn 2009/09/15 for saving comment -----

	for (j = 0; j < iTmpInitIdx; j++)
	{
		for (k = 0; k < INITLEN; k++)
		{
			//abyInit[j][k] = pReadTmp[i];
			i++;
			
			if (!bIsFromUnproccessedFile)
			{
				if (i == pwTmpAreaAddr[11+j])
				{
					/*abyInit[j][k+1] = 0xff;
					abyInit[j][k+2] = 0xff;
					abyInit[j][k+3] = 0xff;
					abyInit[j][k+4] = 0xff;*/
					
					break;
				}
			}
			else
			{
				if ( (pReadTmp[i] == 0xff) && 
					 (pReadTmp[i+1] == 0xff) && 
					 (pReadTmp[i+2] == 0xff) && 
					 (pReadTmp[i+3] == 0xff) )
				{
					/*abyInit[j][k+1] = pReadTmp[i];
					abyInit[j][k+2] = pReadTmp[i+1];
					abyInit[j][k+3] = pReadTmp[i+2];
					abyInit[j][k+4] = pReadTmp[i+3];*/
						
					i += 4;
					break;
				}
			}
		}
	}

	l = 11 + j;
	// Sensor Init Table (I2C Settings) -----

	// Frame Rate Table +++++
	// shawn 2009/09/15 for saving comment +++++
	if (bIsFromUnproccessedFile && !bIsCheckProc)
	{
		for (j = 0; j < iTmpFRIdx; j++)
		{
			if (pReadTmp2[dwSectionPosStr]   == ',' && 
				pReadTmp2[dwSectionPosStr+1] == ' ' && 
				pReadTmp2[dwSectionPosStr+2] == '\r' && 
				pReadTmp2[dwSectionPosStr+3] == '\n' )
				dwSectionPosStr += 4;
			else if ( (pReadTmp2[dwSectionPosStr] == ',' || pReadTmp2[dwSectionPosStr] == ' ') && 
					 pReadTmp2[dwSectionPosStr+1] == '\r' && 
					 pReadTmp2[dwSectionPosStr+2] == '\n' )
				dwSectionPosStr += 3;
			else if (pReadTmp2[dwSectionPosStr]   == '\r' && 
					 pReadTmp2[dwSectionPosStr+1] == '\n' )
				dwSectionPosStr += 2;
			else if ( (pReadTmp2[dwSectionPosStr] == ',' && pReadTmp2[dwSectionPosStr+1] == ' ') || 
					  (pReadTmp2[dwSectionPosStr] == ' ' && pReadTmp2[dwSectionPosStr+1] == ',') )
				dwSectionPosStr += 2;
			else if (pReadTmp2[dwSectionPosStr] == ',' || pReadTmp2[dwSectionPosStr] == ' ')
				dwSectionPosStr += 1;

			GetSectionPos(pReadTmp2, dwSectionPosStr, dwSectionPosEnd, false);	// shawn 2010/09/13 modify

			for (dwTmpPos = dwSectionPosStr; dwTmpPos < dwSectionPosEnd + 1; dwTmpPos++)
			{
				//szTmp.Format("%c", pReadTmp2[dwTmpPos]);
				//aszFR[j].Insert(dwTmpPos, szTmp);
			}
			
			dwSectionPosStr = dwSectionPosEnd + 1;
		}
	}
	// shawn 2009/09/15 for saving comment -----
	
	for (j = 0; j < iTmpFRIdx; j++)
	{
		for (k = 0; k < FRLEN; k++)
		{
			//abyFR[j][k] = pReadTmp[i];
			i++;
			
			if (!bIsFromUnproccessedFile)
			{
				if (i == pwTmpAreaAddr[l+j])
				{
					/*abyFR[j][k+1] = 0xff;
					abyFR[j][k+2] = 0xff;
					abyFR[j][k+3] = 0xff;
					abyFR[j][k+4] = 0xff;*/
					
					break;
				}
			}
			else
			{
				if ( (pReadTmp[i] == 0xff) && 
					 (pReadTmp[i+1] == 0xff) && 
					 (pReadTmp[i+2] == 0xff) && 
					 (pReadTmp[i+3] == 0xff) )
				{
					/*abyFR[j][k+1] = pReadTmp[i];
					abyFR[j][k+2] = pReadTmp[i+1];
					abyFR[j][k+3] = pReadTmp[i+2];
					abyFR[j][k+4] = pReadTmp[i+3];*/
						
					i += 4;
					break;
				}
			}
		}
	}

	//m_CConfigDlg->Refresh(iTmpInitIdx, iTmpFRIdx);
	// Frame Rate Table -----

	// shawn 2009/06/26 for 275 +++++
	if (m_iRomVer == ST50275A_3005 || m_iRomVer == ST50276A_4006)	// shawn 2010/07/13 add 276
	{
		l += j;
		
		// IQ Table +++++
		// shawn 2009/09/15 for saving comment +++++
		if (bIsFromUnproccessedFile && !bIsCheckProc)
		{
			for (j = 0; j < iTmpIQIdx; j++)
			{
				if (pReadTmp2[dwSectionPosStr]   == ',' && 
					pReadTmp2[dwSectionPosStr+1] == ' ' && 
					pReadTmp2[dwSectionPosStr+2] == '\r' && 
					pReadTmp2[dwSectionPosStr+3] == '\n' )
					dwSectionPosStr += 4;
				else if ( (pReadTmp2[dwSectionPosStr] == ',' || pReadTmp2[dwSectionPosStr] == ' ') && 
						 pReadTmp2[dwSectionPosStr+1] == '\r' && 
						 pReadTmp2[dwSectionPosStr+2] == '\n' )
					dwSectionPosStr += 3;
				else if (pReadTmp2[dwSectionPosStr]   == '\r' && 
						 pReadTmp2[dwSectionPosStr+1] == '\n' )
					dwSectionPosStr += 2;
				else if ( (pReadTmp2[dwSectionPosStr] == ',' && pReadTmp2[dwSectionPosStr+1] == ' ') || 
						  (pReadTmp2[dwSectionPosStr] == ' ' && pReadTmp2[dwSectionPosStr+1] == ',') )
					dwSectionPosStr += 2;
				else if (pReadTmp2[dwSectionPosStr] == ',' || pReadTmp2[dwSectionPosStr] == ' ')
					dwSectionPosStr += 1;

				GetSectionPos(pReadTmp2, dwSectionPosStr, dwSectionPosEnd, false);	// shawn 2010/09/13 modify

				for (dwTmpPos = dwSectionPosStr; dwTmpPos < dwSectionPosEnd + 1; dwTmpPos++)
				{
					//szTmp.Format("%c", pReadTmp2[dwTmpPos]);
					//aszIQ[j].Insert(dwTmpPos, szTmp);
				}
				
				dwSectionPosStr = dwSectionPosEnd + 1;
			}
		}
		// shawn 2009/09/15 for saving comment -----

		for (j = 0; j < iTmpIQIdx; j++)
		{
			for (k = 0; k < IQLEN; k++)
			{
				//abyIQ[j][k] = pReadTmp[i];
				i++;
				
				if (!bIsFromUnproccessedFile)
				{
					if (i == pwTmpAreaAddr[l+j])
					{
						/*abyIQ[j][k+1] = 0xff;
						abyIQ[j][k+2] = 0xff;
						abyIQ[j][k+3] = 0xff;
						abyIQ[j][k+4] = 0xff;*/
						
						break;
					}
				}
				else
				{
					if ( (pReadTmp[i] == 0xff) && 
						 (pReadTmp[i+1] == 0xff) && 
						 (pReadTmp[i+2] == 0xff) && 
						 (pReadTmp[i+3] == 0xff) )
					{
						/*abyIQ[j][k+1] = pReadTmp[i];
						abyIQ[j][k+2] = pReadTmp[i+1];
						abyIQ[j][k+3] = pReadTmp[i+2];
						abyIQ[j][k+4] = pReadTmp[i+3];*/
							
						i += 4;
						break;
					}
				}
			}
		}
		// IQ Table -----

		//m_CConfigDlg->Refresh(iTmpInitIdx, iTmpFRIdx);
	}
	// shawn 2009/06/26 for 275 -----

	// shawn 2009/09/23 for XU +++++
	if (bIsFromUnproccessedFile)
	{
		for (j = dwSectionPosStr; pReadTmp2[j] != '\0'; j++)
		{
			if (_memicmp(pReadTmp2+j, "# XU Information", 16) == 0)
			{
				dwSectionPosStr = j + 18;
				break;
			}
		}

		if (pReadTmp2[j] != '\0')
		{
			BYTE	szNum[32];
			BYTE	szNumTmp[32];
			int len = 0;
			
			isp_init_tokenizer(pReadTmp2+dwSectionPosStr);
			isp_get_next_token(szNum, &len);
			*byActualXUNum = (BYTE)isp_to_octal(szNum, len);
			
			BYTE *abyTmp = NULL;
			abyTmp = (BYTE *)malloc(*byActualXUNum*18);
			memset(abyTmp, 0, *byActualXUNum*18);
			k = 0;
			
			while (isp_get_next_token2(szNum, &len))
			{
				memset(szNumTmp, 0, 32);
				
				if (len == 10)
				{
					memcpy(szNumTmp, szNum, 32);
					abyTmp[k] = (BYTE)isp_to_octal(szNumTmp, 4);
					k++;
					memset(szNumTmp, 0, 32);
					memcpy(szNumTmp, szNum+2, 30);
					abyTmp[k] = (BYTE)isp_to_octal(szNumTmp, 4);
					k++;
					memset(szNumTmp, 0, 32);
					memcpy(szNumTmp, szNum+4, 28);
					abyTmp[k] = (BYTE)isp_to_octal(szNumTmp, 4);
					k++;
					memset(szNumTmp, 0, 32);
					memcpy(szNumTmp, szNum+6, 26);
					abyTmp[k] = (BYTE)isp_to_octal(szNumTmp, 4);
				}
				else if (len == 6)
				{
					memcpy(szNumTmp, szNum, 32);
					abyTmp[k] = isp_to_octal(szNumTmp, 4);
					k++;
					memset(szNumTmp, 0, 32);
					memcpy(szNumTmp, szNum+2, 30);
					abyTmp[k] = isp_to_octal(szNumTmp, 4);
				}
				else
				{
					memcpy(szNumTmp, szNum, 32);
					abyTmp[k] = isp_to_octal(szNumTmp, 4);
				}

				k++;
			}
			
			/*for (j = 2; j < szTmp.GetLength(); j++)
			{
				if (szTmp.GetAt(j) == '0' && szTmp.GetAt(j+1) == 'x')
				{
					for (j = j+2; j < szTmp.GetLength(); j += 2)
					{
						if (szTmp.GetAt(j) != ',' && szTmp.GetAt(j) != ' ')
						{
							szTmpResult = szTmp.GetAt(j);
							szTmpResult += szTmp.GetAt(j+1);
							iResult = _tcstoul(szTmpResult, 0, 16);
							abyTmp[k] = iResult;
							k++;
						}
						else
							break;
					}
				}
			}*/

			for (j = 0, k = 0; j < *byActualXUNum; j++)
			{
				// shawn 2009/10/07 for fixed XU1 GUID +++++
				if (j > 0)
				{
					aGUIDXU[j].Data1 = (abyTmp[k]<<24) + (abyTmp[k+1]<<16) + (abyTmp[k+2]<<8) + abyTmp[k+3];
					aGUIDXU[j].Data2 = (abyTmp[k+4]<<8) + abyTmp[k+5];
					aGUIDXU[j].Data3 = (abyTmp[k+6]<<8) + abyTmp[k+7];
					aGUIDXU[j].Data4[0] = abyTmp[k+8];
					aGUIDXU[j].Data4[1] = abyTmp[k+9];
					aGUIDXU[j].Data4[2] = abyTmp[k+10];
					aGUIDXU[j].Data4[3] = abyTmp[k+11];
					aGUIDXU[j].Data4[4] = abyTmp[k+12];
					aGUIDXU[j].Data4[5] = abyTmp[k+13];
					aGUIDXU[j].Data4[6] = abyTmp[k+14];
					aGUIDXU[j].Data4[7] = abyTmp[k+15];
				}
				// shawn 2009/10/07 for fixed XU1 GUID -----

				awXUCTRL[j] = (abyTmp[k+16]<<8) + abyTmp[k+17];
				k += 18;
			}

			if (abyTmp)
			{
				free(abyTmp);
				abyTmp = NULL;
			}
		}

		// shawn 2009/10/09 for inherit from VGA +++++
		/*for (j = dwSectionPosStr; pReadTmp2[j] != '\0'; j++)
		{
			if (_memicmp(pReadTmp2+j, "# Inherit From VGA", 18) == 0)
			{
				bIsInheritFrmVGA = true;
				InheritAction(true);
				break;
			}
		}*/
		// shawn 2009/10/09 for inherit from VGA -----
	}
	// shawn 2009/09/23 for XU -----

	// shawn 2009/08/19 for check timing table file +++++
	if (bIsWrongFile && bIsCheckProc)
	{
		//bIsReadFile = false;	// shawn 2009/10/08 for XU
		//NewAction();			// shawn 2009/09/28 modify

		if (pwTmpAreaAddr)
		{
				free(pwTmpAreaAddr);
				pwTmpAreaAddr = NULL;
		}

		return false;
	}
	// shawn 2009/08/19 for check timing table file -----

	// Tree Structure +++++
	/*m_CConfigDlg->ShowWindow(SW_HIDE);
	m_CConfigDlg->m_CInitTableDlg->ShowWindow(SW_HIDE);
	m_CConfigDlg->m_CFRTableDlg->ShowWindow(SW_HIDE);

	for (i = IDC_STATIC_NEWEDIT; i <= IDC_BUTTON_SAVE_EDIT; i++)
		GetDlgItem(i)->ShowWindow(SW_HIDE);

	m_TreeStruct.iFirst = 0;
	m_TreeStruct.iSecond = 0;
	m_TreeStruct.iThird = 0;
	m_TreeStruct.iFourth = 0;
	m_TreeStruct.iFifth = 0;
	TreeStructure();*/
	// Tree Structure -----
	
	if (pwTmpAreaAddr)
	{
			free(pwTmpAreaAddr);
			pwTmpAreaAddr = NULL;
	}
	
	return true;
}
