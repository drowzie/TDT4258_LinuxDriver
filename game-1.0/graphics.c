#define screen_height 240
#define screen_width 320
#define pixel_bits 16
#define screen_size_bytes 153600
#define blue 5
#define red 5
#define green 6

#include <linux/fb.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <fcntl.h>

// Framebuffer device
int fbfd;
// Framebuffer pointer
static uint16_t *fbp;

// setup which part of the framebuffer that is to be refreshed
// for performance reasons, use as small rectangle as possible
struct fb_copyarea rect;

// Struct for variable screen info
struct var_screen vinfo;
// Struct for fixed screen info
struct fix_screen finfo;

void init_framebuffer()
{
  // Start pos?
  rect.dx = 0;
  rect.dy = 0;
  // Size of block
  rect.width = screen_width;
  rect.height = screen_height;



  // Open device with Read and Write access
  fbfd = open("/dev/fb0", O_RDWR);

  // Get variable screen info
  ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)

  // Get fixed screen info
  ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)

  // Makes the framebufferpointer with correct settings and save in memory wherever
  fbp = (uint16_t *)mmap(NULL, screen_size_bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
}

void refresh_screen()
{
  // Updates screen from memory map
  ioctl(fbfd, 0x4680, &rect);
}

void draw_square(int pos_x, int pos_y){

  long int location;

  for y = pos_y; y < pos_x; y++
  {
    for x = pos_y; x < pos_y; x++
    {
      // Set location
      location = (x + vinfo.xoffset) * (pixel_bits/8) +
                 (y + vinfo.yoffset) * finfo.line_length;

      int b = 10;
      int g = (100)/6;
      int r = 31-(100)/16;
      unsigned short int t = r << 11 | g << 5 | b;
      *((unsigned short int*)(fbp + location)) = t;
    }            
  }
  refresh_screen();
}

void draw_struct(struct object)
{
  long int location;
  
  for (y = object.pos_y; y < object.height; y++)
  {
    for (x = object.pos_x; x < object.width; x++)
    {
      // Set location
      location = (x + vinfo.xoffset) * (pixel_bits/8) +
                 (y + vinfo.yoffset) * finfo.line_length;

      // Sets color
      int b = 10;
      int g = (100)/6;
      int r = 31-(100)/16;

      unsigned short int t = r << 11 | g << 5 | b;
      *((unsigned short int*)(fbp + location)) = t;
    }            
  }
  refresh_screen();
}