/*! \brief Cairo library interface code
    \file program-fb.c
    \author Seungwoo Kang (ki6080@gmail.com)
    \version 0.1
    \date 2019-11-22
    
    \copyright Copyright (c) 2019. Seungwoo Kang. All rights reserved.
 */
#include "../core/program.h"
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

// -- Resource descriptors
// Image descriptors
typedef struct
{
    char const *fontFamily;
    cairo_font_slant_t slant;
    cairo_font_weight_t weight;
} rsrc_font_t;

typedef struct
{
    cairo_surface_t *screen;
    void *backbuffer_memory[RENDERER_NUM_MAX_BUFFER];
    cairo_surface_t *backbuffer[RENDERER_NUM_MAX_BUFFER];
} program_cairo_wrapper_t;

static cairo_surface_t *cairo_linuxfb_surface_create(const char *fb_name);

void *Internal_PInst_InitFB(UProgramInstance *s, char const *fb)
{
    program_cairo_wrapper_t *v = malloc(sizeof(program_cairo_wrapper_t));
    v->screen = cairo_linuxfb_surface_create(fb);

    size_t w = cairo_image_surface_get_width(v->screen);
    size_t h = cairo_image_surface_get_height(v->screen);
    size_t strd = cairo_image_surface_get_stride(v->screen);
    size_t fmt = cairo_image_surface_get_format(v->screen);

    for (size_t i = 0; i < RENDERER_NUM_MAX_BUFFER; i++)
    {
        v->backbuffer_memory[i] = malloc(h * strd);
        v->backbuffer[i] = cairo_image_surface_create_for_data(v->backbuffer_memory[i], fmt, w, h, strd);
    }

    *PInst_AspectRatio(s) = (float)w / h;

    return v;
}

void Internal_PInst_DeinitFB(struct ProgramInstance *Inst, void *hFB)
{
    program_cairo_wrapper_t *v = hFB;
    for (size_t i = 0; i < RENDERER_NUM_MAX_BUFFER; i++)
    {
        cairo_surface_destroy(v->backbuffer[i]);
        free(v->backbuffer_memory[i]);
    }
    cairo_surface_destroy(v->screen);
}

void *Internal_PInst_LoadImgInternal(struct ProgramInstance *Inst, char const *Path)
{
    // Check if file exists.
    FILE *fp = fopen(Path, "r");
    if (fp == NULL)
        return NULL;

    fclose(fp);
    return cairo_image_surface_create_from_png(Path);
}

void *Internal_PInst_LoadFont(struct ProgramInstance *Inst, char const *Path, LOADRESOURCE_FLAG_T Flag)
{
    cairo_font_slant_t slant = (Flag & LOADRESOURCE_FLAG_FONT_ITALIC) ? CAIRO_FONT_SLANT_ITALIC : CAIRO_FONT_SLANT_NORMAL;
    cairo_font_weight_t weight = (Flag & LOADRESOURCE_FLAG_FONT_BOLD) ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL;

    rsrc_font_t *fontdat = malloc(sizeof(rsrc_font_t));
    fontdat->fontFamily = Path;
    fontdat->slant = slant;
    fontdat->weight = weight;

    return fontdat;
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

static cairo_surface_t *cairo_linuxfb_surface_create(const char *fb_name)
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

void Internal_PInst_Draw(void *hFB, struct RenderEventArg const *Arg)
{
}

void Internal_PInst_Flush(void *hFB)
{
}
