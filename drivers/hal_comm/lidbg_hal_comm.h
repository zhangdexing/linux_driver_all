
#ifndef _IOCOM_NONE
# define _IOCOM_NONE      0U
#endif
 
#ifndef _IOCOM_WRITE
# define _IOCOM_WRITE     1U
#endif
 
#ifndef _IOCOM_READ
# define _IOCOM_READ      2U
#endif

#define _IOCOM_DIRBITS 2
#define _IOCOM_SIZEBITS	30

#define _IOCOM_DIRMASK ((1 << _IOCOM_DIRBITS)-1)
#define _IOCOM_SIZEMASK ((1 << _IOCOM_SIZEBITS)-1)


#define _IOCOM_SIZESHIFT 0
#define _IOCOM_DIRSHIFT (_IOCOM_SIZESHIFT + _IOCOM_SIZEBITS)

#define _IOCOM(dir,size) \
	(((dir) << _IOCOM_DIRSHIFT) | \
	 ((size) << _IOCOM_SIZESHIFT))

/* used to create numbers */
#define _IOOM(size)     _IOCOM(_IOCOM_NONE,(size))
#define _IOROM(size)      _IOCOM(_IOCOM_READ,(size))
#define _IOWOM(size)      _IOCOM(_IOCOM_WRITE,(size))

/* used to decode ioctl numbers.. */
#define _IOCOM_DIR(arg)            (((arg) >> _IOCOM_DIRSHIFT) & _IOCOM_DIRMASK)
#define _IOCOM_SIZE(arg)           (((arg) >> _IOCOM_SIZESHIFT) & _IOCOM_SIZEMASK)


