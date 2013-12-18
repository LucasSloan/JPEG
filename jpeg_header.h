int zigzag[64] = {0,  1,  8, 16,  9,  2,  3, 10,
		  17, 24, 32, 25, 18, 11,  4,  5,
		  12, 19, 26, 33, 40, 48, 41, 34,
		  27, 20, 13,  6,  7, 14, 21, 28,
		  35, 42, 49, 56, 57, 50, 43, 36,
		  29, 22, 15, 23, 30, 37, 44, 51,
		  58, 59, 52, 45, 38, 31, 39, 46,
		  53, 60, 61, 54, 47, 55, 62, 63};

void output_header(FILE* fp, int height, int width, int num_colors, uint16_t** codes, uint8_t** sizes) {
  compute_huffman_table(codes[0], sizes[0], s_ac_lum_bits, s_ac_lum_val);
  compute_huffman_table(codes[1], sizes[1], s_dc_lum_bits, s_dc_lum_val);

  if (num_colors > 1) {
    compute_huffman_table(codes[2], sizes[2], s_ac_chroma_bits, s_ac_chroma_val);
    compute_huffman_table(codes[3], sizes[3], s_dc_chroma_bits, s_dc_chroma_val);
  }


  SOI soi;

  soi.SOI[0] = 0xff;
  soi.SOI[1] = 0xd8;

  fwrite(&soi, sizeof(SOI), 1, fp);

  JFIFHEAD header;

  header.APP0[0] = 0xff;
  header.APP0[1] = 0xe0;
  header.Length[0] = 0x00;
  header.Length[1] = 0x10;
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

  DQTH dqt;

  dqt.marker = 0xff;
  dqt.type = 0xdb;
  dqt.length[0] = 0x00;
  dqt.length[1] = 67 + ((num_colors > 1) ? 65 : 0);
  
  DQTT dqtl;

  dqtl.preid = 0x00;
  for (int i = 0; i < 64; i++) {
    dqtl.table[i] = s_std_lum_quant[zigzag[i]];
  }

  fwrite(&dqt, sizeof(DQTH), 1, fp);
  fwrite(&dqtl, sizeof(DQTT), 1, fp);

  if (num_colors > 1) {
    DQTT dqtc;
    dqtc.preid = 0x01;
    for (int i = 0; i < 64; i++) {
      dqtc.table[i] = s_std_croma_quant[zigzag[i]];
    }
    fwrite(&dqtc, sizeof(DQTT), 1, fp);
  }

  SOF sof;

  sof.marker = 0xff;
  sof.type = 0xc0;
  sof.length[0] = 0x00;
  sof.length[1] = 8 + num_colors*3;
  sof.precision = 8;
  sof.height[0] = height >> 8;
  sof.height[1] = height & 255;
  sof.width[0] = width >> 8;
  sof.width[1] = width & 255;
  sof.components = num_colors;

  fwrite(&sof, sizeof(SOF), 1, fp);

  FComp Yframe;

  Yframe.id = 1;
  Yframe.sampling = 0x11;
  Yframe.quant_table = 0;

  fwrite(&Yframe, sizeof(FComp), 1, fp);

  if (num_colors > 1) {

    FComp Cbframe;

    Cbframe.id = 2;
    Cbframe.sampling = 0x11;
    Cbframe.quant_table = 1;

    fwrite(&Cbframe, sizeof(FComp), 1, fp);

    FComp Crframe;

    Crframe.id = 3;
    Crframe.sampling = 0x11;
    Crframe.quant_table = 1;

    fwrite(&Crframe, sizeof(FComp), 1, fp);
  }

  HuffT dcl;

  int dcl_size = 0;

  dcl.classid = 0x00;
  for (int i = 0; i < 16; i++) {
    dcl.codelengths[i] = s_dc_lum_bits[i+1];
    dcl_size += s_dc_lum_bits[i+1];
  }

  HuffT acl;

  int acl_size = 0;

  acl.classid = 0x10;
  for (int i = 0; i < 16; i++) {
    acl.codelengths[i] = s_ac_lum_bits[i+1];
    acl_size += s_ac_lum_bits[i+1];
  }

  int dcc_size = 0;
  int acc_size = 0;
  HuffT dcc;
  HuffT acc;

  if (num_colors > 1) {

    dcc.classid = 0x01;
    for (int i = 0; i < 16; i++) {
      dcc.codelengths[i] = s_dc_chroma_bits[i+1];
      dcc_size += s_dc_chroma_bits[i+1];
    }

    acc.classid = 0x11;
    for (int i = 0; i < 16; i++) {
      acc.codelengths[i] = s_ac_chroma_bits[i+1];
      acc_size += s_ac_chroma_bits[i+1];
    }
  }

  HUFFMANHead huffhead;

  huffhead.marker = 0xff;
  huffhead.type = 0xc4;
  int hlength = 2 + dcl_size + acl_size + dcc_size + acc_size + 17 * ((num_colors > 1) ? 4 : 2);
  huffhead.length[0] = hlength >> 8;
  huffhead.length[1] = hlength & 0xff;

  fwrite(&huffhead, sizeof(HUFFMANHead), 1, fp);
  fwrite(&dcl, sizeof(HuffT), 1, fp);
  fwrite(&s_dc_lum_val, sizeof(uint8_t), dcl_size, fp);
  fwrite(&acl, sizeof(HuffT), 1, fp);
  fwrite(&s_ac_lum_val, sizeof(uint8_t), acl_size, fp);
  if (num_colors > 1) {
    fwrite(&dcc, sizeof(HuffT), 1, fp);
    fwrite(&s_dc_chroma_val, sizeof(uint8_t), dcc_size, fp);
    fwrite(&acc, sizeof(HuffT), 1, fp);
    fwrite(&s_ac_chroma_val, sizeof(uint8_t), acc_size, fp);
  }


  SOS sos;

  sos.marker = 0xff;
  sos.type = 0xda;
  sos.length[0] = 0x00;
  sos.length[1] = 6+2*num_colors;
  sos.components = num_colors;

  fwrite(&sos, sizeof(SOS), 1, fp);

  SComp Yscan;

  Yscan.id = 1;
  Yscan.huff_table = 0x00;

  fwrite(&Yscan, sizeof(SComp), 1, fp);

  if (num_colors > 1) {
    SComp Cbscan;

    Cbscan.id = 2;
    Cbscan.huff_table = 0x11;

    fwrite(&Cbscan, sizeof(SComp), 1, fp);

    SComp Crscan;

    Crscan.id = 3;
    Crscan.huff_table = 0x11;

    fwrite(&Crscan, sizeof(SComp), 1, fp);
  }

  int buffer = 0;
  fwrite(&buffer, sizeof(uint8_t), 1, fp);
  buffer = 63;
  fwrite(&buffer, sizeof(uint8_t), 1, fp);
  buffer = 0;
  fwrite(&buffer, sizeof(uint8_t), 1, fp);
}
