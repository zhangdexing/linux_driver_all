#ifndef _SEM_CMD_H
#define _SEM_CMD_H


#define SEM_MAGIC 's' //????
#define SEM_MAX_NR 2 //?????????

//ioctl
#define SEM_LOCK _IO(SEM_MAGIC, 1) 		//????
#define SEM_UNLOCK _IO(SEM_MAGIC, 2)	//????

#define READ_PAGEID_FORJAVA _IOR(SEM_MAGIC, 3 ,int) //JAVA??????
#define WRITE_PAGEID_FORJAVA _IOW(SEM_MAGIC, 4 ,short)	//JAVA?????

#define READ_PAGEID_FORC _IOR(SEM_MAGIC, 5 ,int) //C??????
#define WRITE_PAGEID_FORC _IOW(SEM_MAGIC, 6 ,short)	//C?????

#endif /*_SEM_CMD_H*/
