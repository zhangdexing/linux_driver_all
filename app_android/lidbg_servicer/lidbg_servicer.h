#ifndef _LIGDBG_SERVER_APP__
#define _LIGDBG_SERVER_APP__

#define LIDBG_CALL(cmd,buf,ret_bytes) do{\
	int fd;\
	 fd = open("/dev/mlidbg0", O_RDWR);\
	 if((fd == 0)||(fd == 0xfffffffe)|| (fd == 0xffffffff))break;\
	 write(fd, cmd, sizeof(cmd));\
	 if((buf != NULL)&&(ret_bytes))\
	 {\
	 	read(fd, buf, ret_bytes);\
	 }\
	close(fd);\
}while(0)

#if 1
#define LIDBG_PRINT(msg...) do{\
	int fd;\
	char s[64];\
	sprintf(s, "lidbg_msg: " msg);\
	 fd = open("/dev/lidbg_msg", O_RDWR);\
	 if((fd == 0)||((int)fd == 0xfffffffe)|| (fd == 0xffffffff))break;\
	 write(fd, s, 64);\
	 close(fd);\
}while(0)
#endif



#endif
