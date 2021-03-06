#ifndef _LIGDBG_INSMOD__
#define _LIGDBG_INSMOD__
#include <sys/syscall.h>
inline void *read_file( char *filename, ssize_t *_size)
{
    int ret, fd;
    struct stat sb;
    ssize_t size;
    void *buffer = NULL;

    /* open the file */
    fd = open(filename, O_RDONLY);
    if (fd < 0)
        return NULL;

    /* find out how big it is */
    if (fstat(fd, &sb) < 0)
        goto bail;
    size = sb.st_size;

    /* allocate memory for it to be read into */
    buffer = malloc(size);
    if (!buffer)
        goto bail;

    /* slurp it into our buffer */
    ret = read(fd, buffer, size);
    if (ret != size)
        goto bail;

    /* let the caller know how big it is */
    *_size = size;

bail:
    close(fd);
    return buffer;
}

int init_module(void *module_image, unsigned long len,
                const char *param_values);
int module_insmod(char *file);

inline int module_insmod(char *file)
{
	int fd = open(file, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
	if (fd == -1) {
		lidbg("insmod: open(\"%s\") failed: %s", file, strerror(errno));
		return -1;
	}
	int rc = syscall(__NR_finit_module, fd, "", 0);
	if (rc == -1) {
		lidbg("finit_module for \"%s\" failed: %s", file, strerror(errno));
	}
	close(fd);
	return rc;

}

#if 0
inline int module_insmod(char *file)
{
    void *module = NULL;
    ssize_t size = 0;
    int ret;
    module = read_file(file, &size);
    if(!module)
    {
        lidbg("not found module : \"%s\" \n", file);
        return -1;
    }
    else
    {
        ret = init_module(module, size, "");
        if(ret < 0)
        {
            if (0 == memcmp("File exists", strerror(errno), strlen(strerror(errno))))
            {
                lidbg("insmod -> %s File exists", file);
                free(module);
                return 0;
            }

            lidbg("init module \"%s\" fail!\n", file);
            free(module);
            return -1;
        }
        else
        {
            lidbg("init module success! \"%s\" \n", file);
            free(module);
            return 0;
        }
    }
}
#endif
#endif
