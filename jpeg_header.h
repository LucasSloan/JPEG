int zigzag[64] = {0,  1,  8, 16,  9,  2,  3, 10,
		  17, 24, 32, 25, 18, 11,  4,  5,
		  12, 19, 26, 33, 40, 48, 41, 34,
		  27, 20, 13,  6,  7, 14, 21, 28,
		  35, 42, 49, 56, 57, 50, 43, 36,
		  29, 22, 15, 23, 30, 37, 44, 51,
		  58, 59, 52, 45, 38, 31, 39, 46,
		  53, 60, 61, 54, 47, 55, 62, 63};
int quant[64] = {16 , 11 , 10 , 16 , 24 , 40 , 51 , 61 , 12 , 12 , 14 , 19 , 26 , 58 , 60 , 55 , 14 , 13 , 16 , 24 , 40 , 57 , 69 , 56 , 14 , 17 , 22 , 29 , 51 , 87 , 80 , 62 , 18 , 22 , 37 , 56 , 68 , 109 , 103 , 77 , 24 , 35 , 55 , 64 , 81 , 104 , 113 , 92 , 49 , 64 , 78 , 87 , 103 , 121 , 120 , 101 , 72 , 92 , 95 , 98 , 112 , 100 , 103 , 99};

void output_header(FILE* fp, int height, int width, uint16_t* accodes, uint16_t* dccodes, uint8_t* acsizes, uint8_t* dcsizes) {
  compute_huffman_table(accodes, acsizes, s_ac_lum_bits, s_ac_lum_val);
  compute_huffman_table(dccodes, dcsizes, s_dc_lum_bits, s_dc_lum_val);

  SOI soi;

  soi.SOI[0] = 0xff;
  soi.SOI[1] = 0xd8;

  fwrite(&soi, sizeof(SOI), 1, fp);

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

  fwrite(&header, sizeof(JFIFHEAD), 1, fp);

  DQT dqt;

  dqt.marker = 0xff;
  dqt.type = 0xdb;
  dqt.length[0] = 0x00;
  dqt.length[1] = 67;
  dqt.preid = 0x00;
  for (int i = 0; i < 64; i++) {
    dqt.table[i] = quant[zigzag[i]];
  }

  fwrite(&dqt, sizeof(DQT), 1, fp);

  SOF sof;

  sof.marker = 0xff;
  sof.type = 0xc0;
  sof.length[0] = 0x00;
  sof.length[1] = 11;
  sof.precision = 8;
  sof.height[0] = height >> 8;
  sof.height[1] = height & 255;
  sof.width[0] = width >> 8;
  sof.width[1] = width & 255;
  sof.components = 1;

  fwrite(&sof, sizeof(SOF), 1, fp);

  FComp Yframe;

  Yframe.id = 1;
  Yframe.sampling = 0x22;
  Yframe.quant_table = 0;

  fwrite(&Yframe, sizeof(FComp), 1, fp);

  HuffT dc;

  int dc_size = 0;

  dc.classid = 0x00;
  for (int i = 0; i < 16; i++) {
    dc.codelengths[i] = s_dc_lum_bits[i+1];
    dc_size += s_dc_lum_bits[i+1];
  }

  HuffT ac;

  int ac_size = 0;

  ac.classid = 0x10;
  for (int i = 0; i < 16; i++) {
    ac.codelengths[i] = s_ac_lum_bits[i+1];
    ac_size += s_ac_lum_bits[i+1];
  }

  HUFFMANHead huffhead;

  huffhead.marker = 0xff;
  huffhead.type = 0xc4;
  huffhead.length[0] = 0x00;
  huffhead.length[1] = 19+17+dc_size+ac_size;

  fwrite(&huffhead, sizeof(HUFFMANHead), 1, fp);
  fwrite(&dc, sizeof(HuffT), 1, fp);
  fwrite(&s_dc_lum_val, sizeof(uint8_t), dc_size, fp);
  fwrite(&ac, sizeof(HuffT), 1, fp);
  fwrite(&s_ac_lum_val, sizeof(uint8_t), ac_size, fp);

  SOS sos;

  sos.marker = 0xff;
  sos.type = 0xda;
  sos.length[0] = 0x00;
  sos.length[1] = 8;
  sos.components = 1;

  fwrite(&sos, sizeof(SOS), 1, fp);

  SComp Yscan;

  Yscan.id = 1;
  Yscan.huff_table = 0x00;

  fwrite(&Yscan, sizeof(SComp), 1, fp);

  int buffer = 0;
  fwrite(&buffer, sizeof(uint8_t), 1, fp);
  buffer = 63;
  fwrite(&buffer, sizeof(uint8_t), 1, fp);
  buffer = 0;
  fwrite(&buffer, sizeof(uint8_t), 1, fp);
}
