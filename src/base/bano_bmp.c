#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include "bano_bmp.h"
#include "bano_perror.h"



/* bmp file header */

struct __attribute__((packed)) file_header
{
  /* bmp header */

  uint16_t magic;
  uint32_t file_size;
  uint16_t reserved[2];
  uint32_t data_offset;

  /* dib header */
  
  uint32_t dib_size;
  int32_t width;
  int32_t height;
  uint16_t color_plane_count;
  uint16_t bpp;
  uint32_t compression;
  uint32_t data_size;
  int32_t horizontal_resolution;
  int32_t vertical_resolution;
  uint32_t palette_color_count;
  uint32_t palette_important_count;

  /* palette */
};


/* exported */

int bano_bmp_open(bano_bmp_handle_t* bmp, const bano_bmp_info_t* info)
{
  const size_t data_size = info->bpp * info->width * info->height;
  const size_t mem_size = sizeof(struct file_header) + data_size;
  struct file_header* hdr;

  bmp->mem_buf = malloc(mem_size);
  if (bmp->mem_buf == NULL)
  {
    BANO_PERROR();
    return -1;
  }

  bmp->mem_size = mem_size;

  bmp->data_buf = bmp->mem_buf + sizeof(struct file_header);
  bmp->data_size = data_size;

  bmp->bpp = info->bpp;
  bmp->width = info->width;
  bmp->height = info->height;

#define BMP_MAGIC 0x4d42
#define DIB_SIZE 0x28
  hdr = (struct file_header*)bmp->mem_buf;
  memset(hdr, 0, sizeof(struct file_header));
  hdr->magic = BMP_MAGIC;
  hdr->file_size = sizeof(struct file_header) + data_size;
  hdr->data_offset = sizeof(struct file_header);
  hdr->dib_size = DIB_SIZE;
  hdr->width = (int32_t)bmp->width;
  hdr->height = (int32_t)bmp->height;
  hdr->color_plane_count = 1;
  hdr->bpp = bmp->bpp * 8;
  hdr->data_size = data_size;

  return 0;
}

void bano_bmp_close(bano_bmp_handle_t* bmp)
{
  free(bmp->mem_buf);
}
