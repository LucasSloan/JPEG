void run_dct(int width, int height, float *quant, float *input, int32_t *output)
{
  float acosvals[8][8];

  /* Calculating cosines is expensive, and there
   * are only 64 cosines that need to be calculated
   * so precompute them and cache. */
  for (int i = 0; i < 8; i++)
  {
    for (int j = 0; j < 8; j++)
    {
      if (j == 0)
      {
        acosvals[i][j] = sqrt(1.0 / 8.0) * cos(PI / 8.0 * (i + 0.5d) * j);
      }
      else
      {
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
    __m256 avxcosloader, avxcos;
    float avxcosmover;
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

            avxcosloader = _mm256_loadu_ps(&acosvals[y][0]);

            avxcosmover = avxcosloader[0];
            avxcos = _mm256_set1_ps(avxcosmover);
            temp = _mm256_mul_ps(loaderlow, avxcos);
            row0 = _mm256_add_ps(row0, temp);
            temp = _mm256_mul_ps(loaderhigh, avxcos);
            row0 = _mm256_add_ps(row0, temp);

            avxcosmover = avxcosloader[1];
            avxcos = _mm256_set1_ps(avxcosmover);
            temp = _mm256_mul_ps(loaderlow, avxcos);
            row1 = _mm256_add_ps(row1, temp);
            temp = _mm256_mul_ps(loaderhigh, avxcos);
            row1 = _mm256_sub_ps(row1, temp);

            avxcosmover = avxcosloader[2];
            avxcos = _mm256_set1_ps(avxcosmover);
            temp = _mm256_mul_ps(loaderlow, avxcos);
            row2 = _mm256_add_ps(row2, temp);
            temp = _mm256_mul_ps(loaderhigh, avxcos);
            row2 = _mm256_add_ps(row2, temp);

            avxcosmover = avxcosloader[3];
            avxcos = _mm256_set1_ps(avxcosmover);
            temp = _mm256_mul_ps(loaderlow, avxcos);
            row3 = _mm256_add_ps(row3, temp);
            temp = _mm256_mul_ps(loaderhigh, avxcos);
            row3 = _mm256_sub_ps(row3, temp);

            avxcosmover = avxcosloader[4];
            avxcos = _mm256_set1_ps(avxcosmover);
            temp = _mm256_mul_ps(loaderlow, avxcos);
            row4 = _mm256_add_ps(row4, temp);
            temp = _mm256_mul_ps(loaderhigh, avxcos);
            row4 = _mm256_add_ps(row4, temp);

            avxcosmover = avxcosloader[5];
            avxcos = _mm256_set1_ps(avxcosmover);
            temp = _mm256_mul_ps(loaderlow, avxcos);
            row5 = _mm256_add_ps(row5, temp);
            temp = _mm256_mul_ps(loaderhigh, avxcos);
            row5 = _mm256_sub_ps(row5, temp);

            avxcosmover = avxcosloader[6];
            avxcos = _mm256_set1_ps(avxcosmover);
            temp = _mm256_mul_ps(loaderlow, avxcos);
            row6 = _mm256_add_ps(row6, temp);
            temp = _mm256_mul_ps(loaderhigh, avxcos);
            row6 = _mm256_add_ps(row6, temp);

            avxcosmover = avxcosloader[7];
            avxcos = _mm256_set1_ps(avxcosmover);
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