#define buffersize 1024  //size, in bytes, of the write buffer
#define bufferoverflow 4 //size, in bytes, of the write buffer's overflow
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include "jpeg.h"
#include "bmptypes.h"
#include "endian.h"
#include "endian.c"
#include "readbmp.h"
#include "readbmp.c"
#include <sys/time.h>
#include <omp.h>
#include <immintrin.h>
#define PI 3.14159265358979323846
#ifdef UNOPTIMIZED
#include "acc_dct.c"
#else
#include "simd_dct.c"
#endif
#include "writer.c"

typedef struct _JPEGTimings
{
  float read_bmp;
  float transform_colorspace;
  float dct;
  float generate_headers;
  float write_file;
  float total;
} JPEGTimings;

JPEGTimings convertFile(char *input, char *output, int num_colors, bool print_timings)
{
  RGB **argb = (RGB **)malloc(sizeof(RGB *));
  UINT32 *width = (UINT32 *)malloc(sizeof(UINT32 *));
  UINT32 *height = (UINT32 *)malloc(sizeof(UINT32 *));
  FILE *fp;
  struct timeval tv1, tv2, tv3, tv4;
  JPEGTimings timings;

  fp = fopen(input, "rb");

  if (fp == NULL)
  {
    perror("Error opening source file");
    return timings;
  }

  gettimeofday(&tv1, 0);

  gettimeofday(&tv3, 0);
  /* Uses a bmp library to read a bmp image into
   * an array of RGB pixels. */
  readSingleImageBMP(fp, argb, width, height);
  gettimeofday(&tv2, 0);
  timings.read_bmp = tv2.tv_sec - tv1.tv_sec + 1e-6 * (tv2.tv_usec - tv1.tv_usec);

  fclose(fp);

  RGB *image = *argb;
  RGB pixel;

  /* Allocate memory for the JPEG color space as floats. */
  float *Y = malloc(sizeof(float) * *width * *height);
  float *Cb = malloc(sizeof(float) * *width * *height);
  float *Cr = malloc(sizeof(float) * *width * *height);

  gettimeofday(&tv1, 0);
  /* JPEG uses a non-RGB color space.  Y stores greyscale
   * information, while Cb and Cr store color offsets. */
  for (int row = 0; row < *height; row++)
  {
    for (int col = 0; col < *width; col++)
    {
      pixel = image[row * *width + col];
      Y[row * *width + col] = 0.299 * pixel.red + 0.587 * pixel.green + 0.114 * pixel.blue;
      Cb[row * *width + col] = 128 - 0.168736 * pixel.red - 0.331264 * pixel.green + 0.5 * pixel.blue;
      Cr[row * *width + col] = 128 + 0.5 * pixel.red - 0.418688 * pixel.green - 0.081312 * pixel.blue;
    }
  }
  gettimeofday(&tv2, 0);
  timings.transform_colorspace = tv2.tv_sec - tv1.tv_sec + 1e-6 * (tv2.tv_usec - tv1.tv_usec);

  gettimeofday(&tv1, 0);

  /* Allocate memory for the JPEG color space as rounded integers. */
  int32_t *Yout = malloc(sizeof(int32_t) * *width * *height);
  int32_t *Cbout = malloc(sizeof(int32_t) * *width * *height);
  int32_t *Crout = malloc(sizeof(int32_t) * *width * *height);

  float lquant[8][8], cquant[8][8];

  /* JPEG uses a quantization table to reduce the number
   * of bits necessary to store the DCT frequencies as
   * well as zero out frequencies that aren't important.
   * The standard form of these tables is in a zigzag, which
   * needs to be undone so that the AVX instructions can make
   * use of them. */
  for (int rrow = 0; rrow < 8; rrow++)
  {
    for (int ccol = 0; ccol < 8; ccol++)
    {
      int pos = zigzag[rrow * 8 + ccol];
      lquant[pos / 8][pos % 8] = s_std_lum_quant[rrow * 8 + ccol];
      cquant[pos / 8][pos % 8] = s_std_croma_quant[rrow * 8 + ccol];
    }
  }

  run_dct(*height, *width, lquant, Y, Yout);
  if (num_colors > 1)
  {
    run_dct(*height, *width, cquant, Cb, Cbout);
    run_dct(*height, *width, cquant, Cr, Crout);
  }

  free(Y);
  free(Cb);
  free(Cr);

  gettimeofday(&tv2, 0);
  timings.dct = tv2.tv_sec - tv1.tv_sec + 1e-6 * (tv2.tv_usec - tv1.tv_usec);

  fp = fopen(output, "wb");

  gettimeofday(&tv1, 0);
  /* JPEG has a standard file format, with a header defining
   * the file type, the size of the image, the quantization tables
   * used to round the color values, and the huffman tables used to
   * store the information in a variable length code. */
  output_header(fp, *height, *width, num_colors);
  gettimeofday(&tv2, 0);
  timings.generate_headers = tv2.tv_sec - tv1.tv_sec + 1e-6 * (tv2.tv_usec - tv1.tv_usec);

  gettimeofday(&tv1, 0);
  write_out(fp, *height, *width, num_colors, Yout, Cbout, Crout);
  gettimeofday(&tv2, 0);
  timings.write_file = tv2.tv_sec - tv1.tv_sec + 1e-6 * (tv2.tv_usec - tv1.tv_usec);

  gettimeofday(&tv4, 0);
  timings.total = tv4.tv_sec - tv3.tv_sec + 1e-6 * (tv4.tv_usec - tv3.tv_usec);

  /* Free all remaining memory. */
  free(argb);
  free(width);
  free(height);
  free(Yout);
  free(Cbout);
  free(Crout);

  return timings;
}


