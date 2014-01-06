#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include "jpeg.h"

int main(int argc, char *argv[]) {
  uint16_t nums[16] = {0x01af, 0x00aa, 0x01af, 0x00ff, 0x01ff, 0x0010, 0x01ff, 0x0010, 0x01ff, 0xffeb, 0x01ff, 0x0010, 0xffeb, 0x0010, 0x01ff, 0x0010};
  uint8_t sizes[16] = {8, 8, 8, 7, 9, 5, 4, 6, 3, 16, 11, 9, 16, 11, 11, 2};
  int counter = 64;
  int ssize = 0;

  FILE* fp = fopen("mtestdump", "wb");

  uint8_t* buffer = malloc(8*sizeof(uint8_t));
  for (int i = 0; i < 8; i++) {
    buffer[i] = 0;
  }

  for (int i = 0; i < 16; i++)
    ssize += sizes[i];

  printf("sum of sizes: %d\n", ssize);

  for (int i = 0; i < 16; i++) {
    multibitwriter(&counter, buffer, (nums[i] & ((1 << (sizes[i]))-1)), sizes[i], fp);
    printf("called!\n");
  }

  finishfilemulti(&counter, buffer, fp);
  free(buffer);
}
