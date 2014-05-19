/*
LodePNG Examples

Copyright (c) 2005-2012 Lode Vandevenne

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
    distribution.
*/

/* this example is taken from lodepng distribution then reduced to minimum */
/* compile with: */
/* gcc -O2 -Wall -I. */
/* -DLODEPNG_NO_COMPILE_DECODER -DLODEPNG_NO_COMPILE_CPP */
/* main.c_encode.c lodepng.c */

#include "lodepng.h"

#include <stdio.h>
#include <stdlib.h>

void encodeTwoSteps(const char* filename, const unsigned char* image, unsigned width, unsigned height)
{
  unsigned char* png;
  size_t pngsize;

  unsigned error = lodepng_encode24(&png, &pngsize, image, width, height);
  if(!error) lodepng_save_file(png, pngsize, filename);

  /*if there's an error, display it*/
  if(error) printf("error %u: %s\n", error, lodepng_error_text(error));

  free(png);
}

int main(int argc, char *argv[])
{
  const char* filename = argc > 1 ? argv[1] : "test.png";

  /*generate some image*/
  unsigned width = 320, height = 240;
  unsigned char* image = malloc(width * height * 3);
  unsigned x, y;
  for(y = 0; y < height; y++)
  for(x = 0; x < width; x++)
  {
    image[3 * width * y + 3 * x + 0] = 255 * !(x & y);
    image[3 * width * y + 3 * x + 1] = x ^ y;
    image[3 * width * y + 3 * x + 2] = x | y;
  }

  /*run an example*/
  encodeTwoSteps(filename, image, width, height);

  free(image);
  return 0;
}
