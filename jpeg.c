#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include "huffman.c"
#include "jpeg.h"
#include "bmptypes.h"
#include "endian.h"
#include "endian.c"
#include "readbmp.h"
#include "readbmp.c"
#include "jpeg_header.h"
#include <sys/time.h>
#include <omp.h>
#include <immintrin.h>
#define PI 3.14159265358979323846

int length[10] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512};

int main(int argc, char *argv[]) {
  RGB** argb = (RGB**) malloc(sizeof(RGB*));
  UINT32* width = (UINT32*) malloc(sizeof(UINT32*));
  UINT32* height = (UINT32*) malloc(sizeof(UINT32*));
  FILE *fp;
  struct timeval tv1, tv2, tv3, tv4;
  double time;


  int num_colors = 3;

  fp = fopen(argv[1], "rb");

  if (fp == NULL) {
    perror ("Error opening source file");
    return 2;
  }

  gettimeofday(&tv1, 0);
  gettimeofday(&tv3, 0);
  readSingleImageBMP(fp, argb, width, height);
  gettimeofday(&tv2, 0);

  time = tv2.tv_sec - tv1.tv_sec + 1e-6 * (tv2.tv_usec - tv1.tv_usec);

  printf("Read BMP: %f seconds.\n", time);


  fclose(fp);

  RGB* image = *argb;
  RGB pixel;

  float* Y = malloc(sizeof(float) * *width * *height);
  float* Cb = malloc(sizeof(float) * *width * *height);
  float* Cr = malloc(sizeof(float) * *width * *height);
  float* Ye = malloc(sizeof(float) * *width * *height);
  float* Cbe = malloc(sizeof(float) * *width * *height);
  float* Cre = malloc(sizeof(float) * *width * *height);
  int* Yout = malloc(sizeof(int) * *width * *height);
  int* Cbout = malloc(sizeof(int) * *width * *height);
  int* Crout = malloc(sizeof(int) * *width * *height);

  gettimeofday(&tv1, 0);
  for (int row = 0; row < *height; row++) {
    for (int col = 0; col < *width; col++) {
      pixel = image[row * *width + col];
      Y[row * *width + col] = 0.299*pixel.red + 0.587*pixel.green + 0.114*pixel.blue;
      Cb[row * *width + col] = 128 - 0.168736*pixel.red - 0.331264*pixel.green + 0.5*pixel.blue;
      Cr[row * *width + col] = 128 + 0.5*pixel.red - 0.418688*pixel.green - 0.081312*pixel.blue;
    }
  }
  gettimeofday(&tv2, 0);

  time = tv2.tv_sec - tv1.tv_sec + 1e-6 * (tv2.tv_usec - tv1.tv_usec);

  printf("Transform colorspace: %f seconds.\n", time);

  float cosvals[8][8];

  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      cosvals[i][j] = cos(PI/8.0*(i+0.5)*j);
    }
  }

  gettimeofday(&tv1, 0);
#pragma omp parallel
{
  float avload[8] = {0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5};
  avload[0] =  sqrt(1.0/8.0);
  __m256 row0, row1, row2, row3, row4, row5, row6, row7;
  __m256 loader;
  __m256 temp;
  __m256 minus128 = _mm256_set1_ps(-128.0);
  __m256 av = _mm256_loadu_ps(&avload[0]), au1 = _mm256_broadcast_ss(&avload[0]), au2 = _mm256_broadcast_ss(&avload[1]);
  __m256 avxcos;

  float writer[8];

#pragma omp for
  for (int brow = 0; brow < *height/8; brow++) {
    for (int bcol = 0; bcol < *width/8; bcol++) {
      int head_pointer = bcol*8 + brow * 8 * *width;
      row0 = _mm256_setzero_ps();
      row1 = _mm256_setzero_ps();
      row2 = _mm256_setzero_ps();
      row3 = _mm256_setzero_ps();
      row4 = _mm256_setzero_ps();
      row5 = _mm256_setzero_ps();
      row6 = _mm256_setzero_ps();
      row7 = _mm256_setzero_ps();
      for (int x = 0; x < 8; x++) {
	for (int y = 0; y < 8; y++) {
	  loader = _mm256_broadcast_ss(&Y[head_pointer+x+(y * *width)]);
          loader = _mm256_add_ps(loader, minus128);
          loader = _mm256_mul_ps(loader, av);
          avxcos = _mm256_loadu_ps(&cosvals[x][0]);
	  loader = _mm256_mul_ps(loader, avxcos);

          temp = loader;
          avxcos = _mm256_broadcast_ss(&cosvals[y][0]);
	  temp = _mm256_mul_ps(temp, au1);
          temp = _mm256_mul_ps(temp, avxcos);
          row0 = _mm256_add_ps(row0, temp);

          temp = loader;
          avxcos = _mm256_broadcast_ss(&cosvals[y][1]);
	  temp = _mm256_mul_ps(temp, au2);
          temp = _mm256_mul_ps(temp, avxcos);
          row1 = _mm256_add_ps(row1, temp);

          temp = loader;
          avxcos = _mm256_broadcast_ss(&cosvals[y][2]);
	  temp = _mm256_mul_ps(temp, au2);
          temp = _mm256_mul_ps(temp, avxcos);
          row2 = _mm256_add_ps(row2, temp);

          temp = loader;
          avxcos = _mm256_broadcast_ss(&cosvals[y][3]);
	  temp = _mm256_mul_ps(temp, au2);
          temp = _mm256_mul_ps(temp, avxcos);
          row3 = _mm256_add_ps(row3, temp);

          temp = loader;
          avxcos = _mm256_broadcast_ss(&cosvals[y][4]);
	  temp = _mm256_mul_ps(temp, au2);
          temp = _mm256_mul_ps(temp, avxcos);
          row4 = _mm256_add_ps(row4, temp);

          temp = loader;
          avxcos = _mm256_broadcast_ss(&cosvals[y][5]);
	  temp = _mm256_mul_ps(temp, au2);
          temp = _mm256_mul_ps(temp, avxcos);
          row5 = _mm256_add_ps(row5, temp);

          temp = loader;
          avxcos = _mm256_broadcast_ss(&cosvals[y][6]);
	  temp = _mm256_mul_ps(temp, au2);
          temp = _mm256_mul_ps(temp, avxcos);
          row6 = _mm256_add_ps(row6, temp);

          temp = loader;
          avxcos = _mm256_broadcast_ss(&cosvals[y][7]);
	  temp = _mm256_mul_ps(temp, au2);
          temp = _mm256_mul_ps(temp, avxcos);
          row7 = _mm256_add_ps(row7, temp);
	}
      }
      _mm256_storeu_ps(&Ye[head_pointer], row0);
      _mm256_storeu_ps(&Ye[head_pointer + *width], row1);
      _mm256_storeu_ps(&Ye[head_pointer + (2 * *width)], row2);
      _mm256_storeu_ps(&Ye[head_pointer + (3 * *width)], row3);
      _mm256_storeu_ps(&Ye[head_pointer + (4 * *width)], row4);
      _mm256_storeu_ps(&Ye[head_pointer + (5 * *width)], row5);
      _mm256_storeu_ps(&Ye[head_pointer + (6 * *width)], row6);
      _mm256_storeu_ps(&Ye[head_pointer + (7 * *width)], row7);
    }
  }

  if (num_colors > 1) {
#pragma omp for
    for (int brow = 0; brow < *height/8; brow++) {
      for (int bcol = 0; bcol < *width/8; bcol++) {
	int head_pointer = bcol*8 + brow * 8 * *width;
	row0 = _mm256_setzero_ps();
	row1 = _mm256_setzero_ps();
	row2 = _mm256_setzero_ps();
	row3 = _mm256_setzero_ps();
	row4 = _mm256_setzero_ps();
	row5 = _mm256_setzero_ps();
	row6 = _mm256_setzero_ps();
	row7 = _mm256_setzero_ps();
	for (int x = 0; x < 8; x++) {
	  for (int y = 0; y < 8; y++) {
	    loader = _mm256_broadcast_ss(&Cb[head_pointer+x+(y * *width)]);
	    loader = _mm256_add_ps(loader, minus128);
	    loader = _mm256_mul_ps(loader, av);
	    avxcos = _mm256_loadu_ps(&cosvals[x][0]);
	    loader = _mm256_mul_ps(loader, avxcos);

	    temp = loader;
	    avxcos = _mm256_broadcast_ss(&cosvals[y][0]);
	    temp = _mm256_mul_ps(temp, au1);
	    temp = _mm256_mul_ps(temp, avxcos);
	    row0 = _mm256_add_ps(row0, temp);

	    temp = loader;
	    avxcos = _mm256_broadcast_ss(&cosvals[y][1]);
	    temp = _mm256_mul_ps(temp, au2);
	    temp = _mm256_mul_ps(temp, avxcos);
	    row1 = _mm256_add_ps(row1, temp);

	    temp = loader;
	    avxcos = _mm256_broadcast_ss(&cosvals[y][2]);
	    temp = _mm256_mul_ps(temp, au2);
	    temp = _mm256_mul_ps(temp, avxcos);
	    row2 = _mm256_add_ps(row2, temp);

	    temp = loader;
	    avxcos = _mm256_broadcast_ss(&cosvals[y][3]);
	    temp = _mm256_mul_ps(temp, au2);
	    temp = _mm256_mul_ps(temp, avxcos);
	    row3 = _mm256_add_ps(row3, temp);

	    temp = loader;
	    avxcos = _mm256_broadcast_ss(&cosvals[y][4]);
	    temp = _mm256_mul_ps(temp, au2);
	    temp = _mm256_mul_ps(temp, avxcos);
	    row4 = _mm256_add_ps(row4, temp);

	    temp = loader;
	    avxcos = _mm256_broadcast_ss(&cosvals[y][5]);
	    temp = _mm256_mul_ps(temp, au2);
	    temp = _mm256_mul_ps(temp, avxcos);
	    row5 = _mm256_add_ps(row5, temp);

	    temp = loader;
	    avxcos = _mm256_broadcast_ss(&cosvals[y][6]);
	    temp = _mm256_mul_ps(temp, au2);
	    temp = _mm256_mul_ps(temp, avxcos);
	    row6 = _mm256_add_ps(row6, temp);

	    temp = loader;
	    avxcos = _mm256_broadcast_ss(&cosvals[y][7]);
	    temp = _mm256_mul_ps(temp, au2);
	    temp = _mm256_mul_ps(temp, avxcos);
	    row7 = _mm256_add_ps(row7, temp);
	  }
	}
	_mm256_storeu_ps(&Cbe[head_pointer], row0);
	_mm256_storeu_ps(&Cbe[head_pointer + *width], row1);
	_mm256_storeu_ps(&Cbe[head_pointer + (2 * *width)], row2);
	_mm256_storeu_ps(&Cbe[head_pointer + (3 * *width)], row3);
	_mm256_storeu_ps(&Cbe[head_pointer + (4 * *width)], row4);
	_mm256_storeu_ps(&Cbe[head_pointer + (5 * *width)], row5);
	_mm256_storeu_ps(&Cbe[head_pointer + (6 * *width)], row6);
	_mm256_storeu_ps(&Cbe[head_pointer + (7 * *width)], row7);
      }
    }
#pragma omp for
    for (int brow = 0; brow < *height/8; brow++) {
      for (int bcol = 0; bcol < *width/8; bcol++) {
	int head_pointer = bcol*8 + brow * 8 * *width;
	row0 = _mm256_setzero_ps();
	row1 = _mm256_setzero_ps();
	row2 = _mm256_setzero_ps();
	row3 = _mm256_setzero_ps();
	row4 = _mm256_setzero_ps();
	row5 = _mm256_setzero_ps();
	row6 = _mm256_setzero_ps();
	row7 = _mm256_setzero_ps();
	for (int x = 0; x < 8; x++) {
	  for (int y = 0; y < 8; y++) {
	    loader = _mm256_broadcast_ss(&Cr[head_pointer+x+(y * *width)]);
	    loader = _mm256_add_ps(loader, minus128);
	    loader = _mm256_mul_ps(loader, av);
	    avxcos = _mm256_loadu_ps(&cosvals[x][0]);
	    loader = _mm256_mul_ps(loader, avxcos);

	    temp = loader;
	    avxcos = _mm256_broadcast_ss(&cosvals[y][0]);
	    temp = _mm256_mul_ps(temp, au1);
	    temp = _mm256_mul_ps(temp, avxcos);
	    row0 = _mm256_add_ps(row0, temp);

	    temp = loader;
	    avxcos = _mm256_broadcast_ss(&cosvals[y][1]);
	    temp = _mm256_mul_ps(temp, au2);
	    temp = _mm256_mul_ps(temp, avxcos);
	    row1 = _mm256_add_ps(row1, temp);

	    temp = loader;
	    avxcos = _mm256_broadcast_ss(&cosvals[y][2]);
	    temp = _mm256_mul_ps(temp, au2);
	    temp = _mm256_mul_ps(temp, avxcos);
	    row2 = _mm256_add_ps(row2, temp);

	    temp = loader;
	    avxcos = _mm256_broadcast_ss(&cosvals[y][3]);
	    temp = _mm256_mul_ps(temp, au2);
	    temp = _mm256_mul_ps(temp, avxcos);
	    row3 = _mm256_add_ps(row3, temp);

	    temp = loader;
	    avxcos = _mm256_broadcast_ss(&cosvals[y][4]);
	    temp = _mm256_mul_ps(temp, au2);
	    temp = _mm256_mul_ps(temp, avxcos);
	    row4 = _mm256_add_ps(row4, temp);

	    temp = loader;
	    avxcos = _mm256_broadcast_ss(&cosvals[y][5]);
	    temp = _mm256_mul_ps(temp, au2);
	    temp = _mm256_mul_ps(temp, avxcos);
	    row5 = _mm256_add_ps(row5, temp);

	    temp = loader;
	    avxcos = _mm256_broadcast_ss(&cosvals[y][6]);
	    temp = _mm256_mul_ps(temp, au2);
	    temp = _mm256_mul_ps(temp, avxcos);
	    row6 = _mm256_add_ps(row6, temp);

	    temp = loader;
	    avxcos = _mm256_broadcast_ss(&cosvals[y][7]);
	    temp = _mm256_mul_ps(temp, au2);
	    temp = _mm256_mul_ps(temp, avxcos);
	    row7 = _mm256_add_ps(row7, temp);
	  }
	}
	_mm256_storeu_ps(&Cre[head_pointer], row0);
	_mm256_storeu_ps(&Cre[head_pointer + *width], row1);
	_mm256_storeu_ps(&Cre[head_pointer + (2 * *width)], row2);
	_mm256_storeu_ps(&Cre[head_pointer + (3 * *width)], row3);
	_mm256_storeu_ps(&Cre[head_pointer + (4 * *width)], row4);
	_mm256_storeu_ps(&Cre[head_pointer + (5 * *width)], row5);
	_mm256_storeu_ps(&Cre[head_pointer + (6 * *width)], row6);
	_mm256_storeu_ps(&Cre[head_pointer + (7 * *width)], row7);
      }
    }
  }
 }


  gettimeofday(&tv2, 0);

  time = tv2.tv_sec - tv1.tv_sec + 1e-6 * (tv2.tv_usec - tv1.tv_usec);

  printf("DCT: %f seconds.\n", time);

  gettimeofday(&tv1, 0);
  for (int brow = 0; brow < *height/8; brow++) {
    for (int bcol = 0; bcol < *width/8; bcol++) {
      int head_pointer = bcol*8 + brow * 8 * *width;
      for (int rrow = 0;  rrow < 8; rrow++) {
        for (int ccol = 0; ccol < 8; ccol++) {
          int pos = zigzag[rrow*8 + ccol];
          Yout[(bcol + brow * (*width/8))*64 + (rrow * 8) + ccol] = round(Ye[head_pointer + (pos/8 * *width) + pos%8] / s_std_lum_quant[pos]);
          Cbout[(bcol + brow * (*width/8))*64 + (rrow * 8) + ccol] = round(Cbe[head_pointer + (pos/8 * *width) + pos%8] / s_std_croma_quant[pos]);
          Crout[(bcol + brow * (*width/8))*64 + (rrow * 8) + ccol] = round(Cre[head_pointer + (pos/8 * *width) + pos%8] / s_std_croma_quant[pos]);
	}
      }
    }
  }
  gettimeofday(&tv2, 0);

  time = tv2.tv_sec - tv1.tv_sec + 1e-6 * (tv2.tv_usec - tv1.tv_usec);

  printf("Quantize: %f seconds.\n", time);

  int run = 0;
  int amplitude;
  int* sout;
  int counter = 7;
  uint8_t buffer = 0;
  int lastdc[3] = {0, 0, 0};
  int ac_array;

  uint16_t** codes = malloc(sizeof(uint16_t*) * 4);
  uint8_t** sizes = malloc(sizeof(uint8_t*) * 4);

  for (int i = 0; i < ((num_colors > 1) ? 4 : 2); i++) {
    codes[i] = malloc(sizeof(uint16_t) * 256);
    sizes[i] = malloc(sizeof(uint8_t) * 256);
  }

  fp = fopen(argv[2], "wb");

  gettimeofday(&tv1, 0);
  output_header(fp, *height, *width, num_colors, codes, sizes);
  gettimeofday(&tv2, 0);

  time = tv2.tv_sec - tv1.tv_sec + 1e-6 * (tv2.tv_usec - tv1.tv_usec);

  printf("Generate headers: %f seconds.\n", time);

  gettimeofday(&tv1, 0);
  for (int z = 0; z < *height * *width; z += 64) {
    for (int w = 0; w < num_colors; w++) {
      if (w == 0) {
	sout = &Yout[z];
        ac_array = 0;
      } else if (w == 1) {
        sout = &Cbout[z];
        ac_array = 2;
      } else if (w == 2) {
        sout = &Crout[z];
        ac_array = 2;
      }
      for (int j = 0; j < 11; j++) {
	if ((sout[0] - lastdc[w]) < length[j] && (sout[0] - lastdc[w]) > -length[j]) {
	  for (int k = sizes[ac_array+1][j]-1; k >= 0; k--) {
	    bitwriter(&counter, &buffer, (bool) ((codes[ac_array+1][j] & 1<<k) ? 1 : 0), fp);
	  }
	  if ((sout[0] - lastdc[w]) > 0)
	    amplitude = (1<<(j-1)) + ((sout[0] - lastdc[w]) - length[j-1]);
	  else
	    amplitude = length[j] + (sout[0] - lastdc[w]) - 1;
	  for (int i = j-1; i >= 0; i--) {
	    bitwriter(&counter, &buffer, (bool) ((amplitude & 1<<i) ? 1 : 0), fp);
	  }
	  lastdc[w] = sout[0];
	  break;
	}
      }

      for (int i = 1; i < 64; i++) {
	if (sout[i] == 0) {
	  run++;
	  if (i == 63 && run > 0) {
	    for (int k = sizes[ac_array][0x00]-1; k >= 0; k--) {
	      bitwriter(&counter, &buffer, (bool) ((codes[ac_array][0x00] & 1<<k) ? 1 : 0), fp);
	    }
	    run = 0;
	  }
	  continue;
	}
	for (int j = 1; j < 11; j++) {
	  if (sout[i] < length[j] && sout[i] > -length[j]) {
	    while (run > 15) {
	      for (int k = sizes[ac_array][0xf0]-1; k >= 0; k--) {
		bitwriter(&counter, &buffer, (bool) ((codes[ac_array][0xf0] & 1<<k) ? 1 : 0), fp);
	      }
	      run -= 16;
	    }
	    for (int k = sizes[ac_array][(run<<4) + j]-1; k >= 0; k--) {
	      bitwriter(&counter, &buffer, (bool) ((codes[ac_array][(run<<4) + j] & 1<<k) ? 1 : 0), fp);
	    }
	    if (sout[i] > 0)
	      amplitude = (1<<(j-1)) + (sout[i] - length[j-1]);
	    else
	      amplitude = length[j] + sout[i] - 1;
	    for (int k = j-1; k >= 0; k--) {
	      bitwriter(&counter, &buffer, (bool) ((amplitude & 1<<k) ? 1 : 0), fp);
	    }
	    run = 0;
	    break;
	  }
	}
      }
    }
  }
  gettimeofday(&tv2, 0);

  time = tv2.tv_sec - tv1.tv_sec + 1e-6 * (tv2.tv_usec - tv1.tv_usec);

  printf("Write to file: %f seconds.\n", time);

  finishfile(&counter, &buffer, fp);

  gettimeofday(&tv4, 0);
  time = tv4.tv_sec - tv3.tv_sec + 1e-6 * (tv4.tv_usec - tv3.tv_usec);

  printf("Total: %f seconds.\n", time);

}
