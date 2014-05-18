#ifndef BANO_BMP_H_INCLUDED
# define BANO_BMP_H_INCLUDED


#include <stdint.h>
#include <sys/types.h>


typedef struct bano_bmp_info
{
  /* bpp in bytes */
  size_t bpp;
  size_t width;
  size_t height;
} bano_bmp_info_t;


typedef struct bano_bmp_handle
{
  /* dimensions */
  size_t bpp;
  size_t width;
  size_t height;

  /* header plus data buffer */
  uint8_t* mem_buf;
  size_t mem_size;

  /* actual data pointer */
  uint8_t* data_buf;
  size_t data_size;

} bano_bmp_handle_t;


int bano_bmp_open(bano_bmp_handle_t*, const bano_bmp_info_t*);
void bano_bmp_close(bano_bmp_handle_t*);


#endif /* ! BMP_H_INCLUDED */
