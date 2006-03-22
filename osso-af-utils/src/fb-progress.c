/* 
 * Framebuffer Progressbar
 * 
 * Compile with:
 *	gcc -s -Os -W -Wall -o fb-progress fb-progress.c -lpng
 * 
 * This shows a progress bar on screen for a given time.
 * Progressbar advances the always the same amount to appear
 * smooth, but it can advance faster.
 * 
 * NOTES
 * - Framebuffer and its signal handling was originally based on
 *   the code from Torsten Scherer's W window system m68k-linux
 *   graphics initialization code
 * - PNG handling came originally from Henrik Saari
 * - The screen width has to be taken from finfo.line_length
 *   instead of vinfo.xres(_virtual) as some screens might include
 *   padding there.  Framebuffer is panned to the bottom so that
 *   the graphics are visible.
 *
 * Changelog:
 * 2005-07-11
 *   output progress bar to fb, switch VT on start & end
 *   and get progress updates from the fifo as numbers
 * 2005-07-14
 *   remove progress number handling, update only on '#'
 *   and terminate cleanly on sigTERM
 * 2005-07-22
 *   catch vt switching, add verbose option,
 *   rewrote argument handling so that user can select whether:
 *   - vt is switched,
 *   - screen is cleared at startup, and
 *   - progress bar is advanced once a sec or only
 *     when something is written to the fifo
 * 2005-09-01
 *   - Use new FB API
 *   - Options for overriding the progress & clear default colors
 *   - Fix deadlock between console switching and console switch catching
 *     (do either one of them, not both)
 *   - Removed fifo usage and made progress advance always
 *     the same amount, the time just changes
 *   - Optionally load and show an PNG (logo) image
 *   - Catch SIGINT for more pleasent testing
 * 2005-09-12
 *   - Add option for an image progress bar
 * 2005-10-04
 *   - Remove remains of "night rider" mode
 * 2005-03-22
 *   - Fix fd leak and add more debugging output
 * 
 * This is free software; you can redistribute it and/or modify it
 * under the terms specified in the GNU Public Licence (GPL).
 *
 * Copyright (C) 1998 Torsten Scherer (fb init and its signal handling)
 * Copyright (C) 2005,2006 Nokia Corporation
 * Author: Eero Tamminen
 * Contact: Kimmo Hämäläinen <kimmo.hamalainen@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <linux/vt.h>
#include <linux/kd.h>
#include <png.h>
#include "omapfb.h"	/* stuff needed by fb_flush() */

#define PROGRESS_HEIGHT_MAX 200	/* max. height for image progressbar */
#define PROGRESS_HEIGHT 20	/* height for non-image progressbar */
#define PROGRESS_ADVANCE 8	/* advance this many pixels at the time */

/* VT switch takes some time during which the progress bar cannot
 * be updated.  For N770 it's about 500ms
 */
#define VT_SWITCH_DELAY 500

/* expected framebuffer properties */
#define CONSOLE	"/dev/tty0"
#define FB_DEV	"/dev/fb0"

/* colors from 24-bit 888 RGB to 16-bit 565 RGB
 * - clears to white
 * - bar is blue Nokia logo blue
 */
#define COLOR_BG_DEFAULT  0xffffff
#define COLOR_BAR_DEFAULT 0x0040a3

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

typedef struct {
	int fd;		/* framebuffer device file descriptor */
	int wd;		/* screen width in pixels (shorts) */
	int ht;		/* visible screen height */
	void *mem;	/* memory mapped framebuffer */
	size_t size;	/* size of mapped memory in bytes */

	int dirty;	/* whether framebuffer needs updating */
	int dx1, dx2;	/* + the dirty area */
	int dy1, dy2;

	int sig;	/* when the console switched, set to signal */
} myfb_t;

typedef struct {
	int wd;
	int ht;
	uint16_t *pixel_buffer;
} image_info_t;


typedef struct {
	/* whether to switch my own console/VT */
	int use_vt;
	/* set when program should exit (due to SIGTERM etc) */
	int do_exit;
	/* clear screen before progressing */
	int do_clear;
	/* color for clearing and progressbar */
	uint16_t bg_color;
	uint16_t bar_color;
	/* be verbose */
	int verbose;
	/* logo image to show */
	image_info_t *img_logo;
	/* image to use for drawing the progress bar */
	image_info_t *img_progress;
} myoptions_t;

static myfb_t Fb;
static myoptions_t Options;

/* converts 24-bit 888 RGB value to 16-bit 565 RGB value */
static inline uint16_t rgb_888_to_565(int rgb888)
{
        return
	  (((rgb888 & 0xff0000) >> 8) & 0xf800) |
	  (((rgb888 & 0x00ff00) >> 5) & 0x07e0) |
	  (((rgb888 & 0x0000ff) >> 3) & 0x001f);
}

/* open the console device, return its file descriptor or -1 */
static int console_open(void)
{
	int fd;
	if ((fd = open(CONSOLE, O_RDWR)) < 0) {
		perror("open(" CONSOLE ")");
		return -1;
	}
	return fd;
}

/* changes to the given console
 * returns current console or -1 on error
 */
static int console_switch(int vt)
{
	struct vt_stat vtstat;
	int fd, ret;

	if (!Options.use_vt) {
		return 0;
	}
	if (Options.verbose) {
		printf("fb-progress: switching console...\n");
	}

	if ((fd = console_open()) < 0) {
		return -1;
	}
	if (ioctl(fd, VT_GETSTATE, &vtstat)) {
		perror("ioctl(VT_GETSTATE)");
		close(fd);
		return -1;
	}
	ret = vtstat.v_active;
	if (ioctl(fd, VT_ACTIVATE, vt)) {
		perror("ioctl(VT_ACTIVATE)");
		close(fd);
		return -1;
	}
	if (ioctl(fd, VT_WAITACTIVE, vt)) {
		perror("ioctl(VT_WAITACTIVATE)");
		close(fd);
		return -1;
	}
	close(fd);

	if (Options.verbose) {
		printf("fb-progress: previous console %d, switched to %d\n",
		       ret, vt);
	}
	return ret;
}

/* open console and do given ioctl operation with given value,
 * show message if verbose and name with perror.
 * return zero on success, negative on error
 */
static int console_ioctl(int op, int val, const char *name, const char *msg)
{
	int fd;

	if (Options.verbose) {
		printf("fb-progress: %s\n", msg);
	}
	if ((fd = console_open()) >= 0) {
		if (ioctl(fd, op, val) == 0) {
			close(fd);
			return 0;
		} else {
			perror(name);
		}
		close(fd);
	}
	Options.do_exit = 1;
	return -1;
}

/* vt switch signal (SIGUSR*) handler, mark which signal was delivered
 * and ignore all handled signals until this is handled
 */
static void signal_handle(int sig)
{
	struct sigaction sa;

	sa.sa_flags = 0;
	sa.sa_handler = SIG_IGN;
	sigaction(SIGUSR1, &sa, NULL);
	sigaction(SIGUSR2, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
	
	/* which signal */
	Fb.sig = sig;
}

/* set up signal handlers for termination and vtswitch
 */
static void signal_set_handlers(void)
{
	struct sigaction sa;
	sa.sa_handler = signal_handle;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;	
	sigaction(SIGUSR1, &sa, NULL);
	sigaction(SIGUSR2, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
}

/* this routine will be called at signal delivery by mainloop
 * when all graphics has been done, therefore we needn't any
 * kind of semaphore for the screen...
 * 
 * it returns true if screen needs redrawing.
 */
static int signal_process(void)
{
	/* screen does not need redraw */
	int ret = 0;

	switch(Fb.sig) {

	case SIGUSR1:
		if (console_ioctl(VT_RELDISP, 1, "VT_RELDISP", "Release VT") == 0) {
			/* switched screen OK */
			ret = 1;
		}
		break;

	case SIGUSR2:
		if (console_ioctl(VT_ACKACQ, 1, "VT_ACKACQ", "Acquire VT") == 0) {
			/* switched screen OK */
			ret = 1;
		}
		break;

	case SIGHUP:
	case SIGINT:
	case SIGTERM:
		fprintf(stderr,
			"fb-progress: Exiting on INT/TERM/HUP %d signal\n",
			Fb.sig);
		Options.do_exit = 1;
		break;
	default:
		fprintf(stderr, "fb-progress: Unknown signal %d\n", Fb.sig);
		Options.do_exit = 1;
		break;
	}
	/* no signal */
	Fb.sig = 0;

	/* signals ignored until now, set handler back */
	signal_set_handlers();
	
	return ret;
}

/* init vt switch catchup
 * return 0 on success, -1 on error
 */
static int signal_init(void)
{
	struct vt_mode vt;
	int fd;

	if (Options.verbose) {
		printf("fb-progress: Catch signals\n");
	}
	signal_set_handlers();

	if (Options.use_vt) {
		/* if I'm switching VT, don't setup VT switch catching */
		return 0;
	}

	if ((fd = console_open()) < 0) {
		return -1;
	}
	if (Options.verbose) {
		printf("fb-progress: Catch VT switches\n");
	}
	/* catch vty switches
	 */
	vt.mode = VT_PROCESS;	/* take control */
	vt.waitv = 0;		/* do not hang writes when not active */
	vt.relsig = SIGUSR1;	/* signal to send on release request */
	vt.acqsig = SIGUSR2;	/* signal to send on acquisition notice */
	vt.frsig = SIGUSR1;	/* signal to use to force VT switch */
	if (ioctl(fd, VT_SETMODE, &vt)) {
		perror("ioctl(VT_SETMODE)");
		return -1;
	}
	close(fd);
	return 0;
}

/* initialize framebuffer and return struct to it or NULL for error
 */
static myfb_t* fb_init(void)
{
	struct fb_var_screeninfo vinfo;
	struct fb_fix_screeninfo finfo;
	
	if ((Fb.fd = open(FB_DEV, O_RDWR)) < 0) {
		perror("open(" FB_DEV ")");
		return NULL;
	}
	if (ioctl(Fb.fd, FBIOGET_FSCREENINFO, &finfo)) {
		perror("ioctl(FBIOGET_FSCREENINFO)");
		close(Fb.fd);
		return NULL;
	}

	if (ioctl(Fb.fd, FBIOGET_VSCREENINFO, &vinfo)){
		perror("ioctl(FBIOGET_VSCREENINFO)");
		close(Fb.fd);
		return NULL;
	}
	if (Options.verbose) {
		printf("fb-progress: using %ix%i of %ix%i pixels,\n"
		       "\t%i bits per pixel, with line_len %i\n",
		       vinfo.xres, vinfo.yres,
		       vinfo.xres_virtual, vinfo.yres_virtual,
		       vinfo.bits_per_pixel,
		       finfo.line_length);
	}
	Fb.wd = finfo.line_length / sizeof(uint16_t);
	Fb.size = finfo.line_length * vinfo.yres;
	Fb.wd = vinfo.xres;
	Fb.ht = vinfo.yres;

	Fb.mem = mmap(0, Fb.size, PROT_READ | PROT_WRITE, MAP_SHARED, Fb.fd, 0);
	if (Fb.mem < NULL) {
		perror("mmap(" FB_DEV ")");
		close(Fb.fd);
		return NULL;
	}
	if (Options.verbose) {
		printf("fb-progress: mapped %ik videoram to %p\n",
		       Fb.size >> 10, Fb.mem);
	}

	/* pan to start of fb */
	vinfo.xoffset = 0;
	vinfo.yoffset = 0;
	if (ioctl(Fb.fd, FBIOPAN_DISPLAY, &vinfo)) {
		perror("ioctl(FBIOPAN_DISPLAY)");
		close(Fb.fd);
		return NULL;
	}
	Fb.dirty = 0;
	return &Fb;
}

static void fb_dirty(int x1, int y1, int x2, int y2)
{
	if (x1 < 0  || y1 < 0 || x2 < x1 || y2 < y1 ||
	    x2 > Fb.wd || y2 > Fb.ht) {
		fprintf(stderr,
			"Error: invalid flush region (%d,%d),(%d,%d)\n",
			x1, y1, x2, y2);
		return;
	}
	if (!Fb.dirty) {
		Fb.dx1 = x1;
		Fb.dy1 = y1;
		Fb.dx2 = x2;
		Fb.dy2 = y2;
		Fb.dirty = 1;
		return;
	}
	if (x1 < Fb.dx1) {
		Fb.dx1 = x1;
	}
	if (y1 < Fb.dy1) {
		Fb.dy1 = y1;
	}
	if (x2 > Fb.dx2) {
		Fb.dx2 = x2;
	}
	if (y2 > Fb.dy2) {
		Fb.dy2 = y2;
	}
}

static void fb_flush(void)
{
	struct fb_update_window update;

	if (Fb.mem && Fb.dirty) {
		update.x = Fb.dx1;
		update.y = Fb.dy1;
		update.width = Fb.dx2 - Fb.dx1;
		update.height = Fb.dy2 - Fb.dy1;
		update.format = FB_COLOR_RGB565;
		if (Options.verbose) {
			printf("fb-progress: update %dx%d+%d+%d\n",
			       update.width, update.height,
			       update.x, update.y);
		}
		if (ioctl(Fb.fd, OMAPFB_UPDATE_WINDOW, &update) < 0) {
			perror("ioctl(OMAPFB_UPDATE_WINDOW)");
		}
		Fb.dirty = 0;
	}
}

static void fb_exit(void)
{
	if (Fb.fd) {
		if (Options.verbose) {
			printf("fb-progress: close fb\n");
		}
		/* close frame buffer */
		munmap(Fb.mem, Fb.size);
		close(Fb.fd);
		Fb.mem = NULL;
	}
}

/* draw progress bar between steps 'start' and 'end' when the
 * maximum number of steps is 'max'. return negative on error
 */
static int draw_steps(int start, int end, int max)
{
	int x, w, h, off;
	uint16_t *dst, color;

	if (end < start) {
		fprintf(stderr, "ERROR: end step (%d) is smaller than start (%d)\n", end, start);
		return -1;
	}
	if (end > max) {
		fprintf(stderr, "ERROR: end step (%d) is larger than max step (%d)\n", end, max);
		return -1;
	}
	/* convert step indeces to co-ordinates */
	start = start * Fb.wd / max;
	end = end * Fb.wd / max;
	w = end - start;
	h = PROGRESS_HEIGHT;

	/* destination offset is calculated from the screen bottom */
	dst = (uint16_t *)(Fb.mem + Fb.size) - Fb.wd * h + start;
	off = Fb.wd - w;

	color = Options.bar_color;
	while (--h >= 0) {

		x = w;
		while (--x >= 0) {
			*dst++ = color;
		}
		dst += off;
	}

	fb_dirty(start, Fb.ht - PROGRESS_HEIGHT, end, Fb.ht);
	return 0;
}
/* draw progress bar between steps 'start' and 'end' when the
 * maximum number of steps is 'max'. return negative on error
 */
static int draw_steps_with_image(int start, int end, int max)
{
	int w, h, off_src, off_dst, step, flush_x1, flush_x2;
	uint16_t *src, *dst, *line_start;
	image_info_t *image = Options.img_progress;

	if (end < start) {
		fprintf(stderr, "ERROR: end step (%d) is smaller than start (%d)\n", end, start);
		return -1;
	}
	if (end > max || end * PROGRESS_ADVANCE > Fb.wd) {
		fprintf(stderr, "ERROR: end step (%d) is larger than max step (%d)\n", end, max);
		return -1;
	}
	if (image->wd != 3*PROGRESS_ADVANCE) {
		fprintf(stderr, "ERROR: progress image width != %d (3*%d)\n",
			3*PROGRESS_ADVANCE, PROGRESS_ADVANCE);
		return -1;
	}

	/* area to flush */
	flush_x1 = start * PROGRESS_ADVANCE;
	flush_x2 = end * PROGRESS_ADVANCE;
	
	/* destination offset is calculated from the screen bottom */
	line_start = (uint16_t *)(Fb.mem + Fb.size) - Fb.wd * image->ht;

	/* all drawing is done one PROGRESS_ADVANCE sized block at the time */
	off_src = image->wd - PROGRESS_ADVANCE;
	off_dst = Fb.wd - PROGRESS_ADVANCE;

	if (!start) {
		/* draw left end */
		dst = line_start;
		src = image->pixel_buffer;

		for (h = 0; h < image->ht; h++) {
			for (w = 0; w < PROGRESS_ADVANCE; w++) {
				*dst++ = *src++;
			}
			dst += off_dst;
			src += off_src;
		}
		start++;
	} else {
		if (start > 1) {
			/* overdraw last time end with middle tile */
			start--;
			flush_x1 -= PROGRESS_ADVANCE;
		}
	}
	/* middle tile doesn't cover right end */
	end--;
	
	/* tile middle part */
	line_start += start * PROGRESS_ADVANCE;
        for (step = start; step < end; step++) {
		dst = line_start;
		/* middle part is after left part */
		src = image->pixel_buffer + PROGRESS_ADVANCE;
		
		for (h = 0; h < image->ht; h++) {
			for (w = 0; w < PROGRESS_ADVANCE; w++) {
				*dst++ = *src++;
			}
			dst += off_dst;
			src += off_src;
		}
		line_start += PROGRESS_ADVANCE;
	}

	/* draw right end */
	dst = line_start;
	src = image->pixel_buffer + 2*PROGRESS_ADVANCE;
	for (h = 0; h < image->ht; h++) {
		for (w = 0; w < PROGRESS_ADVANCE; w++) {
			*dst++ = *src++;
		}
		dst += off_dst;
		src += off_src;
	}

	fb_dirty(flush_x1, Fb.ht - image->ht, flush_x2, Fb.ht);
	return 0;
}

static void fb_clear_with_image(void)
{
	image_info_t image;
	if (Options.img_logo) {
		image = *(Options.img_logo);
	} else {
		/* skip the image */
		image.wd = image.ht = 0;
		image.pixel_buffer = NULL;
	}
	
	/* image too large */
	if (Fb.wd < image.wd || Fb.ht < image.ht) {
		fprintf(stderr, "image too large for screen, skipping\n");
		return;
	}
	/* image and screen same size */
	if (Fb.wd == image.wd && Fb.ht == image.ht) {
		memcpy(Fb.mem, image.pixel_buffer, Fb.size);
		return;
	}

	/* Image smaller than screen.
	 * Drawing image to center of screen.
	 */
	uint16_t *out = Fb.mem;
	uint16_t *in = image.pixel_buffer;
	int start_x = (Fb.wd - image.wd) / 2;
	int start_y = (Fb.ht - image.ht) / 2;
	uint16_t bg_color = Options.bg_color;
	int i,j;
	
	/* put some color above the image */
	if (Options.do_clear) {
		for (j = 0; j < start_y; j++) {
			for (i = 0; i < Fb.wd; i++) {
				*out++ = bg_color;
			}
		}
	} else {
		out += start_y * Fb.wd;
	}
	
	/* now image to the center and bg_color to both sides */
	for (j = 0; j < image.ht; j++) {
		if (Options.do_clear) {
			for (i = 0; i < start_x; i++) {
				*out++ = bg_color;
			}
		} else {
			out += start_x;
		}
		for (i = 0; i < image.wd; i++) {
			*out++ = *in++;
		}
		if (Options.do_clear) {
			for (i = start_x + image.wd; i < Fb.wd; i++) {
				*out++ = bg_color;
			}
		} else {
			out += (Fb.wd - image.wd - start_x);
		}
	}
		
	/* and some color under the image */
	if (Options.do_clear) {
		for (j = 0; j < (Fb.ht - image.ht - start_y); j++) {
			for (i = 0; i < Fb.wd; i++) {
				*out++ = bg_color;
			}
		}
		fb_dirty(0, 0, Fb.wd, Fb.ht);
	} else {
		fb_dirty(start_x, start_y,
			 start_x + image.wd, start_y + image.ht);
	}
	return;
}

static image_info_t *decompress_png(const char *filename)
{
	FILE *fp;
	char  header[8];
	image_info_t image, *ret;
	
	fp = fopen(filename, "rb");
	
	if (!fp){
		perror("PNG file open");
		return NULL;
	}
	
	fread(header, 1, 8, fp);
	int is_png = !png_sig_cmp(header, 0, 8);
	if (!is_png) {
		fprintf(stderr, "image is not a PNG!\n");
		return NULL;
	}
	
	png_structp png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, 
						      NULL, NULL, NULL);
	if (!png_ptr) {
		fprintf(stderr, "could not create png_ptr!\n");
		return NULL;
	}
	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		return NULL;
	}
	png_infop end_info = png_create_info_struct(png_ptr);
	if (!end_info) {
		png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
		return NULL;
	}
	
	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, 8);
	
	png_uint_32 wd, ht;
	int bit_depth, color_type;

	png_read_info(png_ptr, info_ptr);  /* read all PNG info up to image data */
	png_get_IHDR(png_ptr, info_ptr, &wd, &ht, &bit_depth, &color_type,
		     NULL, NULL, NULL);

	image.wd = wd;
	image.ht = ht;
	
	/* alpha channel not supported. At least not yet... */
	if (color_type & PNG_COLOR_MASK_ALPHA)
		png_set_strip_alpha(png_ptr);
	
	/* transfer image to RGB if not already */
	if (color_type != PNG_COLOR_TYPE_RGB)
		png_set_expand(png_ptr); 

	/* we are pleased with 8 bit per pixel */
	if (bit_depth == 16) 
		png_set_strip_16(png_ptr);
	
	png_read_update_info(png_ptr, info_ptr);
	png_uint_32 rowbytes = png_get_rowbytes(png_ptr, info_ptr);
	
	uint8_t * image_data = malloc(rowbytes * image.ht);
	if(!image_data) {
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
		return NULL;
	}
	
	png_bytepp row_pointers = malloc(image.ht * sizeof(png_bytep));
	if(!row_pointers) {
		free(image_data);
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
		return NULL;
	}
	
	/* set the individual row_pointers to point at the correct offsets */
	int i,j;
	for (i = 0;  i < image.ht; i++)
		row_pointers[i] = image_data + i * rowbytes;
	
	png_read_image(png_ptr, row_pointers);
	
	image.pixel_buffer = malloc(image.wd * image.ht *  sizeof(uint16_t));
	if (!image.pixel_buffer) {
		fprintf(stderr, "Not enought memory\n");
		free(image_data);
                png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
		return NULL;
	}
	      
	int k=0;
	for (j = 0; j < image.ht; j++) {
		uint8_t *picture = row_pointers[j];
		uint16_t red, green, blue;
		for (i = 0; i < image.wd; i++) {
			red = (*picture++ >> 3);
			green = (*picture++ >> 2);
			blue = (*picture++ >> 3);
			
			*(image.pixel_buffer + k) = red << 11 | green << 5 | blue;
			k++;
		}
	}
	
	/* let's free some memory */
	png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
	free(row_pointers);
	free(image_data);
	
	ret = malloc (sizeof(image_info_t));
	*ret = image;
	return ret;
}

static void free_image(image_info_t *img)
{
	if (img) {
		free(img->pixel_buffer);
		free(img);
	}
}

/* show error message, usage, and exit */
static void usage(const char *name, const char *error)
{
	fprintf(stderr, "\nERROR: %s!\n\n", error);
	printf("Usage: %s [options] <secs>\n\n"
	       "A progress bar for the the framebuffer, show at the bottom.\n"
	       "Options:\n"
	       "-v\t\tverbose\n"
	       "-c\t\tclear screen at startup\n"
	       "-b <color>\tcolor to use for the clearing, as 24-bit hex value\n"
	       "-p <color>\tcolor to use for the progressbar, as 24-bit hex value\n"
	       "-l <image>\tlogo PNG image to show on the background\n"
	       "-g <image>\tgraphics PNG image to use for drawing the progressbar\n"
	       "-t <vt>\t\tswitch to given virtual terminal while showing the progress\n"
	       "-i <step>\tinitial seconds (< all secs)\n\n"
	       "Examples:\n"
	       "\t%s -s -c -t 3 -b ffffff -l logo.png 30\n"
	       "\tsleep 30\n"
	       "\t%s -n -i 1 3\n\n"
	       "NOTE: this program need to be run as root (for VT ioctls)!\n",
	       name, name, name);
	exit(1);
}

int main(int argc, const char *argv[])
{
	int vt = 0, i, color, steps, step = 0, oldstep = 0, secs;
	struct timespec sleep_req, sleep_rem;
	myfb_t *fb;

	/* default options */
	Options.bg_color = rgb_888_to_565(COLOR_BG_DEFAULT);
	Options.bar_color = rgb_888_to_565(COLOR_BAR_DEFAULT);
	
	/* ---------- parse args ----------- */
	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-') {
			/* not an option */
			break;
		}
		if (argv[i][2]) {
			/* not a single letter option */
			usage(*argv, "Unknown option");
		}
		switch(argv[i][1]) {
		case 'b':
			if (++i >= argc) {
				usage(*argv, "color hex value missing");
			}
			color = strtol(argv[i], NULL, 16);
			Options.bg_color = rgb_888_to_565(color);
			break;
		case 'p':
			if (++i >= argc) {
				usage(*argv, "color hex value missing");
			}
			color = strtol(argv[i], NULL, 16);
			Options.bar_color = rgb_888_to_565(color);
			break;
		case 'c':
			Options.do_clear = 1;
			break;
		case 'i':
			if (++i >= argc) {
				usage(*argv, "-i <initial step> argument missing");
			}
			step = atoi(argv[i]);
			/* error checking done later */
			break;
		case 'l':
			if (++i >= argc) {
				usage(*argv, "-l <logo image> image file name missing");
			}
			Options.img_logo = decompress_png(argv[i]);
			if (!Options.img_logo) {
				usage(*argv, "logo image loading failed");
			}
			break;
		case 'g':
			if (++i >= argc) {
				usage(*argv, "-l <progressbar image> image file name missing");
			}
			Options.img_progress = decompress_png(argv[i]);
			if (!Options.img_progress) {
				usage(*argv, "progress graphics loading failed");
			}
			if (Options.img_progress->ht > PROGRESS_HEIGHT_MAX) {
				usage(*argv, "progress image too high");
			}
			if (Options.img_progress->wd != 3*PROGRESS_ADVANCE) {
				usage(*argv, "progress image width not 3*8");
			}
			break;
		case 'v':
			Options.verbose = 1;
			break;
		case 't':
			if (++i >= argc) {
				usage(*argv, "-v <vt> argument missing");
			}
			vt = atoi(argv[i]);
			if (vt < 1 || vt > 10) {
				usage(*argv, "Illegal console/VT number");
			}
			Options.use_vt = 1;
			break;
		default:
			usage(*argv, "Unknown option");
			break;
		}
	}
	if (i+1 != argc) {
		usage(*argv, "Number of steps missing");
	}
	steps = atoi(argv[i]);
	if (steps < 2 || steps > 255) {
		usage(*argv, "Invalid number of steps (should be 2-255)");
	}
	if (step < 0 || step >= steps-1) {
		usage(*argv, "Invalid number of initial steps");
	}
	
	/* --------- setup signals + framebuffer ----------- */

	/* just react to others VT switches */
	if (signal_init() < 0) {
		return -1;
	}
	/* or switch to another VT */
	vt = console_switch(vt);
	if (vt < 0) {
		return -1;
	}
	if (!(fb = fb_init())) {
		console_switch(vt);
		return -1;
	}
	if (fb->ht < PROGRESS_HEIGHT_MAX) {
		fprintf(stderr, "Error: progress bar max hight higher (%d) than screen (%d)\n", PROGRESS_HEIGHT_MAX, fb->ht);
		fb_exit();
		console_switch(vt);
		return -1;
	}
	
	if (Options.img_logo &&
	    (Options.img_logo->ht > fb->ht ||
	     Options.img_logo->wd > fb->wd)) {
		fb_exit();
		console_switch(vt);
		fprintf(stderr,
			"PNG picture is bigger than screen. Please provide\n"
		       "image that is same size or smaller that the screen.\n"
		       "Screen resolution is: %dx%d\n", fb->wd, fb->ht);
		return -1;
	}
	if (Options.do_clear || Options.img_logo) {
		fb_clear_with_image();
		fb_flush();
	}
	
	/* re-calculate steps to correspond to seconds */
	secs = steps;
	steps = fb->wd / PROGRESS_ADVANCE;
	step = step * fb->wd / secs / PROGRESS_ADVANCE;
	if (step < 2) {
		/* image progress will be always at least two steps */
		step = 2;
	}
	/* calculate advancing time interval */
	sleep_req.tv_sec = secs / steps;
	sleep_req.tv_nsec = (long long)(secs % steps) * 1000000000L / steps;
	
	if (Options.img_progress) {
		draw_steps_with_image(oldstep, step, steps);
	} else {
		draw_steps(oldstep, step, steps);
	}
	fb_flush();

	if (Options.verbose) {
		printf("fb-progress: time interval %ldms for %d steps\n",
		       sleep_req.tv_sec * 1000 + sleep_req.tv_nsec / 1000000,
		       steps);
	}

	/* ------ show progress and redraw logo -------- */
	
	while (step < steps && !Options.do_exit) {

		/* just advance the progress bar */
		sleep_rem.tv_sec = sleep_rem.tv_nsec = 0;
		nanosleep(&sleep_req, &sleep_rem);
		step++;

		if (Fb.sig) {
			if (signal_process()) {
				struct timespec sleep_x;
				
				/* need to redraw stuff on screen and
				 * wait a while so that the screen
				 * switcher has had time to actually switch
				 * the VT and draw its own stuff
				 */
				sleep_x.tv_sec = 0;
				sleep_x.tv_nsec = VT_SWITCH_DELAY*1000000L;
				nanosleep(&sleep_x, &sleep_rem);
				if (Options.do_clear || Options.img_logo) {
					fb_clear_with_image();
					fb_flush();
				}
				oldstep = 0;
				/* just in case we get more signals */
				continue;
			}
		}
		if (Options.img_progress) {
			if (draw_steps_with_image(oldstep, step, steps) < 0) {
				break;
			}
		} else {
			if (draw_steps(oldstep, step, steps) < 0) {
				break;
			}
		}
		fb_flush();

		if (Options.verbose) {
			printf("fb-progress: oldstep %d, newstep %d\n",
			       oldstep, step);
		}
		oldstep = step;
	}
	fb_exit();
	console_switch(vt);
	free_image(Options.img_logo);
	free_image(Options.img_progress);
	return 0;
}
