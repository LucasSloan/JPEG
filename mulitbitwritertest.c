#define buffersize 1024 //size, in bytes, of the write buffer
#define bufferoverflow 4 //size, in bytes, of the write buffer's overflow
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>
#include "jpeg.h"

int main(int argc, char *argv[]) {
  struct timeval tv1, tv2;
  uint16_t nums[16] = {0x01af, 0x00aa, 0x01af, 0x00ff, 0x01ff, 0x0010, 0x01ff, 0x0010, 0x01ff, 0xffeb, 0x01ff, 0x0010, 0xffeb, 0x0010, 0x01ff, 0x0010};
  uint8_t sizes[16] = {8, 8, 8, 7, 9, 5, 4, 6, 3, 16, 11, 9, 16, 11, 11, 2};
  int counter = 8*(buffersize+bufferoverflow); //initialized to buffer size, in bits
  int scounter = 7;
  int ssize = 0;

  FILE* fp = fopen("mtestdump", "wb");

  uint8_t* buffer = malloc((buffersize+bufferoverflow)*sizeof(uint8_t));
  uint8_t sbuffer = 0;
  for (int i = 0; i < 8; i++) {
    buffer[i] = 0;
  }

  for (int i = 0; i < 16; i++)
    ssize += sizes[i];

  gettimeofday(&tv1, 0);
  for (int times = 0; times < 65536; times++) {
    for (int i = 0; i < 16; i++) {
      for (int k = sizes[i]-1; k >= 0; k--) {
	bitwriter(&scounter, &sbuffer, (bool) ((nums[i] & 1<<k) ? 1 : 0), fp);
      }
    }
  }
  gettimeofday(&tv2, 0);

  double stime = tv2.tv_sec - tv1.tv_sec + 1e-6 * (tv2.tv_usec - tv1.tv_usec);

  printf("Single bit Writer: %f seconds.\n", stime);


  printf("sum of sizes: %d\n", ssize);

  gettimeofday(&tv1, 0);
  for (int times = 0; times < 65536; times++) {
    for (int i = 0; i < 16; i++) {
      multibitwriter(&counter, buffer, (nums[i] & ((1 << (sizes[i]))-1)), sizes[i], fp);
    }
  }
  finishfilemulti(&counter, buffer, fp);
  gettimeofday(&tv2, 0);

  double mtime = tv2.tv_sec - tv1.tv_sec + 1e-6 * (tv2.tv_usec - tv1.tv_usec);

  printf("Multibit Writer: %f seconds.\n", mtime);

  printf("Difference: %f seconds, %f times speed up.\n", stime - mtime, stime/mtime);


  free(buffer);
}
