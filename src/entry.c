/*! \brief
    \file entry.c
    \author Seungwoo Kang (ki6080@gmail.com)
    \version 0.1
    \date 2019-11-23 
    \copyright Copyright (c) 2019. Seungwoo Kang. All rights reserved.
    
    \details
 */
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
#include "core/program.h"
#include <time.h>
#include <sys/time.h>

#define DESIRED_DELTA_TIME (1.0 / 120.0)
#define RENDERING_PERIOD 5

bool g_bRun = true;
static double g_TimeInSeconds;
UProgramInstance *g_pInst;

double GetTimeInSeconds()
{
    return g_TimeInSeconds;
}

void sigint_handler(int signo)
{
    lvlog(LOGLEVEL_INFO, "SIG %d RECV\n", signo);
    g_bRun = false;
}

static inline uint64_t time_in_100usec(struct timeval const *v)
{
    return v->tv_sec * 10000ull + v->tv_usec / 100;
}

static inline double time_100usec_to_sec(uint64_t usec)
{
    return usec / 10000.0;
}

int main(int argc, char *argv[])
{
    g_logLv = LOGLEVEL_VERBOSE;

    signal(SIGABRT, sigint_handler);
    signal(SIGINT, sigint_handler);
    signal(SIGKILL, sigint_handler);
    signal(SIGQUIT, sigint_handler);
    signal(SIGSTOP, sigint_handler);

    UProgramInstance *program;
    {
        struct ProgramInstInitStruct init;
        PInst_InitializeInitStruct(&init);

        init.NumMaxDrawCall = 0x8000;
        init.NumMaxResource = 0x2000;
        init.RenderStringPoolSize = 0x4000;

        g_pInst = program = PInst_Create(&init);
    }
    uassert(g_pInst);

    // Timer to elapse delta time.
    struct timeval tv;
    double prev_tick, curtime;
    float delta;
    gettimeofday(&tv, NULL);
    curtime = prev_tick = time_100usec_to_sec(time_in_100usec(&tv));

    void OnUpdate(float DeltaTime);
    void OnDestroyGameInstance();
    OnInitGame();

    const size_t render_period_add[2] = {1, 1 - RENDERING_PERIOD};
    size_t render_period_counter = 0;

    // Main program loop
    while (g_bRun)
    {
        PInst_SetRenderingLock(g_pInst, render_period_counter != 0);
        render_period_counter += render_period_add[render_period_counter == (RENDERING_PERIOD - 1)];
        // Wait until delta seconds
        for (; (delta = curtime - prev_tick) < DESIRED_DELTA_TIME;)
        {
            gettimeofday(&tv, NULL);
            curtime = time_100usec_to_sec(time_in_100usec(&tv));
            // pthread_yield(NULL);
        }
        prev_tick = curtime;
        g_TimeInSeconds = curtime;

        // Update program timer
        PInst_UpdateTimer(program, delta);

        // Update game state
        OnUpdate(delta);

        // Flip Buffer
        EStatus flip_result;
        while (g_bRun && (flip_result = PInst_Flip(program)) != STATUS_OK)
        {
            if (flip_result == RENDERER_LOCKED)
                break;
        }

        lvlog(LOGLEVEL_VERBOSE + 1000, "Update() called. Cur time is %f\n", curtime);
    }
    OnDestroyGameInstance();
    PInst_Destroy(program);

    lvlog(LOGLEVEL_INFO, "Shutting down.\n");
    return 0;
}

// -----------------------------------------------------------------------------------------
// typedef struct _cairo_linuxfb_device
// {
//     int fb_fd;
//     char *fb_data;
//     long fb_screensize;
//     struct fb_var_screeninfo fb_vinfo;
//     struct fb_fix_screeninfo fb_finfo;
// } cairo_linuxfb_device_t;

// static void cairo_linuxfb_surface_destroy(void *device)
// {
//     cairo_linuxfb_device_t *dev = (cairo_linuxfb_device_t *)device;

//     if (dev == NULL)
//     {
//         return;
//     }
//     munmap(dev->fb_data, dev->fb_screensize);
//     close(dev->fb_fd);
//     free(dev);
// }

// static cairo_surface_t *cairo_linuxfb_surface_create(const char *fb_name)
// {
//     cairo_linuxfb_device_t *device;
//     cairo_surface_t *surface;

//     if (fb_name == NULL)
//     {
//         fb_name = "/dev/fb0";
//     }

//     device = malloc(sizeof(*device));

//     // Open the file for reading and writing
//     device->fb_fd = open(fb_name, O_RDWR);
//     if (device->fb_fd == -1)
//     {
//         perror("Error: cannot open framebuffer device");
//         exit(1);
//     }

//     // Get variable screen information
//     if (ioctl(device->fb_fd, FBIOGET_VSCREENINFO, &device->fb_vinfo) == -1)
//     {
//         perror("Error reading variable information");
//         exit(3);
//     }

//     // Figure out the size of the screen in bytes
//     device->fb_screensize = device->fb_vinfo.xres * device->fb_vinfo.yres * device->fb_vinfo.bits_per_pixel / 8;

//     // Map the device to memory
//     device->fb_data = (char *)mmap(0, device->fb_screensize,
//                                    PROT_READ | PROT_WRITE, MAP_SHARED,
//                                    device->fb_fd, 0);
//     memset(device->fb_data, 0, device->fb_screensize);

//     if ((int)device->fb_data == -1)
//     {
//         perror("Error: failed to map framebuffer device to memory");
//         exit(4);
//     }

//     // Get fixed screen information
//     if (ioctl(device->fb_fd, FBIOGET_FSCREENINFO, &device->fb_finfo) == -1)
//     {
//         perror("Error reading fixed information");
//         exit(2);
//     }

//     surface = cairo_image_surface_create_for_data(device->fb_data,
//                                                   CAIRO_FORMAT_ARGB32,
//                                                   device->fb_vinfo.xres,
//                                                   device->fb_vinfo.yres,
//                                                   cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32,
//                                                                                 device->fb_vinfo.xres));

//     logprintf("xres: %u, yres: %u, bpp: %d\n",
//               device->fb_vinfo.xres,
//               device->fb_vinfo.yres,
//               device->fb_vinfo.bits_per_pixel);
//     cairo_surface_set_user_data(surface, NULL, device,
//                                 &cairo_linuxfb_surface_destroy);

//     return surface;
// }

// int main(int argc, char *argv[])
// {
//     signal(SIGINT, sigint_handler);

//     cairo_surface_t *surface;
//     cairo_surface_t *fbsurf;
//     void *surfaceBuff;
//     cairo_t *cr;
//     cairo_t *fbout;

//     fbsurf = cairo_linuxfb_surface_create(NULL);
//     fbout = cairo_create(fbsurf);

//     size_t bw = cairo_image_surface_get_width(fbsurf);
//     size_t bh = cairo_image_surface_get_height(fbsurf);
//     size_t bstride = cairo_image_surface_get_stride(fbsurf);
//     size_t buffSz = bh * bstride;
//     cairo_format_t fmt = cairo_image_surface_get_format(fbsurf);
//     surfaceBuff = malloc(bstride * bh);
//     surface = cairo_image_surface_create_for_data(surfaceBuff, fmt, bw, bh, bstride);

//     cairo_select_font_face(cr, "serif", CAIRO_FONT_SLANT_NORMAL,
//                            CAIRO_FONT_WEIGHT_BOLD);
//     // cairo_set_font_size(cr, 32.0);
//     // cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
//     // cairo_move_to(cr, 300.0, 300.0);

//     // cairo_show_text(cr, "Hello, CairoGraphics!");

//     cairo_surface_t *image = cairo_image_surface_create_from_png("../resource/rsrc.png");
//     cairo_t *image_context = cairo_create(image);
//     double angle = 0;

//     while (g_bRun)
//     {
//         cr = cairo_create(surface);
//         void *data = cairo_image_surface_get_data(surface);
//         memset(data, 0xff, buffSz);

//         cairo_set_source_rgb(cr, 1.0, 1, 1);
//         cairo_move_to(cr, 0, 0);
//         cairo_line_to(cr, 500, 100);
//         cairo_move_to(cr, 100, 0);
//         cairo_line_to(cr, 0, 100);
//         cairo_set_line_width(cr, 1);
//         cairo_stroke(cr);

//         cairo_rectangle(cr, 0, 0, 500, 100);
//         cairo_set_source_rgba(cr, 1, 0, 0, 0.801);
//         cairo_fill(cr);

//         cairo_rectangle(cr, 0, 50, 50, 50);
//         cairo_set_source_rgba(cr, 0, 1, 0, 0.60);
//         cairo_fill(cr);

//         cairo_rectangle(cr, 50, 0, 50, 50);
//         cairo_set_source_rgba(cr, 0, 0, 1, 0.40);
//         cairo_fill(cr);

//         int w = cairo_image_surface_get_width(image);
//         int h = cairo_image_surface_get_height(image);
//         // printf("image sz [%d %d]\n", w, h);
//         cairo_set_source_rgba(cr, 0, 0, 1, 1);
//         cairo_rotate(cr, angle += 0.01);
//         cairo_rectangle(cr, 100, 100, 100, 100);
//         cairo_fill(cr);
//         cairo_set_source_surface(cr, image, 0, 0);
//         cairo_paint(cr);

//         cairo_rotate(cr, angle * 2);
//         cairo_set_source_rgba(cr, 1.0, 0, 0, 0.65);
//         cairo_rectangle(cr, 100, 100, 300, 300);
//         cairo_fill(cr);

//         double scx, scy, pxx, pxy;
//         // cairo_surface_get_device_scale(surface, &scx, &scy);
//         // cairo_surface_get_fallback_resolution(surface, &pxx, &pxy);
//         // printf("Device scale: %f, %f\npx Per inch: %f, %f \n", scx, scy, pxx, pxy);

//         cairo_set_source_surface(fbout, surface, 0, 0);
//         cairo_paint(fbout);
//         cairo_destroy(cr);
//     }
//     cairo_destroy(image_context);
//     cairo_destroy(fbout);
//     cairo_surface_destroy(surface);
//     cairo_surface_destroy(fbsurf);
//     cairo_surface_destroy(image);

//     printf("Successfully terminated.\n");
//     return 0;
// }

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
