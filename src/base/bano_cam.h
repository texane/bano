#ifndef BANO_CAM_H_INCLUDED
#define BANO_CAM_H_INCLUDED


#include <stdint.h>
#include <sys/types.h>
#include <linux/videodev2.h>
#include "bano_bmp.h"


typedef enum
{
  BANO_CAM_FMT_YUV420_160_120 = 0,
  BANO_CAM_FMT_YUV420_320_240,
  BANO_CAM_FMT_YUV420_640_480,

  BANO_CAM_FMT_YUYV_160_120,
  BANO_CAM_FMT_YUYV_320_240,
  BANO_CAM_FMT_YUYV_640_480,

  BANO_CAM_FMT_INVALID

} bano_cam_fmt_t;


typedef struct bano_cam_info
{
#define BANO_CAM_FLAG_ID (1 << 0)
#define BANO_CAM_FLAG_FMT (1 << 1)
#define BANO_CAM_FLAG_DUMMY (1 << 2)
  uint32_t flags;
  unsigned int id;
  bano_cam_fmt_t fmt;
} bano_cam_info_t;


typedef struct bano_cam_handle
{
  int fd;
  bano_cam_fmt_t fmt;
  int is_mmap_enabled;
  int is_dummy;

  /* frame buffers */
#define BANO_CAM_FB_COUNT 2
  void* fb_data[BANO_CAM_FB_COUNT];
  size_t fb_size[BANO_CAM_FB_COUNT];
  struct v4l2_buffer fb_v4l2[BANO_CAM_FB_COUNT];
  size_t fb_count;

  /* output bmp file */
  bano_bmp_handle_t bmp;

#ifdef BANO_CONFIG_PNG
  /* output png file */
  uint8_t* png_buf;
  size_t png_size;
#endif /* BANO_CONFIG_PNG */

} bano_cam_handle_t;


int bano_cam_open(bano_cam_handle_t*, const bano_cam_info_t*);
int bano_cam_close(bano_cam_handle_t*);
int bano_cam_capture(bano_cam_handle_t*);
void bano_cam_get_image(bano_cam_handle_t*, const uint8_t**, size_t*, const char**);


#endif /* BANO_CAM_H_INCLUDED */
