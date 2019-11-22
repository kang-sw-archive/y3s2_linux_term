
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <cairo.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>

static bool g_bRun = true;

void sigint_handler(int signo)
{
    g_bRun = false;
}

typedef struct _cairo_linuxfb_device
{
    int fb_fd;
    char *fb_data;
    long fb_screensize;
    struct fb_var_screeninfo fb_vinfo;
    struct fb_fix_screeninfo fb_finfo;
} cairo_linuxfb_device_t;

static void cairo_linuxfb_surface_destroy(void *device)
{
    cairo_linuxfb_device_t *dev = (cairo_linuxfb_device_t *)device;

    if (dev == NULL)
    {
        return;
    }
    munmap(dev->fb_data, dev->fb_screensize);
    close(dev->fb_fd);
    free(dev);
}

cairo_surface_t *cairo_linuxfb_surface_create(const char *fb_name)
{
    cairo_linuxfb_device_t *device;
    cairo_surface_t *surface;

    if (fb_name == NULL)
    {
        fb_name = "/dev/fb0";
    }

    device = malloc(sizeof(*device));

    // Open the file for reading and writing
    device->fb_fd = open(fb_name, O_RDWR);
    if (device->fb_fd == -1)
    {
        perror("Error: cannot open framebuffer device");
        exit(1);
    }

    // Get variable screen information
    if (ioctl(device->fb_fd, FBIOGET_VSCREENINFO, &device->fb_vinfo) == -1)
    {
        perror("Error reading variable information");
        exit(3);
    }

    // Figure out the size of the screen in bytes
    device->fb_screensize = device->fb_vinfo.xres * device->fb_vinfo.yres * device->fb_vinfo.bits_per_pixel / 8;

    // Map the device to memory
    device->fb_data = (char *)mmap(0, device->fb_screensize,
                                   PROT_READ | PROT_WRITE, MAP_SHARED,
                                   device->fb_fd, 0);
    memset(device->fb_data, 0, device->fb_screensize);
    
    if ((int)device->fb_data == -1)
    {
        perror("Error: failed to map framebuffer device to memory");
        exit(4);
    }

    // Get fixed screen information
    if (ioctl(device->fb_fd, FBIOGET_FSCREENINFO, &device->fb_finfo) == -1)
    {
        perror("Error reading fixed information");
        exit(2);
    }

    surface = cairo_image_surface_create_for_data(device->fb_data,
                                                  CAIRO_FORMAT_ARGB32,
                                                  device->fb_vinfo.xres,
                                                  device->fb_vinfo.yres,
                                                  cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32,
                                                                                device->fb_vinfo.xres));

    printf("xres: %u, yres: %u, bpp: %d\n",
           device->fb_vinfo.xres,
           device->fb_vinfo.yres,
           device->fb_vinfo.bits_per_pixel);
    cairo_surface_set_user_data(surface, NULL, device,
                                &cairo_linuxfb_surface_destroy);

    return surface;
}

int main(int argc, char *argv[])
{
    signal(SIGINT, sigint_handler);

    cairo_surface_t *surface; 
    cairo_surface_t *fbsurf;
    void *surfaceBuff;
    cairo_t *cr;
    cairo_t *fbout;
    
    fbsurf = cairo_linuxfb_surface_create(NULL);
    fbout = cairo_create(fbsurf);

    size_t bw = cairo_image_surface_get_width(fbsurf);
    size_t bh = cairo_image_surface_get_height(fbsurf);
    size_t bstride = cairo_image_surface_get_stride(fbsurf);
    size_t buffSz = bh * bstride;
    cairo_format_t fmt = cairo_image_surface_get_format(fbsurf);
    surfaceBuff = malloc(bstride * bh);
    surface = cairo_image_surface_create_for_data(surfaceBuff, fmt, bw, bh, bstride);
    
    // cairo_select_font_face(cr, "serif", CAIRO_FONT_SLANT_NORMAL,
    //                        CAIRO_FONT_WEIGHT_BOLD);
    // cairo_set_font_size(cr, 32.0);
    // cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    // cairo_move_to(cr, 300.0, 300.0);

    // cairo_show_text(cr, "Hello, CairoGraphics!");

    cairo_surface_t *image = cairo_image_surface_create_from_png("../resource/rsrc.png");
    cairo_t *image_context = cairo_create(image);
    double angle = 0;
    
    while (g_bRun)
    {
        cr = cairo_create(surface);
        void *data = cairo_image_surface_get_data(surface);
        memset(data, 0xff, buffSz);
        
        cairo_set_source_rgb(cr, 1.0, 1, 1);
        cairo_move_to(cr, 0, 0);
        cairo_line_to(cr, 500, 100);
        cairo_move_to(cr, 100, 0);
        cairo_line_to(cr, 0, 100);
        cairo_set_line_width(cr, 1);
        cairo_stroke(cr);

        cairo_rectangle(cr, 0, 0, 500, 100);
        cairo_set_source_rgba(cr, 1, 0, 0, 0.801);
        cairo_fill(cr);

        cairo_rectangle(cr, 0, 50, 50, 50);
        cairo_set_source_rgba(cr, 0, 1, 0, 0.60);
        cairo_fill(cr);

        cairo_rectangle(cr, 50, 0, 50, 50);
        cairo_set_source_rgba(cr, 0, 0, 1, 0.40);
        cairo_fill(cr);
        
        int w = cairo_image_surface_get_width(image);
        int h = cairo_image_surface_get_height(image);
        // printf("image sz [%d %d]\n", w, h);
        cairo_set_source_rgba(cr, 0, 0, 1, 1);
        cairo_rotate(cr, angle += 0.01);
        cairo_rectangle(cr, 100, 100, 100, 100);
        cairo_fill(cr);
        cairo_set_source_surface(cr, image, 0, 0);
        cairo_paint(cr);

        cairo_rotate(cr, angle * 2);
        cairo_set_source_rgba(cr, 1.0, 0, 0, 0.65);
        cairo_rectangle(cr, 100, 100, 300, 300);
        cairo_fill(cr);

        double scx, scy, pxx, pxy;
        // cairo_surface_get_device_scale(surface, &scx, &scy);
        // cairo_surface_get_fallback_resolution(surface, &pxx, &pxy);
        // printf("Device scale: %f, %f\npx Per inch: %f, %f \n", scx, scy, pxx, pxy);

        cairo_set_source_surface(fbout, surface, 0, 0);
        cairo_paint(fbout);
        cairo_destroy(cr);
    }
    cairo_destroy(image_context);
    cairo_destroy(fbout);
    cairo_surface_destroy(surface);
    cairo_surface_destroy(fbsurf);
    cairo_surface_destroy(image);

    printf("Successfully terminated.\n");
    return 0;
}

// #include <stdio.h>
// #include <sys/stat.h>
// #include <signal.h>
// #include "fbgraphics.h"

// int keep_running = 1;

// void int_handler(int dummy)
// {
//     keep_running = 0;
// }

// int main(int argc, char *argv[])
// {
//     signal(SIGINT, int_handler);

//     struct _fbg *fbg = fbg_setup("/dev/fb0", 0); // you can also directly use fbg_init(); for "/dev/fb0", last argument mean that will not use page flipping mechanism  for double buffering (it is actually slower on some devices!)

//     int i = 0;

//     do
//     {
//         fbg_clear(fbg, 0); // can also be replaced by fbg_fill(fbg, 0, 0, 0);

//         fbg_draw(fbg);
 
//             /* code */
//         fbg_rect(fbg, i + fbg->width / 2 - 32, fbg->height / 2 - 32, 16, 16, 0, 255, 0);
//         i = (i + 1) % 100;

//         fbg_pixel(fbg, fbg->width / 2, fbg->height / 2, 255, 0, 0);

//         fbg_flip(fbg);

//     } while (keep_running);

//     fbg_close(fbg);

//     return 0;
// }
