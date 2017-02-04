int length[10] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512};

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
