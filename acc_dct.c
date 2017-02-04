void run_dct(int width, int height, float *quant, float *input, int32_t *output)
{
  #pragma acc parallel pcopyin(quant[:64]) pcopyin(input[:width*height]) copyout(output[:width*height])
  {
    #pragma acc for collapse(4)
    for (int brow = 0; brow < height / 8; brow++)
    {
      for (int bcol = 0; bcol < width / 8; bcol++)
      {
        for (int u = 0; u < 8; u++)
        {
          for (int v = 0; v < 8; v++)
          {
            float temp = 0;
            #pragma acc for reduction(+:temp) collapse(2)
            for (int x = 0; x < 8; x++)
            {
              for (int y = 0; y < 8; y++)
              {
                int input_pointer = bcol * 8 + brow * 8 * width + x + (y * width);
                float input_value = input[input_pointer];
                float cosux = cos((2.0 * x + 1.0) * u * PI / 16.0);
                float cosvy = cos((2.0 * y + 1.0) * v * PI / 16.0);
                temp += (input_value - 128.0) * cosux * cosvy;
              }
            }
            float quant_val = quant[u + v * 8];
            int output_pointer = (bcol + brow * (width / 8)) * 64 + u + v * 8;
            float au = (u == 0 ? sqrt(1.0 / 2.0) : 1);
            float av = (v == 0 ? sqrt(1.0 / 2.0) : 1);
            output[output_pointer] = round((0.25 * au * av * temp) / quant_val);
          }
        }
      }
    }
  }
}