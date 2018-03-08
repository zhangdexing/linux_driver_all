
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
#define _IOCOM_TYPEBITS 8
#define _IOCOM_SIZEBITS	22

#define _IOCOM_TYPEMASK ((1 << _IOCOM_TYPEBITS)-1)
#define _IOCOM_DIRMASK ((1 << _IOCOM_DIRBITS)-1)
#define _IOCOM_SIZEMASK ((1 << _IOCOM_SIZEBITS)-1)


#define _IOCOM_TYPESHIFT 0
#define _IOCOM_SIZESHIFT (_IOCOM_TYPESHIFT + _IOCOM_TYPEBITS)
#define _IOCOM_DIRSHIFT (_IOCOM_SIZESHIFT + _IOCOM_SIZEBITS)

#define _IOCOM(dir,type,size) \
	(((dir) << _IOCOM_DIRSHIFT) | \
	 ((type) << _IOCOM_TYPESHIFT) | \
	 ((size) << _IOCOM_SIZESHIFT))

/* used to create numbers */
#define _IOOM(type,size)     _IOCOM(_IOCOM_NONE,(type),(size))
#define _IOROM(type,size)      _IOCOM(_IOCOM_READ,(type),(size))
#define _IOWOM(type,size)      _IOCOM(_IOCOM_WRITE,(type),(size))

/* used to decode ioctl numbers.. */
#define _IOCOM_DIR(arg)            (((arg) >> _IOCOM_DIRSHIFT) & _IOCOM_DIRMASK)
#define _IOCOM_TYPE(arg)           (((arg) >> _IOCOM_TYPESHIFT) & _IOCOM_TYPEMASK)
#define _IOCOM_SIZE(arg)           (((arg) >> _IOCOM_SIZESHIFT) & _IOCOM_SIZEMASK)


