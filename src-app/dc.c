#include "dc.h"
#include <linux/fb.h>
#include <stdlib.h>

struct device_context
{
    uint8_t* pbuff;
    int screen_w;
    int screen_h;
    int byte_per_pxl;
    
    
};
