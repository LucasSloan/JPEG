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
#define PI 3.14159265358979323846

int zigzag[64] = {0,  1,  8, 16,  9,  2,  3, 10,
		  17, 24, 32, 25, 18, 11,  4,  5,
		  12, 19, 26, 33, 40, 48, 41, 34,
		  27, 20, 13,  6,  7, 14, 21, 28,
		  35, 42, 49, 56, 57, 50, 43, 36,
		  29, 22, 15, 23, 30, 37, 44, 51,
		  58, 59, 52, 45, 38, 31, 39, 46,
		  53, 60, 61, 54, 47, 55, 62, 63};
int quant[64] = {16 , 11 , 10 , 16 , 24 , 40 , 51 , 61 , 12 , 12 , 14 , 19 , 26 , 58 , 60 , 55 , 14 , 13 , 16 , 24 , 40 , 57 , 69 , 56 , 14 , 17 , 22 , 29 , 51 , 87 , 80 , 62 , 18 , 22 , 37 , 56 , 68 , 109 , 103 , 77 , 24 , 35 , 55 , 64 , 81 , 104 , 113 , 92 , 49 , 64 , 78 , 87 , 103 , 121 , 120 , 101 , 72 , 92 , 95 , 98 , 112 , 100 , 103 , 99};

int length[10] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512};

int main(int argc, char *argv[]) {
  RGB** argb = (RGB**) malloc(sizeof(RGB*));
  UINT32* width = (UINT32*) malloc(sizeof(UINT32*));
  UINT32* height = (UINT32*) malloc(sizeof(UINT32*));
  FILE *fp;

  fp = fopen(argv[1], "rb");

    if (fp == NULL)
    {
	perror ("Error opening source file");
	return 2;
    }

  readSingleImageBMP(fp, argb, width, height);

  fclose(fp);

  RGB* image = *argb;
  RGB pixel;

  /*for (int row = 0; row < *height; row++) {
    for (int col = 0; col < *width; col++) {
      pixel = image[row * *width + col];
      printf("Red %d, Green %d, Blue %d\n", pixel.red, pixel.green, pixel.blue);
    }
    }*/

  float* Y = malloc(sizeof(float) * *width * *height);
  float* Cb = malloc(sizeof(float) * *width * *height);
  float* Cr = malloc(sizeof(float) * *width * *height);
  float* Ye = malloc(sizeof(float) * *width * *height);
  float* Cbe = malloc(sizeof(float) * *width * *height);
  float* Cre = malloc(sizeof(float) * *width * *height);
  INT8* Yout = malloc(sizeof(INT8) * *width * *height);
  INT8* Cbout = malloc(sizeof(INT8) * *width * *height);
  INT8* Crout = malloc(sizeof(INT8) * *width * *height);

  for (int row = 0; row < *height; row++) {
    for (int col = 0; col < *width; col++) {
      pixel = image[row * *width + col];
      Y[row * *width + col] = 0.299*pixel.red + 0.587*pixel.green + 0.114*pixel.blue;
      Cb[row * *width + col] = 128 - 0.168736*pixel.red - 0.331264*pixel.green + 0.5*pixel.blue;
      Cr[row * *width + col] = 128 + 0.5*pixel.red - 0.418688*pixel.green - 0.081312*pixel.blue;
    }
  }

  /*for (int row = 0; row < *height; row++) {
    for (int col = *width - 1; col < *width; col++) {
      pixel = image[row * *width + col];
      printf("Red %d, Green %d, Blue %d\n", pixel.red, pixel.green, pixel.blue);
      printf("Y %d, Cb %d, Cr %d\n", Y[row * *width + col], Cb[row * *width + col], Cr[row * *width + col]);
    }
    }*/

  for (int row = 0; row < *height; row += 8) {
    for (int col = 0; col < *width; col += 8) {
      for (int rrow = 0;  rrow < 8; rrow++) {
        for (int ccol = 0; ccol < 8; ccol++) {
          float temp1 = 0.0, temp2 = 0.0, temp3 = 0.0;
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
              temp1 += au * av * (Y[(row+x) * *width + col+y]-128) * cos(PI/8.0*(x+0.5)*rrow) * cos(PI/8.0*(y+0.5)*ccol);
	      temp2 += au * av * (Cb[(row+x) * *width + col+y]-128) * cos(PI/8.0*(x+0.5)*rrow) * cos(PI/8.0*(y+0.5)*ccol);
              temp3 += au * av * (Cr[(row+x) * *width + col+y]-128) * cos(PI/8.0*(x+0.5)*rrow) * cos(PI/8.0*(y+0.5)*ccol);
	    }
	  }
	Ye[(row+rrow) * *width + col + ccol] = temp1;
	Cbe[(row+rrow) * *width + col + ccol] = temp2;
	Cre[(row+rrow) * *width + col + ccol] = temp3;
	}
      }
    }
  }

  for (int row = 0; row < *height; row += 8) {
    for (int col = 0; col < *width; col += 8) {
      for (int rrow = 0;  rrow < 8; rrow++) {
        for (int ccol = 0; ccol < 8; ccol++) {
          int pos = zigzag[rrow*8 + ccol];
          Yout[row * *width + col + rrow*8 + ccol] = round(Ye[(row+ pos/8) * *width + col + pos%8] / quant[pos]);
          Cbout[row * *width + col + rrow*8 + ccol] = round(Cbe[(row+ pos/8) * *width + col + pos%8] / quant[pos]);
          Crout[row * *width + col + rrow*8 + ccol] = round(Cre[(row+ pos/8) * *width + col + pos%8] / quant[pos]);
	}
      }
    }
  }

 uint16_t* accodes = malloc(sizeof(uint16_t) * 256);
 uint16_t* dccodes = malloc(sizeof(uint16_t) * 256);

 uint8_t* acsizes = malloc(sizeof(uint8_t) * 256);
 uint8_t* dcsizes = malloc(sizeof(uint8_t) * 256);

 compute_huffman_table(accodes, acsizes, s_ac_lum_bits, s_ac_lum_val);
 compute_huffman_table(dccodes, dcsizes, s_dc_lum_bits, s_dc_lum_val);

 JFIFHEAD header;

 header.SOI[0] = 0xff;
 header.SOI[1] = 0xd8;
 header.APP0[0] = 0xff;
 header.APP0[0] = 0xe0;
 header.Length[0] = 0x00;
 header.Length[1] = 0xa0;//16;
 header.Identifier[0] = 0x4a;
 header.Identifier[1] = 0x46;
 header.Identifier[2] = 0x49;
 header.Identifier[3] = 0x46;
 header.Identifier[4] = 0x00;
 header.Units = 0x00;
 header.Version[0] = 0x01;
 header.Version[1] = 0x02;
 header.Xdensity[0] = 0x00;
 header.Xdensity[1] = 100;
 header.Ydensity[0] = 0x00;
 header.Ydensity[1] = 100;
 header.XThumbnail = 0x00;
 header.YThumbnail = 0x00;

 DQT dqt;

 dqt.marker = 0xff;
 dqt.type = 0xdb;
 dqt.length[0] = 0x00;
 dqt.length[1] = 67;
 dqt.preid = 0x00;
 for (int i = 0; i < 64; i++) {
   dqt.table[i] = quant[zigzag[i]];
 }

 HUFFMANHead huffhead;

 huffhead.marker = 0xff;
 huffhead.type = 0xc4;
 huffhead.length[0] = 0x00;
 huffhead.length[1] = 19+12+162+17;

 HuffT dc;

 dc.classid = 0x00;
 for (int i = 0; i < 16; i++) {
   dc.codelengths[i] = s_dc_lum_bits[i+1];
 }

 //s_dc_lum_val;

 HuffT ac;

 ac.classid = 0x10;
 for (int i = 0; i < 16; i++) {
   ac.codelengths[i] = s_ac_lum_bits[i+1];
 }

 //s_ac_lum_val;

 SOF sof;

 sof.marker = 0xff;
 sof.type = 0xc0;
 sof.length[0] = 0x00;
 sof.length[1] = 11;
 sof.precision = 8;
 sof.height[0] = 0x02;//height;
 sof.height[1] = 0x00;
 sof.width[0] = 0x02;//width;
 sof.width[1] = 0x00;
 sof.components = 1;

 FComp Yframe;

 Yframe.id = 1;
 Yframe.sampling = 0x22;
 Yframe.quant_table = 0;

 SOS sos;

 sos.marker = 0xff;
 sos.type = 0xda;
 sos.length[0] = 0x00;
 sos.length[1] = 8;
 sos.components = 1;

 SComp Yscan;

 Yscan.id = 1;
 Yscan.huff_table = 0x00;

  int run = 0;
  int fuck;
  int counter = 7;
  uint8_t buffer = 0;
  uint8_t* sout;
  uint8_t prevdc = 0;

  fp = fopen(argv[2], "wb");
  fwrite(&header, sizeof(JFIFHEAD), 1, fp);
  fwrite(&dqt, sizeof(DQT), 1, fp);
  fwrite(&sof, sizeof(SOF), 1, fp);
  fwrite(&Yframe, sizeof(FComp), 1, fp);
  fwrite(&huffhead, sizeof(HUFFMANHead), 1, fp);
  fwrite(&dc, sizeof(HuffT), 1, fp);
  fwrite(&s_dc_lum_val, sizeof(uint8_t), 12, fp);
  fwrite(&ac, sizeof(HuffT), 1, fp);
  fwrite(&s_ac_lum_val, sizeof(uint8_t), 162, fp);
  fwrite(&sos, sizeof(SOS), 1, fp);
  fwrite(&Yscan, sizeof(SComp), 1, fp);
  fwrite(&buffer, sizeof(uint8_t), 1, fp);
  fwrite(&buffer, sizeof(uint8_t), 1, fp);
  fwrite(&buffer, sizeof(uint8_t), 1, fp);

  for (int block = 0; block < *width * *height; block += 64) {
    sout = &Yout[block];
    printf("Assigned block pointer\n");
    for (int j = 1; j < 11; j++) {
      if ((prevdc - sout[0]) < length[j] && (prevdc - sout[0]) > -length[j]) {
	printf("(%d, %d)(%d); ", run, j, (prevdc - sout[0]));
	for (int k = dcsizes[j]-1; k >= 0; k--) {
	  printf("%d", (dccodes[j] & 1<<k)>>k);
	  //bitwriter(&counter, &buffer, (bool) (dccodes[j] & 1<<k)>>k, fp);
	}
	printf("\n");
	for (int k = dcsizes[j]-1; k >= 0; k--) {
	  //printf("%d", (dccodes[j] & 1<<k)>>k);
	  bitwriter(&counter, &buffer, (bool) ((dccodes[j] & 1<<k) ? 1 : 0), fp);
	}
	if ((prevdc - sout[0]) > 0)
	  fuck = length[j] - (prevdc - sout[0]);
	else
	  fuck = length[j] + (prevdc - sout[0]) - 1;
	for (int i = j-1; i >= 0; i--) {
	  printf("%d", (fuck & 1<<i)>>i);
	  //bitwriter(&counter, &buffer, (bool) (fuck & 1<<i)>>i, fp);
	}
	printf("\n");
	for (int i = j-1; i >= 0; i--) {
	  //printf("%d", (fuck & 1<<i)>>i);
	  bitwriter(&counter, &buffer, (bool) ((fuck & 1<<i) ? 1 : 0), fp);
	}
	printf("\n");
        prevdc = sout[0];
	break;
      }
    }
    
    /*for (int k = acsizes[0x00]-1; k >= 0; k--) {
      bitwriter(&counter, &buffer, (bool) ((accodes[0x00] & 1<<k) ? 1 : 0), fp);
      }*/
    
    for (int i = 1; i < 64; i++) {
      if (sout[i] == 0) {
	run++;
	if (i == 63 && run > 0) {
	  printf("(%d, %d).\n", 0, 0);
	  for (int k = acsizes[0x00]-1; k >= 0; k--) {
	    bitwriter(&counter, &buffer, (bool) ((accodes[0x00] & 1<<k) ? 1 : 0), fp);
	  }
	}
	continue;
      }
      for (int j = 1; j < 11; j++) {
	if (sout[i] < length[j] && sout[i] > -length[j]) {
	  while (run > 15) {
	    printf("(%d, %d)(%d); ", 15, 0, 0);
	    for (int k = acsizes[0xf0]-1; k >= 0; k--) {
	      bitwriter(&counter, &buffer, (bool) ((accodes[0xf0] & 1<<k) ? 1 : 0), fp);
	    }
	    run -= 16;
	  }
	  printf("(%d, %d)(%d); ", run, j, sout[i]);
	  printf("byte: %d size: %d code: %d\t", (uint8_t) (run<<4)+j,  acsizes[(uint8_t) (run<<4) + j], accodes[(uint8_t) (run<<4) + j]);
	  for (int k = acsizes[(run<<4) + j]-1; k >= 0; k--) {
	    printf("%d", (accodes[(run<<4) + j] & 1<<k)>>k);
	    //bitwriter(&counter, &buffer, (bool) (accodes[(run<<4) + j] & 1<<k)>>k, fp);
	  }
	  printf("\n");
	  for (int k = acsizes[(run<<4) + j]-1; k >= 0; k--) {
	    //printf("%d", (accodes[(run<<4) + j] & 1<<k)>>k);
	    bitwriter(&counter, &buffer, (bool) ((accodes[(run<<4) + j] & 1<<k) ? 1 : 0), fp);
	  }
	  if (sout[i] > 0)
	    fuck = length[j] - sout[i];
	  else
	    fuck = length[j] + sout[i] - 1;
	  for (int k = j-1; k >= 0; k--) {
	    printf("%d", (fuck & 1<<k)>>k);
	    //bitwriter(&counter, &buffer, (bool) (fuck & 1<<k)>>k, fp);
	  }
	  printf("\n");
	  for (int k = j-1; k >= 0; k--) {
	    //printf("%d", (fuck & 1<<k)>>k);
	    bitwriter(&counter, &buffer, (bool) ((fuck & 1<<k) ? 1 : 0), fp);
	  }
	  printf("\n");
	  run = 0;
	  break;
	}
      }
    }
  }

  flushbuffer(&counter, &buffer, fp);
}
