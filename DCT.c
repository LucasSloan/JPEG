#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "huffman.c"
#include "jpeg.h"
#include "jpeg_header.h"
#define PI 3.14159265358979323846


int main(int argc, char *argv[]) {

  int Y[64] = {-76 , -73 , -67 , -62 , -58 , -67 , -64 , -55 , -65 , -69 , -73 , -38 , -19 , -43 , -59 , -56 , -66 , -69 , -60 , -15 , 16 , -24 , -62 , -55 , -65 , -70 , -57 , -6 , 26 , -22 , -58 , -59 , -61 , -67 , -60 , -24 , -2 , -40 , -60 , -58 , -49 , -63 , -68 , -58 , -51 , -60 , -70 , -53 , -43 , -57 , -64 , -69 , -73 , -67 , -63 , -45 , -41 , -49 , -59 , -60 , -63 , -52 , -50 , -34};

int zigzag[64] = {0,  1,  8, 16,  9,  2,  3, 10,
		  17, 24, 32, 25, 18, 11,  4,  5,
		  12, 19, 26, 33, 40, 48, 41, 34,
		  27, 20, 13,  6,  7, 14, 21, 28,
		  35, 42, 49, 56, 57, 50, 43, 36,
		  29, 22, 15, 23, 30, 37, 44, 51,
		  58, 59, 52, 45, 38, 31, 39, 46,
		  53, 60, 61, 54, 47, 55, 62, 63};
int quant[64] = {16 , 11 , 10 , 16 , 24 , 40 , 51 , 61 , 12 , 12 , 14 , 19 , 26 , 58 , 60 , 55 , 14 , 13 , 16 , 24 , 40 , 57 , 69 , 56 , 14 , 17 , 22 , 29 , 51 , 87 , 80 , 62 , 18 , 22 , 37 , 56 , 68 , 109 , 103 , 77 , 24 , 35 , 55 , 64 , 81 , 104 , 113 , 92 , 49 , 64 , 78 , 87 , 103 , 121 , 120 , 101 , 72 , 92 , 95 , 98 , 112 , 100 , 103 , 99};

  FILE *fp;

  int duplication_factor = argc > 2 ? atoi(argv[2]) : 1;

  int height = 8, width = 8;

  float out[64];
  int sout[64];

  for (int row = 0; row < height; row += 8) {
    for (int col = 0; col < width; col += 8) {
      for (int rrow = 0;  rrow < 8; rrow++) {
        for (int ccol = 0; ccol < 8; ccol++) {
          float temp = 0.0;
          float au, av;
          if (rrow == 0) {
            au = sqrt(1.0/8.0);
	  } else {
            au = sqrt(2.0/8.0);
          }
          if (ccol == 0) {
            av = sqrt(1.0/8.0);
	  } else {
            av = sqrt(2.0/8.0);
          }
          for (int x = 0; x < 8; x++) {
            for (int y = 0; y < 8; y++) {
              temp += au * av * Y[(row+y) * width + col+x] * cos(PI/8.0*(x+0.5)*rrow) * cos(PI/8.0*(y+0.5)*ccol);
	    }
	  }
	out[(row+rrow) * width + col + ccol] = temp;
	}
      }
    }
  }

  for (int row = 0; row < height; row += 8) {
    for (int col = 0; col < width; col += 8) {
      for (int rrow = 0;  rrow < 8; rrow++) {
        for (int ccol = 0; ccol < 8; ccol++) {
          int pos = zigzag[rrow*8 + ccol];
          sout[row * width + col + rrow*8 + ccol] = round(out[(row+ pos/8) * width + col + pos%8] / quant[pos]);
	}
      }
    }
  }

  height *= duplication_factor;
  width *= duplication_factor;

  int length[10] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512};
  int run = 0;
  int amplitude;
  int counter = 7;
  uint8_t buffer = 0;
  int lastdc = 0;

  uint16_t* accodes = malloc(sizeof(uint16_t) * 256);
  uint16_t* dccodes = malloc(sizeof(uint16_t) * 256);

  uint8_t* acsizes = malloc(sizeof(uint8_t) * 256);
  uint8_t* dcsizes = malloc(sizeof(uint8_t) * 256);

  fp = fopen(argv[1], "wb");
  output_header(fp, height, width, accodes, dccodes, acsizes, dcsizes);

  for (int z = 0; z < duplication_factor*duplication_factor; z++) {
    for (int j = 0; j < 11; j++) {
      if ((sout[0] - lastdc) < length[j] && (sout[0] - lastdc) > -length[j]) {
	for (int k = dcsizes[j]-1; k >= 0; k--) {
	  bitwriter(&counter, &buffer, (bool) ((dccodes[j] & 1<<k) ? 1 : 0), fp);
	}
	if ((sout[0] - lastdc) > 0)
	  amplitude = (1<<(j-1)) + ((sout[0] - lastdc) - length[j-1]);
	else
	  amplitude = length[j] + (sout[0] - lastdc) - 1;
	for (int i = j-1; i >= 0; i--) {
	  bitwriter(&counter, &buffer, (bool) ((amplitude & 1<<i) ? 1 : 0), fp);
	}
	lastdc = sout[0];
	break;
      }
    }

    for (int i = 1; i < 64; i++) {
      if (sout[i] == 0) {
	run++;
	if (i == 63 && run > 0) {
	  for (int k = acsizes[0x00]-1; k >= 0; k--) {
	    bitwriter(&counter, &buffer, (bool) ((accodes[0x00] & 1<<k) ? 1 : 0), fp);
	  }
          run = 0;
	}
	continue;
      }
      for (int j = 1; j < 11; j++) {
	if (sout[i] < length[j] && sout[i] > -length[j]) {
          if (run == 16)
            printf("Run hit 16\n");
	  while (run > 15) {
	    for (int k = acsizes[0xf0]-1; k >= 0; k--) {
	      bitwriter(&counter, &buffer, (bool) ((accodes[0xf0] & 1<<k) ? 1 : 0), fp);
	    }
            printf("Run before: %d\n", run);
	    run -= 16;
            printf("Run after: %d\n", run);
	  }
	  for (int k = acsizes[(run<<4) + j]-1; k >= 0; k--) {
	    bitwriter(&counter, &buffer, (bool) ((accodes[(run<<4) + j] & 1<<k) ? 1 : 0), fp);
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

  finishfile(&counter, &buffer, fp);
}
