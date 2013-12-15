#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "huffman.c"
#include "jpeg.h"
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

 int height = 8, width = 8;

 uint16_t* accodes = malloc(sizeof(uint16_t) * 256);
 uint16_t* dccodes = malloc(sizeof(uint16_t) * 256);

 uint8_t* acsizes = malloc(sizeof(uint8_t) * 256);
 uint8_t* dcsizes = malloc(sizeof(uint8_t) * 256);

 compute_huffman_table(accodes, acsizes, s_ac_lum_bits, s_ac_lum_val);
 compute_huffman_table(dccodes, dcsizes, s_dc_lum_bits, s_dc_lum_val);

 SOI soi;

 soi.SOI[0] = 0xff;
 soi.SOI[1] = 0xd8;

 JFIFHEAD header;

 header.APP0[0] = 0xff;
 header.APP0[1] = 0xe0;
 header.Length[0] = 0x00;
 header.Length[1] = 0x10;//16;
 header.Identifier[0] = 0x4a;
 header.Identifier[1] = 0x46;
 header.Identifier[2] = 0x49;
 header.Identifier[3] = 0x46;
 header.Identifier[4] = 0x00;
 header.Version[0] = 0x01;
 header.Version[1] = 0x01;
 header.Units = 0x00;
 header.Xdensity[0] = 0x00;
 header.Xdensity[1] = 0x01;
 header.Ydensity[0] = 0x00;
 header.Ydensity[1] = 0x01;
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
 sof.height[0] = 0x00;//height;
 sof.height[1] = 0x08;
 sof.width[0] = 0x00;//width;
 sof.width[1] = 0x08;
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
              temp += au * av * Y[(row+x) * width + col+y] * cos(PI/8.0*(x+0.5)*rrow) * cos(PI/8.0*(y+0.5)*ccol);
	    }
	  }
	out[(row+rrow) * width + col + ccol] = temp;
	}
      }
    }
  }

  /*for (int i = 0; i < 64; i++) {
    printf("%f\n", out[i]);
    }*/

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

  /*for (int i = 0; i < 64; i++) {
    printf("%d\n", sout[i]);
    }*/

  int length[10] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512};
  int run = 0;
  int fuck;
  int counter = 7;
  uint8_t buffer = 0;
  int lastdc = 0;

  fp = fopen(argv[1], "wb");
  fwrite(&soi, sizeof(SOI), 1, fp);
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
  buffer = 63;
  fwrite(&buffer, sizeof(uint8_t), 1, fp);
  buffer = 0;
  fwrite(&buffer, sizeof(uint8_t), 1, fp);

  for (int z = 0; z < 1; z++) {
  for (int j = 1; j < 11; j++) {
    if ((sout[0] - lastdc) < length[j] && (sout[0] - lastdc) > -length[j]) {
      printf("(%d, %d)(%d); ", run, j, (sout[0] - lastdc));
      for (int k = dcsizes[j]-1; k >= 0; k--) {
        printf("%d", (dccodes[j] & 1<<k)>>k);
        //bitwriter(&counter, &buffer, (bool) (dccodes[j] & 1<<k)>>k, fp);
      }
      printf("\n");
      for (int k = dcsizes[j]-1; k >= 0; k--) {
        //printf("%d", (dccodes[j] & 1<<k)>>k);
        bitwriter(&counter, &buffer, (bool) ((dccodes[j] & 1<<k) ? 1 : 0), fp);
      }
      if ((sout[0] - lastdc) > 0)
	fuck = length[j] - (sout[0] - lastdc);
      else
	fuck = length[j] + (sout[0] - lastdc) - 1;
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
      lastdc = sout[0];
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
          printf("%d", (accodes[0x00] & 1<<k)>>k);
	  //bitwriter(&counter, &buffer, (bool) ((accodes[0x00] & 1<<k) ? 1 : 0), fp);
	}
	printf("\n");
	for (int k = acsizes[0x00]-1; k >= 0; k--) {
          //printf("%d", (accodes[0x00] & 1<<k)>>k);
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
	    //bitwriter(&counter, &buffer, (bool) (accodes[0xf0] & 1<<k)>>k, fp);
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
	  fuck = (1<<(j-1)) + (sout[i] - length[j-1]);//length[j] - sout[i];
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
  //flushbuffer(&counter, &buffer, fp);
  }

  finishfile(&counter, &buffer, fp);
}
