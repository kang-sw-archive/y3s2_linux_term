#include <stdio.h>
#include <sys/stat.h>
#include <signal.h>
#include "fbgraphics.h"

int keep_running = 1;

void int_handler(int dummy)
{
    keep_running = 0;
}

int main(int argc, char *argv[])
{
    signal(SIGINT, int_handler);

    struct _fbg *fbg = fbg_setup("/dev/fb0", 0); // you can also directly use fbg_init(); for "/dev/fb0", last argument mean that will not use page flipping mechanism  for double buffering (it is actually slower on some devices!)

    do
    {
        fbg_clear(fbg, 0); // can also be replaced by fbg_fill(fbg, 0, 0, 0);

        fbg_draw(fbg);

        fbg_rect(fbg, fbg->width / 2 - 32, fbg->height / 2 - 32, 16, 16, 0, 255, 0);

        fbg_pixel(fbg, fbg->width / 2, fbg->height / 2, 255, 0, 0);

        fbg_flip(fbg);

    } while (keep_running);

    fbg_close(fbg);

    return 0;
}
