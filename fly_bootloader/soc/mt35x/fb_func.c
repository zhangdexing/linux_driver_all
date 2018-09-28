#include <video_fb.h>
#include "../soc.h"

void *fb_base_get()
{
	return mt_get_fb_addr();
}
