#include <memory.h>
#include <ctype.h>

void 	_str_tolower(char str[256], size_t len)
{
	int	i;
	for (i=0; i<(int)len; i++)
		str[i] = tolower(str[i]);
}

int 	_memicmp(const void *buf1, const void *buf2, size_t count)
{
	char	s1[256], s2[256];
	
	memcpy(s1, buf1, count);
	memcpy(s2, buf2, count);
	_str_tolower(s1, count);
	_str_tolower(s2, count);
	
	return memcmp(s1, s2, count);
}

bool	_buf_cmp(unsigned char *buf1, unsigned char *buf2, unsigned int len)
{
	unsigned int	i;
	
	for (i=0; i<len; i++)
	{
		if (buf1[i] != buf2[i])
		{	
			return false;
		}
	}
	return true;
}
