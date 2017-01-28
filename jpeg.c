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

typedef struct _JPEGTimings
{
  float read_bmp;
  float transform_colorspace;
  float dct;
  float generate_headers;
  float write_file;
  float total;
} JPEGTimings;

int length[10] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512};

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

void run_dct(int width, int height,float *quant, float *input, int32_t *output)
{
  float acosvals[8][8];

  /* Calculating cosines is expensive, and there
   * are only 64 cosines that need to be calculated
   * so precompute them and cache. */
  for (int i = 0; i < 8; i++)
  {
    for (int j = 0; j < 8; j++)
    {
      if (j == 0) {
        acosvals[i][j] = sqrt(1.0 / 8.0) * cos(PI / 8.0 * (i + 0.5d) * j);
      }
      else {
        acosvals[i][j] = 0.5 * cos(PI / 8.0 * (i + 0.5d) * j);
      }
    }
  }

/* Separate the parallel from the for, so each processor gets its
   * own copy of the buffers and variables. */
#pragma omp parallel
  {
    float avload[8] = {0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5};
    avload[0] = sqrt(1.0 / 8.0);
    __m256 row0, row1, row2, row3, row4, row5, row6, row7;
    __m256 loaderlow, loaderhigh;
    __m256 temp;
    __m256 minus128 = _mm256_set1_ps(-128.0);
    __m256 avxcos;
    __m256i integer;

    /* The DCT breaks the image into 8 by 8 blocks and then
   * transforms them into color frequencies. */
#pragma omp for
    for (int brow = 0; brow < height / 8; brow++)
    {
      for (int bcol = 0; bcol < width / 8; bcol++)
      {
        int head_pointer = bcol * 8 + brow * 8 * width;
        row0 = _mm256_setzero_ps();
        row1 = _mm256_setzero_ps();
        row2 = _mm256_setzero_ps();
        row3 = _mm256_setzero_ps();
        row4 = _mm256_setzero_ps();
        row5 = _mm256_setzero_ps();
        row6 = _mm256_setzero_ps();
        row7 = _mm256_setzero_ps();

        /* This pair of loops uses AVX instuctions to add the frequency
       * component from each pixel to all of the buckets at once.  Allows
       * us to do the DCT on a block in 64 iterations of a loop rather
       * than 64 iterations of 64 iterations of a loop (all 64 pixels affect
       * all 64 frequencies) */
        for (int x = 0; x < 8; x++)
        {
          for (int y = 0; y < 4; y++)
          {
            loaderlow = _mm256_broadcast_ss(&input[head_pointer + x + (y * width)]);
            loaderlow = _mm256_add_ps(loaderlow, minus128);
            loaderhigh = _mm256_broadcast_ss(&input[head_pointer + x + ((7 - y) * width)]);
            loaderhigh = _mm256_add_ps(loaderhigh, minus128);

            avxcos = _mm256_loadu_ps(&acosvals[x][0]);
            loaderlow = _mm256_mul_ps(loaderlow, avxcos);
            loaderhigh = _mm256_mul_ps(loaderhigh, avxcos);

            avxcos = _mm256_broadcast_ss(&acosvals[y][0]);
            temp = _mm256_mul_ps(loaderlow, avxcos);
            row0 = _mm256_add_ps(row0, temp);
            temp = _mm256_mul_ps(loaderhigh, avxcos);
            row0 = _mm256_add_ps(row0, temp);

            avxcos = _mm256_broadcast_ss(&acosvals[y][1]);
            temp = _mm256_mul_ps(loaderlow, avxcos);
            row1 = _mm256_add_ps(row1, temp);
            temp = _mm256_mul_ps(loaderhigh, avxcos);
            row1 = _mm256_sub_ps(row1, temp);

            avxcos = _mm256_broadcast_ss(&acosvals[y][2]);
            temp = _mm256_mul_ps(loaderlow, avxcos);
            row2 = _mm256_add_ps(row2, temp);
            temp = _mm256_mul_ps(loaderhigh, avxcos);
            row2 = _mm256_add_ps(row2, temp);

            avxcos = _mm256_broadcast_ss(&acosvals[y][3]);
            temp = _mm256_mul_ps(loaderlow, avxcos);
            row3 = _mm256_add_ps(row3, temp);
            temp = _mm256_mul_ps(loaderhigh, avxcos);
            row3 = _mm256_sub_ps(row3, temp);

            avxcos = _mm256_broadcast_ss(&acosvals[y][4]);
            temp = _mm256_mul_ps(loaderlow, avxcos);
            row4 = _mm256_add_ps(row4, temp);
            temp = _mm256_mul_ps(loaderhigh, avxcos);
            row4 = _mm256_add_ps(row4, temp);

            avxcos = _mm256_broadcast_ss(&acosvals[y][5]);
            temp = _mm256_mul_ps(loaderlow, avxcos);
            row5 = _mm256_add_ps(row5, temp);
            temp = _mm256_mul_ps(loaderhigh, avxcos);
            row5 = _mm256_sub_ps(row5, temp);

            avxcos = _mm256_broadcast_ss(&acosvals[y][6]);
            temp = _mm256_mul_ps(loaderlow, avxcos);
            row6 = _mm256_add_ps(row6, temp);
            temp = _mm256_mul_ps(loaderhigh, avxcos);
            row6 = _mm256_add_ps(row6, temp);

            avxcos = _mm256_broadcast_ss(&acosvals[y][7]);
            temp = _mm256_mul_ps(loaderlow, avxcos);
            row7 = _mm256_add_ps(row7, temp);
            temp = _mm256_mul_ps(loaderhigh, avxcos);
            row7 = _mm256_sub_ps(row7, temp);
          }
        }

        /* Each frequency stored as a float needs to be divided by
       * the quantization value, then rounded to the nearest integer.
       * Also changes the order of the values from pixel order to
       * each 8 by 8 block stored one after another. */
        temp = _mm256_loadu_ps(&quant[0]);
        row0 = _mm256_div_ps(row0, temp);
        row0 = _mm256_round_ps(row0, _MM_FROUND_TO_NEAREST_INT);
        integer = _mm256_cvttps_epi32(row0);
        _mm256_storeu_si256(output + (bcol + brow * (width / 8)) * 64, integer);

        temp = _mm256_loadu_ps(&quant[8]);
        row1 = _mm256_div_ps(row1, temp);
        row1 = _mm256_round_ps(row1, _MM_FROUND_TO_NEAREST_INT);
        integer = _mm256_cvttps_epi32(row1);
        _mm256_storeu_si256(output + 8 + (bcol + brow * (width / 8)) * 64, integer);

        temp = _mm256_loadu_ps(&quant[16]);
        row2 = _mm256_div_ps(row2, temp);
        row2 = _mm256_round_ps(row2, _MM_FROUND_TO_NEAREST_INT);
        integer = _mm256_cvttps_epi32(row2);
        _mm256_storeu_si256(output + 16 + (bcol + brow * (width / 8)) * 64, integer);

        temp = _mm256_loadu_ps(&quant[24]);
        row3 = _mm256_div_ps(row3, temp);
        row3 = _mm256_round_ps(row3, _MM_FROUND_TO_NEAREST_INT);
        integer = _mm256_cvttps_epi32(row3);
        _mm256_storeu_si256(output + 24 + (bcol + brow * (width / 8)) * 64, integer);

        temp = _mm256_loadu_ps(&quant[32]);
        row4 = _mm256_div_ps(row4, temp);
        row4 = _mm256_round_ps(row4, _MM_FROUND_TO_NEAREST_INT);
        integer = _mm256_cvttps_epi32(row4);
        _mm256_storeu_si256(output + 32 + (bcol + brow * (width / 8)) * 64, integer);

        temp = _mm256_loadu_ps(&quant[40]);
        row5 = _mm256_div_ps(row5, temp);
        row5 = _mm256_round_ps(row5, _MM_FROUND_TO_NEAREST_INT);
        integer = _mm256_cvttps_epi32(row5);
        _mm256_storeu_si256(output + 40 + (bcol + brow * (width / 8)) * 64, integer);

        temp = _mm256_loadu_ps(&quant[48]);
        row6 = _mm256_div_ps(row6, temp);
        row6 = _mm256_round_ps(row6, _MM_FROUND_TO_NEAREST_INT);
        integer = _mm256_cvttps_epi32(row6);
        _mm256_storeu_si256(output + 48 + (bcol + brow * (width / 8)) * 64, integer);

        temp = _mm256_loadu_ps(&quant[56]);
        row7 = _mm256_div_ps(row7, temp);
        row7 = _mm256_round_ps(row7, _MM_FROUND_TO_NEAREST_INT);
        integer = _mm256_cvttps_epi32(row7);
        _mm256_storeu_si256(output + 56 + (bcol + brow * (width / 8)) * 64, integer);
      }
    }
  }
}

/* Outputs the frequency information using variable length encoding.
 * The first frequency (DC value) is encoded separately from the
 * remaining (AC values). Each value is encoded in two parts.  The
 * first encodes the order of magnitude of the value (both types)
 * and the number of zero valued frequencies preceding this one
 * (AC values).  This is then encoded using a huffman table.  The
 * second encodes the precise value of the
 * frequency, with the exact number of bits stored varying with
 * order of magnitude previously described. */
void write_out(FILE *fp, int height, int width, int num_colors, int32_t *Yout, int32_t *Cbout, int32_t *Crout)
{
  int run = 0;
  int amplitude;
  int *sout;
  int counter = 8 * (buffersize + bufferoverflow); //initialized to buffer size, in bits
  uint8_t *buffer = malloc(sizeof(uint8_t) * (buffersize + bufferoverflow));
  uint8_t btemp = 0;
  int lastdc[3] = {0, 0, 0};
  int ac_array;

  for (int i = 0; i < 8; i++)
  {
    buffer[i] = 0;
  }

  uint16_t **codes = malloc(sizeof(uint16_t *) * 4);
  uint8_t **sizes = malloc(sizeof(uint8_t *) * 4);

  for (int i = 0; i < ((num_colors > 1) ? 4 : 2); i++)
  {
    codes[i] = malloc(sizeof(uint16_t) * 256);
    sizes[i] = malloc(sizeof(uint8_t) * 256);
  }
  generate_huffman_tables(num_colors, codes, sizes);

  for (int z = 0; z < height * width; z += 64)
  {
    for (int w = 0; w < num_colors; w++)
    {
      if (w == 0)
      {
        sout = &Yout[z];
        ac_array = 0;
      }
      else if (w == 1)
      {
        sout = &Cbout[z];
        ac_array = 2;
      }
      else if (w == 2)
      {
        sout = &Crout[z];
        ac_array = 2;
      }
      for (int j = 0; j < 11; j++)
      {
        if ((sout[0] - lastdc[w]) < length[j] && (sout[0] - lastdc[w]) > -length[j])
        {
          multibitwriter(&counter, buffer, codes[ac_array + 1][j], sizes[ac_array + 1][j], &btemp, fp);
          if ((sout[0] - lastdc[w]) > 0)
            amplitude = (1 << (j - 1)) + ((sout[0] - lastdc[w]) - length[j - 1]);
          else
            amplitude = length[j] + (sout[0] - lastdc[w]) - 1;
          multibitwriter(&counter, buffer, amplitude, j, &btemp, fp);
          lastdc[w] = sout[0];
          break;
        }
      }

      for (int i = 1; i < 64; i++)
      {
        if (sout[zigzag[i]] == 0)
        {
          run++;
          if (i == 63 && run > 0)
          {
            multibitwriter(&counter, buffer, codes[ac_array][0x00], sizes[ac_array][0x00], &btemp, fp);
            run = 0;
          }
          continue;
        }
        for (int j = 1; j < 11; j++)
        {
          if (sout[zigzag[i]] < length[j] && sout[zigzag[i]] > -length[j])
          {
            while (run > 15)
            {
              multibitwriter(&counter, buffer, codes[ac_array][0xf0], sizes[ac_array][0xf0], &btemp, fp);
              run -= 16;
            }
            multibitwriter(&counter, buffer, codes[ac_array][(run << 4) + j], sizes[ac_array][(run << 4) + j], &btemp, fp);
            if (sout[zigzag[i]] > 0)
              amplitude = (1 << (j - 1)) + (sout[zigzag[i]] - length[j - 1]);
            else
              amplitude = length[j] + sout[zigzag[i]] - 1;
            multibitwriter(&counter, buffer, amplitude, j, &btemp, fp);
            run = 0;
            break;
          }
        }
      }
    }
  }
  /* Flush the bits remaining on the buffer,
   * put an end of file marker and close the file. */
  finishfilemulti(&counter, buffer, btemp, fp);

  for (int i = 0; i < ((num_colors > 1) ? 4 : 2); i++)
  {
    free(codes[i]);
    free(sizes[i]);
  }
  free(codes);
  free(sizes);
  free(buffer);
}
