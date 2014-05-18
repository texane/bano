#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <linux/videodev2.h>
#include "bano_perror.h"
#include "bano_cam.h"
#include "bano_bmp.h"


/* transform routines */

static uint8_t saturate(int x)
{
  if (x < 0) x = 0;
  else if (x > 255) x = 255;
  return x;
}

static void yuv_to_rgb
(uint8_t* rgb, uint8_t y, uint8_t cr, uint8_t cb)
{
  int n;

  n = (int)y + (int)(1.402 * (double)((int)cr - 128));
  rgb[0] = saturate(n);

  n =
    (int)y -
    0.34414 * (double)((int)cb - 128) -
    0.71414 * (double)((int)cr - 128);
  rgb[1] = saturate(n);

  n = (int)y + 1.772 * (double)((int)cb - 128);
  rgb[2] = saturate(n);
}


static void trans_yuv420_to_rgb24
(uint8_t* out_data, const uint8_t* in_data, uint32_t w, uint32_t h)
{
  uint32_t l;
  uint32_t m;

  const int8_t* y_plane;
  const int8_t* cr_plane;
  const int8_t* cb_plane;

  int8_t cb;
  int8_t cr;

  /* planes */

  y_plane = (const int8_t*)in_data;
  cr_plane = y_plane + w * h;
  cb_plane = cr_plane + (w * h) / 4;

  /* save bytes per y line */

  m = w;

  /* bytes per rgb line */

  l = w * 3;

  /* 2 per 2 blocks */

  h /= 2;

  while (h--)
  {
    w = m / 2;

    while (w--)
    {
      cr = *cr_plane;
      cb = *cb_plane;

      yuv_to_rgb(out_data, y_plane[0], cr, cb);
      yuv_to_rgb(out_data + l, y_plane[m], cr, cb);
      out_data += 3;
      y_plane += 1;

      yuv_to_rgb(out_data, y_plane[0], cr, cb);
      yuv_to_rgb(out_data + l, y_plane[m], cr, cb);
      out_data += 3;
      y_plane += 1;

      cr_plane += 1;
      cb_plane += 1;
    }

    /* skip line */

    y_plane += m;

    out_data += l;
  }
}

static void trans_yuyv_to_rgb24
(uint8_t* out_data, const uint8_t* in_data, uint32_t w, uint32_t h)
{
  /* y0 u0 y1 v0 */

  const uint8_t* p = in_data;

  uint32_t i;
  uint32_t j;

  j = h;

  while (j)
  {
    i = w;

    while (i)
    {
      yuv_to_rgb(out_data, p[0], p[1], p[3]);
      out_data += 3;

      yuv_to_rgb(out_data, p[2], p[1], p[3]);
      out_data += 3;

      p += 4;

      i -= 2;
    }

    --j;
  }
}

static void swap_lines(uint8_t* a, uint8_t* b, uint32_t w)
{
/* vertical flip */

  uint32_t tmp;
  uint32_t n;

  n = w & (sizeof(uint32_t) - 1);

  w /= sizeof(uint32_t);

  while (w--)
  {
    tmp = *(uint32_t*)a;
    *(uint32_t*)a = *(uint32_t*)b;
    *(uint32_t*)b = tmp;

    a += sizeof(uint32_t);
    b += sizeof(uint32_t);
  }

  switch (n)
  {
  case 3:
    tmp = *a;
    *a = *b;
    *b = (uint8_t)tmp;
    ++a;
    ++b;

    /* passthru
     */

  case 2:
    tmp = *(uint16_t*)a;
    *(uint16_t*)a = *(uint16_t*)b;
    *(uint16_t*)b = (uint16_t)tmp;
    break;

  default:
  case 1:
    tmp = *a;
    *a = *b;
    *b = (uint8_t)tmp;
    break;
  }
}

static void trans_vflip_rgb24(uint8_t* data, uint32_t w, uint32_t h)
{
  uint32_t j;
  uint32_t k;
  uint32_t l;

  l = w * 3;

  j = 0;
  k = (h - 1) * l;

  while (j < k)
  {
    swap_lines(data + j, data + k, l);
    j += l;
    k -= l;
  }
}

static void trans_frame
(bano_cam_handle_t* cam, const uint8_t* data, size_t size)
{
  bano_bmp_handle_t* const bmp = &cam->bmp;

  switch (cam->fmt)
  {
  case BANO_CAM_FMT_YUV420_160_120:
  case BANO_CAM_FMT_YUV420_320_240:
  case BANO_CAM_FMT_YUV420_640_480:
    trans_yuv420_to_rgb24(bmp->data_buf, data, bmp->width, bmp->height);
    break;

  case BANO_CAM_FMT_YUYV_160_120:
  case BANO_CAM_FMT_YUYV_320_240:
  case BANO_CAM_FMT_YUYV_640_480:
    trans_yuyv_to_rgb24(bmp->data_buf, data, bmp->width, bmp->height);
    break ;

  default:
    break ;
  }

  trans_vflip_rgb24(bmp->data_buf, bmp->width, bmp->height);
}


/* v4l2 routines */

static int set_format(int fd, uint32_t fourcc, uint32_t width, uint32_t height)
{
  struct v4l2_format format;

  format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  if (ioctl(fd, VIDIOC_G_FMT, &format) == -1)
  {
    BANO_PERROR();
    return -1;
  }

  format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  format.fmt.pix.width = width;
  format.fmt.pix.height = height;
  format.fmt.pix.pixelformat = fourcc;

  if (ioctl(fd, VIDIOC_S_FMT, &format) == -1)
  {
    BANO_PERROR();
    return -1;
  }

  return 0;
}


static int expand_format
(bano_cam_fmt_t format, uint32_t* fourcc, uint32_t* width, uint32_t* height)
{
  int res;

#define EXPAND_FORMAT_CASE(c, w, h)		\
 case BANO_CAM_FMT_ ## c ## _ ## w ## _ ## h:	\
   *fourcc = V4L2_PIX_FMT_ ## c;		\
   *width = w;					\
   *height = h;					\
   break

  res = 0;

  switch (format)
  {
    EXPAND_FORMAT_CASE(YUV420, 160, 120);
    EXPAND_FORMAT_CASE(YUV420, 320, 240);
    EXPAND_FORMAT_CASE(YUV420, 640, 480);

    EXPAND_FORMAT_CASE(YUYV, 160, 120);
    EXPAND_FORMAT_CASE(YUYV, 320, 240);
    EXPAND_FORMAT_CASE(YUYV, 640, 480);

  default:
    res = -1;
    break;
  }

  return res;
}


static uint32_t format_to_frame_size(bano_cam_fmt_t format)
{
  uint32_t c;
  uint32_t w;
  uint32_t h;
  uint32_t n;

  if (expand_format(format, &c, &w, &h)) return 0;

  switch (c)
  {
  case V4L2_PIX_FMT_YUV420:
    n = w * h + (w * h) / 2;
    break;

  case V4L2_PIX_FMT_YUYV:
    /* bits per pixel = 16 */
    n = w * h * 2;
    break;

  default:
    /* unknown format */
    n = 0;
    break;
  }

  return n;
}


/* mmap disabled capture loop */

static int start_mmap_disabled(bano_cam_handle_t* cam)
{
  int err = -1;
  int flags;

  if (!(cam->fb_size[0] = format_to_frame_size(cam->fmt)))
  {
    BANO_PERROR();
    goto on_error;
  }

  if ((cam->fb_data[0] = malloc(cam->fb_size[0])) == NULL)
  {
    BANO_PERROR();
    goto on_error;
  }

  cam->fb_count = 1;

  if (!((flags = fcntl(cam->fd, F_GETFL)) & O_NONBLOCK))
    fcntl(cam->fd, flags | O_NONBLOCK);

  err = 0;

 on_error:
  return err;
}

static int stop_mmap_disabled(bano_cam_handle_t* cam)
{
  if (cam->fb_count == 0) return 0;

  free(cam->fb_data[0]);
  cam->fb_count = 0;
  return 0;
}

static int capture_mmap_disabled(bano_cam_handle_t* cam)
{
  int err = -1;
  ssize_t nread;
  fd_set rds;
  int n;

  FD_ZERO(&rds);
  FD_SET(cam->fd, &rds);

  n = select(cam->fd + 1, &rds, NULL, NULL, NULL);
  if (n == -1)
  {
    if (errno != EINTR)
    {
      BANO_PERROR();
      goto on_error;
    }
  }
  else if (n == 1)
  {
    nread = read(cam->fd, cam->fb_data[0], cam->fb_size[0]);
    if (nread == -1)
    {
      BANO_PERROR();
      goto on_error;
    }
    else if (nread > 0)
    {
      trans_frame(cam, cam->fb_data[0], (size_t)nread);
    }
  }

  /* success */
  err = 0;

 on_error:
  return err;
}

static int map_frame_buffer(bano_cam_handle_t* cam, size_t i)
{
  cam->fb_data[i] = MAP_FAILED;
  cam->fb_size[i] = 0;

  memset(&cam->fb_v4l2[i], 0, sizeof(struct v4l2_buffer));
  cam->fb_v4l2[i].index = i;
  cam->fb_v4l2[i].type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  cam->fb_v4l2[i].memory = V4L2_MEMORY_MMAP;

  if (ioctl(cam->fd, VIDIOC_QUERYBUF, &cam->fb_v4l2[i]))
  {
    BANO_PERROR();
    return -1;
  }

  cam->fb_data[i] = mmap
  (
   NULL,
   cam->fb_v4l2[i].length,
   PROT_READ | PROT_WRITE,
   MAP_SHARED,
   cam->fd,
   0
  );

  if (cam->fb_data[i] == MAP_FAILED)
  {
    BANO_PERROR();
    return -1;
  }

  cam->fb_size[i] = cam->fb_v4l2[i].length;

  return 0;
}


static void unmap_frame_buffer(bano_cam_handle_t* cam, size_t i)
{
  if (cam->fb_data[i] == MAP_FAILED) return ;
  munmap(cam->fb_data[i], cam->fb_size[i]);
  cam->fb_data[i] = MAP_FAILED;
  cam->fb_size[i] = 0;
}


static int start_mmap_enabled(bano_cam_handle_t* cam)
{
  static const enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  struct v4l2_requestbuffers req;
  size_t i;
  int flags;

  /* request buffers */

  memset(&req, 0, sizeof(req));
  req.count = 1;
  req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_MMAP;

  if (ioctl(cam->fd, VIDIOC_REQBUFS, &req) == -1)
  {
    BANO_PERROR();
    goto on_error;
  }

  if ((req.count < 1) || (req.count > BANO_CAM_FB_COUNT))
  {
    BANO_PERROR();
    goto on_error;
  }

  /* alloc frame buffer */

  for (i = 0; i < req.count; ++i)
  {
    if (map_frame_buffer(cam, i))
    {
      BANO_PERROR();
      goto on_error;
    }

    ++cam->fb_count;

    /* queue buffer */

    if (ioctl(cam->fd, VIDIOC_QBUF, &cam->fb_v4l2[i]))
    {
      BANO_PERROR();
      goto on_error;
    }
  }

  /* start streaming */

  if (ioctl(cam->fd, VIDIOC_STREAMON, &type))
  {
    BANO_PERROR();
    goto on_error;
  }

  if (!((flags = fcntl(cam->fd, F_GETFL)) & O_NONBLOCK))
    fcntl(cam->fd, flags | O_NONBLOCK);

  return 0;

 on_error:
  return -1;
}


static int stop_mmap_enabled(bano_cam_handle_t* cam)
{
  static const enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  size_t i;

  for (i = 0; i < cam->fb_count; ++i) unmap_frame_buffer(cam, i);
  cam->fb_count = 0;

  ioctl(cam->fd, VIDIOC_STREAMOFF, &type);

  return 0;
}


static int capture_mmap_enabled(bano_cam_handle_t* cam)
{
  struct v4l2_buffer buffer;
  unsigned int bug_count = 0;

 redo_select:
  {
    /* this fix is required due to some hardware */
    /* then driver related bug that makes ioctl */
    /* blocks forever. restart stream on timeout. */
    fd_set rds;
    int n;
    struct timeval tm;
    FD_ZERO(&rds);
    FD_SET(cam->fd, &rds);
    tm.tv_sec = 0;
    tm.tv_usec = 500000;
    n = select(cam->fd + 1, &rds, NULL, NULL, &tm);
    if (n != 1)
    {
      stop_mmap_enabled(cam);
      start_mmap_enabled(cam);
      if (++bug_count == 4)
      {
	BANO_PERROR();
	goto on_error;
      }
      goto redo_select;
    }
  }

  memset(&buffer, 0, sizeof(buffer));
  buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buffer.memory = V4L2_MEMORY_MMAP;

  if (ioctl(cam->fd, VIDIOC_DQBUF, &buffer) == -1)
  {
    BANO_PERROR();
    goto on_error;
  }

  trans_frame(cam, cam->fb_data[buffer.index], cam->fb_size[buffer.index]);

  if (ioctl(cam->fd, VIDIOC_QBUF, &buffer) == -1)
  {
    BANO_PERROR();
    goto on_error;
  }

  return 0;

 on_error:
  return -1;
}


int bano_cam_open(bano_cam_handle_t* cam, const bano_cam_info_t* info)
{
#define DEV_PATH "/dev/videoNNN"
  char filename[sizeof(DEV_PATH)];
  struct v4l2_capability cap;
  bano_bmp_info_t bmp_info;
  uint32_t width;
  uint32_t height;
  uint32_t fourcc;
  int id;
  int err;

  /* extract format info */

  if (expand_format(info->fmt, &fourcc, &width, &height))
  {
    BANO_PERROR();
    goto on_error_1;
  }

  /* create output bmp */

  bmp_info.width = width;
  bmp_info.height = height;
  bmp_info.bpp = 3;
  if (bano_bmp_open(&cam->bmp, &bmp_info))
  {
    BANO_PERROR();
    goto on_error_0;
  }

  cam->is_dummy = 0;
  if (info->flags & BANO_CAM_FLAG_DUMMY)
  {
    cam->is_dummy = 1;
    return 0;
  }

  if ((info->flags & BANO_CAM_FLAG_ID) == 0) id = 0;
  else id = (int)info->id;

  snprintf(filename, sizeof(filename), "/dev/video%d", id);

  if ((cam->fd = open(filename, O_RDWR)) == -1)
  {
    BANO_PERROR();
    goto on_error_1;
  }

  if (ioctl(cam->fd, VIDIOC_QUERYCAP, &cap) == -1)
  {
    BANO_PERROR();
    goto on_error_2;
  }

  if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
  {
    BANO_PERROR();
    goto on_error_2;
  }

  cam->fmt = info->fmt;
  cam->is_mmap_enabled = 0;
  cam->fb_count = 0;

  /* set format */

  if (set_format(cam->fd, fourcc, width, height))
  {
    BANO_PERROR();
    goto on_error_2;
  }

  if (cap.capabilities & V4L2_CAP_STREAMING)
  {
    cam->is_mmap_enabled = 1;
    err = start_mmap_enabled(cam);
  }
  else
  {
    cam->is_mmap_enabled = 0;
    err = start_mmap_disabled(cam);
  }

  if (err)
  {
    BANO_PERROR();
    goto on_error_2;
  }

  return 0;

 on_error_2:
  close(cam->fd);
 on_error_1:
  bano_bmp_close(&cam->bmp);
 on_error_0:
  return -1;
}

int bano_cam_close(bano_cam_handle_t* cam)
{
  bano_bmp_close(&cam->bmp);

  if (cam->is_dummy) return 0;

  if (cam->is_mmap_enabled) stop_mmap_enabled(cam);
  else stop_mmap_disabled(cam);
  close(cam->fd);

  return 0;
}

int bano_cam_capture(bano_cam_handle_t* cam)
{
  int err;

  if (cam->is_dummy) return 0;

  if (cam->is_mmap_enabled) err = capture_mmap_enabled(cam);
  else err = capture_mmap_disabled(cam);

  return err;
}

void bano_cam_get_bmp
(bano_cam_handle_t* cam, const uint8_t** bufp, size_t* sizep)
{
  *bufp = cam->bmp.mem_buf;
  *sizep = cam->bmp.mem_size;
}

#if 0 /* unused */
int bano_cam_print_formats(bano_cam_t* cam)
{
  struct v4l2_fmtdesc desc;

  desc.index = 0;

  while (1)
  {
    desc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (ioctl(cam->fd, VIDIOC_ENUM_FMT, &desc) == -1)
    {
      if (errno != EINVAL) return -1;
    }

    printf("%s:0x%08x\n", desc.description, desc.pixelformat);

    ++desc.index;
  }

  return 0;
}
#endif /* unused */
