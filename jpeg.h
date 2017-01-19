int zigzag[64] = {0, 1, 8, 16, 9, 2, 3, 10,
                  17, 24, 32, 25, 18, 11, 4, 5,
                  12, 19, 26, 33, 40, 48, 41, 34,
                  27, 20, 13, 6, 7, 14, 21, 28,
                  35, 42, 49, 56, 57, 50, 43, 36,
                  29, 22, 15, 23, 30, 37, 44, 51,
                  58, 59, 52, 45, 38, 31, 39, 46,
                  53, 60, 61, 54, 47, 55, 62, 63};

enum
{
  M_SOF0 = 0xC0,
  M_DHT = 0xC4,
  M_SOI = 0xD8,
  M_EOI = 0xD9,
  M_SOS = 0xDA,
  M_DQT = 0xDB,
  M_APP0 = 0xE0
};
enum
{
  DC_LUM_CODES = 12,
  AC_LUM_CODES = 256,
  DC_CHROMA_CODES = 12,
  AC_CHROMA_CODES = 256,
  MAX_HUFF_SYMBOLS = 257,
  MAX_HUFF_CODESIZE = 32
};

static int16_t s_std_lum_quant[64] = {16, 11, 12, 14, 12, 10, 16, 14, 13, 14, 18, 17, 16, 19, 24, 40, 26, 24, 22, 22, 24, 49, 35, 37, 29, 40, 58, 51, 61, 60, 57, 51, 56, 55, 64, 72, 92, 78, 64, 68, 87, 69, 55, 56, 80, 109, 81, 87, 95, 98, 103, 104, 103, 62, 77, 113, 121, 112, 100, 120, 92, 101, 103, 99};
static int16_t s_std_croma_quant[64] = {17, 18, 18, 24, 21, 24, 47, 26, 26, 47, 99, 66, 56, 66, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99};
static uint8_t s_dc_lum_bits[17] = {0, 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0};
static uint8_t s_dc_lum_val[DC_LUM_CODES] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
static uint8_t s_ac_lum_bits[17] = {0, 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d};
static uint8_t s_ac_lum_val[AC_LUM_CODES] =
    {
        0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08, 0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
        0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
        0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
        0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
        0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
        0xf9, 0xfa};
static uint8_t s_dc_chroma_bits[17] = {0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0};
static uint8_t s_dc_chroma_val[DC_CHROMA_CODES] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
static uint8_t s_ac_chroma_bits[17] = {0, 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77};
static uint8_t s_ac_chroma_val[AC_CHROMA_CODES] =
    {
        0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71, 0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91, 0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
        0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34, 0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
        0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
        0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
        0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
        0xf9, 0xfa};

/* Takes huffman tables stored in JPEG standard format and generates code
   * and size look up tables.
   *
   * Taken from https://code.google.com/p/jpeg-compressor/
   *
   * codes - pointer to code look up table
   * code_sizes - pointer to size look up table
   * bits - pointer to number of codes of each size
   * val - pointer to encodable bytes in order of increasing notional size */
void compute_huffman_table(uint16_t *codes, uint8_t *code_sizes, uint8_t *bits, uint8_t *val)
{
  int i, l, last_p, si;
  uint8_t huff_size[257];
  uint16_t huff_code[257];
  uint16_t code;

  int p = 0;
  for (l = 1; l <= 16; l++)
    for (i = 1; i <= bits[l]; i++)
      huff_size[p++] = (char)l;

  huff_size[p] = 0;
  last_p = p; // write sentinel

  code = 0;
  si = huff_size[0];
  p = 0;

  while (huff_size[p])
  {
    while (huff_size[p] == si)
      huff_code[p++] = code++;
    code <<= 1;
    si++;
  }

  memset(codes, 0, sizeof(codes[0]) * 256);
  memset(code_sizes, 0, sizeof(code_sizes[0]) * 256);
  for (p = 0; p < last_p; p++)
  {
    codes[val[p]] = huff_code[p];
    code_sizes[val[p]] = huff_size[p];
  }
}

typedef struct _SOI
{
  uint8_t SOI[2];
} SOI;

typedef struct _JFIFHeader
{
  uint8_t APP0[2];       /* 02h  Application Use Marker    */
  uint8_t Length[2];     /* 04h  Length of APP0 Field      */
  uint8_t Identifier[5]; /* 06h  "JFIF" (zero terminated) Id String */
  uint8_t Version[2];    /* 07h  JFIF Format Revision      */
  uint8_t Units;         /* 09h  Units used for Resolution */
  uint8_t Xdensity[2];   /* 0Ah  Horizontal Resolution     */
  uint8_t Ydensity[2];   /* 0Ch  Vertical Resolution       */
  uint8_t XThumbnail;    /* 0Eh  Horizontal Pixel Count    */
  uint8_t YThumbnail;    /* 0Fh  Vertical Pixel Count      */
} JFIFHEAD;

/*
SOI = 0xFF, 0xD8
APP0 = 0xFF, 0xE0
Length = 16
Identifier = "JFIF\null"
Version = 0x01, 0x02
units = 0x0
xdensity = x res
ydensity = y res
xthumb = 0
ythumb = 0
*/

typedef struct _DQTHeader
{
  uint8_t marker;
  uint8_t type;
  uint8_t length[2];
} DQTH;

typedef struct _DQTTable
{
  uint8_t preid;
  uint8_t table[64];
} DQTT;

/*
marker = 0xff
type = 0xdb
length = 69
preid = upper 4: 8, lower four = table id
table = quantisation table
*/

typedef struct _HUFFMANHead
{
  uint8_t marker;
  uint8_t type;
  uint8_t length[2];
  //uint8_t classid;
  //uint8_t codelengths[16];
} HUFFMANHead;

typedef struct _HUFFMANTable
{
  uint8_t classid;
  uint8_t codelengths[16];
} HuffT;

/*
marker = 0xff
type = 0xc4
length = 21 + num of codes
classid = upper 4: 0 if DC, 1 if AC; lower four table id
codelengths = number of codes of length N

follow with array of bytes to be encoded
*/

typedef struct _STARTofFrame
{
  uint8_t marker;
  uint8_t type;
  uint8_t length[2];
  uint8_t precision;
  uint8_t height[2];
  uint8_t width[2];
  uint8_t components;
} SOF;

/*
marker = 0xff
type = 0xc0
length = 8 + 3 * components
precision = 8
height = y res
width = x res
components = 1 greyscale or 3 color
*/

typedef struct _FRAMEComponent
{
  uint8_t id;
  uint8_t sampling;
  uint8_t quant_table;
} FComp;

/*
id = 1 Y, 2 Cb, 3 Cr
sampling = sampling factors = 0x44
quant_table = number of quant table
*/

typedef struct _STARTOfScan
{
  uint8_t marker;
  uint8_t type;
  uint8_t length[2];
  uint8_t components;
} SOS;

/*
marker = 0xff
type = 0xda
length = 6 + 2 * components
components = 1 greyscale, 3 color

must be followed by SComp structs
*/

typedef struct _SCANComponent
{
  uint8_t id;
  uint8_t huff_table;
} SComp;

/*
id = 1 Y, 2 Cb, 3 Cr
huff_table = upper 4 DC table, lower 4 AC table
*/

typedef struct _ENDOfImage
{
  uint8_t marker;
  uint8_t id;
} EOI;

/*
marker = 0xff
id = 0xd9
*/

void bitwriter(int *counter, uint8_t *buffer, bool bit, FILE *fp)
{
  if (bit)
  {
    *buffer = *buffer + (1 << *counter);
  }
  if (*counter == 0)
  {
    fwrite(buffer, sizeof(uint8_t), 1, fp);
    if (*buffer == 0xff)
    {
      *buffer = 0;
      fwrite(buffer, sizeof(uint8_t), 1, fp);
    }
    *buffer = 0;
    *counter = 8;
  }
  *counter = *counter - 1;
}

/* At a high level, writes sub-byte quantities to a file.
 * Takes a two byte input and a size and adds size bits from
 * input and puts them on a buffer.  When the buffer fills up,
 * the buffer is written to a file. JPEG file format uses bytes of
 * 255 to signal the start of reserved segment, so bytes of 255
 * must be followed by bytes of 0. An 8 bit buffer is also used,
 * as most calls to this function are of size less than or equal to
 * 3, so the critical path can be sped up.
 *
 * counter - pointer to an integer keeping track of where in the buffer we are
 * buffer - pointer to the buffer
 * input - the two byte input from which bits are added
 * size - the number of bits to be added
 * temp - an 8 bit buffer used to speed up small writes
 * fp - pointer to the file we're writing to */
void multibitwriter(int *counter, uint8_t *buffer, uint16_t input, uint8_t size, uint8_t *temp, FILE *fp)
{
  if (size < (*counter % 8 == 0 ? 8 : *counter % 8))
  {
    *temp = *temp + (input << ((*counter % 8 == 0 ? 8 : *counter % 8) - size));
    *counter = *counter - size;
    return;
  }
  else if (size == (*counter % 8 == 0 ? 8 : *counter % 8))
  {
    *temp = *temp + (input << ((*counter % 8 == 0 ? 8 : *counter % 8) - size));
    buffer[(buffersize + bufferoverflow) - ((*counter % 8 == 0) ? *counter / 8 : *counter / 8 + 1)] = *temp;
    *counter = *counter - size;
    if (*temp == 0xff)
    {
      *counter = *counter - 8;
      buffer[(buffersize + bufferoverflow) - ((*counter % 8 == 0) ? *counter / 8 : *counter / 8 + 1)] = 0x00;
    }
    *temp = 0;
  }
  else
  {
    *temp += (input >> (size - (*counter % 8 == 0 ? 8 : *counter % 8)));
    buffer[(buffersize + bufferoverflow) - ((*counter % 8 == 0) ? *counter / 8 : *counter / 8 + 1)] = *temp;
    if (*temp == 0xff)
    {
      *counter = *counter - 8;
      buffer[(buffersize + bufferoverflow) - ((*counter % 8 == 0) ? *counter / 8 : *counter / 8 + 1)] = 0x00;
    }
    input = input & ~(0xffff << (size - (*counter % 8 == 0 ? 8 : *counter % 8)));
    size = size - (*counter % 8 == 0 ? 8 : *counter % 8);
    *counter = *counter - (*counter % 8 == 0 ? 8 : *counter % 8);
    if (size < 8)
    {
      *temp = input << (8 - size);
      *counter -= size;
    }
    else if (size == 8)
    {
      *temp = input << (8 - size);
      buffer[(buffersize + bufferoverflow) - ((*counter % 8 == 0) ? *counter / 8 : *counter / 8 + 1)] = *temp;
      if (*temp == 0xff)
      {
        *counter = *counter - 8;
        buffer[(buffersize + bufferoverflow) - ((*counter % 8 == 0) ? *counter / 8 : *counter / 8 + 1)] = 0x00;
      }
      *counter -= size;
      *temp = 0;
    }
    else
    {
      *temp = input >> (size - 8);
      buffer[(buffersize + bufferoverflow) - ((*counter % 8 == 0) ? *counter / 8 : *counter / 8 + 1)] = *temp;
      if (*temp == 0xff)
      {
        *counter = *counter - 8;
        buffer[(buffersize + bufferoverflow) - ((*counter % 8 == 0) ? *counter / 8 : *counter / 8 + 1)] = 0x00;
      }
      input = input & ~(0xffff << (size - 8));
      size -= 8;
      *temp = input << (8 - size);
      buffer[((buffersize + bufferoverflow) + 1) - ((*counter % 8 == 0) ? *counter / 8 : *counter / 8 + 1)] = *temp;
      if (*temp == 0xff)
      {
        *counter = *counter - 8;
        buffer[((buffersize + bufferoverflow) + 1) - ((*counter % 8 == 0) ? *counter / 8 : *counter / 8 + 1)] = 0x00;
      }
      *counter = *counter - 8 - size;
    }
  }

  if ((*counter) <= (bufferoverflow * 8))
  {
    fwrite(buffer, sizeof(uint8_t), buffersize, fp);
    for (int i = 0; i < bufferoverflow; i++)
      buffer[i] = buffer[i + buffersize];
    for (int i = bufferoverflow; i < (buffersize + bufferoverflow); i++)
      buffer[i] = 0;
    *counter += buffersize * 8;
  }
}

/* Finishes a file by flushing the remaining bits on the 
 * buffer, then writing an end of file marker, then closing
 * the file pointer.
 *
 * counter - pointer to integer storing where in the buffer we are
 * buffer - pointer to the write buffer
 * temp - byte buffer, might need to be written out
 * fp - pointer to the file being written to. */
void finishfilemulti(int *counter, uint8_t *buffer, uint8_t temp, FILE *fp)
{
  EOI eoi;
  eoi.marker = 0xff;
  eoi.id = 0xd9;

  /* Make sure the byte buffer's in the buffer */
  buffer[(buffersize + bufferoverflow) - ((*counter % 8 == 0) ? *counter / 8 : *counter / 8 + 1)] = temp;

  if (*counter != ((buffersize + bufferoverflow) * 8))
  {
    fwrite(buffer, sizeof(uint8_t), (buffersize + bufferoverflow) - *counter / 8, fp);
  }

  fwrite(&eoi, sizeof(EOI), 1, fp);

  fclose(fp);
}

/* JPEG has a standard file format, with a header defining
 * the file type, the size of the image, the quantization tables
 * used to round the color values, and the huffman tables used to
 * store the information in a variable length code.  Function also
 * initializes the huffman code and size table used in the encoding
 * of the image data.
 *
 * fp - pointer to the file to write to
 * height - the height of the image
 * width - the width of the image
 * num_colors - 1 if greyscale, 3 if color
 * codes - array of arrays of huffman codes
 * sizes - array of arrays of sizes of corresponding codes */
void output_header(FILE *fp, int height, int width, int num_colors, uint16_t **codes, uint8_t **sizes)
{
  compute_huffman_table(codes[0], sizes[0], s_ac_lum_bits, s_ac_lum_val);
  compute_huffman_table(codes[1], sizes[1], s_dc_lum_bits, s_dc_lum_val);

  if (num_colors > 1)
  {
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
  for (int i = 0; i < 64; i++)
  {
    dqtl.table[i] = s_std_lum_quant[i];
  }

  fwrite(&dqt, sizeof(DQTH), 1, fp);
  fwrite(&dqtl, sizeof(DQTT), 1, fp);

  if (num_colors > 1)
  {
    DQTT dqtc;
    dqtc.preid = 0x01;
    for (int i = 0; i < 64; i++)
    {
      dqtc.table[i] = s_std_croma_quant[i];
    }
    fwrite(&dqtc, sizeof(DQTT), 1, fp);
  }

  SOF sof;

  sof.marker = 0xff;
  sof.type = 0xc0;
  sof.length[0] = 0x00;
  sof.length[1] = 8 + num_colors * 3;
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

  if (num_colors > 1)
  {

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
  for (int i = 0; i < 16; i++)
  {
    dcl.codelengths[i] = s_dc_lum_bits[i + 1];
    dcl_size += s_dc_lum_bits[i + 1];
  }

  HuffT acl;

  int acl_size = 0;

  acl.classid = 0x10;
  for (int i = 0; i < 16; i++)
  {
    acl.codelengths[i] = s_ac_lum_bits[i + 1];
    acl_size += s_ac_lum_bits[i + 1];
  }

  int dcc_size = 0;
  int acc_size = 0;
  HuffT dcc;
  HuffT acc;

  if (num_colors > 1)
  {

    dcc.classid = 0x01;
    for (int i = 0; i < 16; i++)
    {
      dcc.codelengths[i] = s_dc_chroma_bits[i + 1];
      dcc_size += s_dc_chroma_bits[i + 1];
    }

    acc.classid = 0x11;
    for (int i = 0; i < 16; i++)
    {
      acc.codelengths[i] = s_ac_chroma_bits[i + 1];
      acc_size += s_ac_chroma_bits[i + 1];
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
  if (num_colors > 1)
  {
    fwrite(&dcc, sizeof(HuffT), 1, fp);
    fwrite(&s_dc_chroma_val, sizeof(uint8_t), dcc_size, fp);
    fwrite(&acc, sizeof(HuffT), 1, fp);
    fwrite(&s_ac_chroma_val, sizeof(uint8_t), acc_size, fp);
  }

  SOS sos;

  sos.marker = 0xff;
  sos.type = 0xda;
  sos.length[0] = 0x00;
  sos.length[1] = 6 + 2 * num_colors;
  sos.components = num_colors;

  fwrite(&sos, sizeof(SOS), 1, fp);

  SComp Yscan;

  Yscan.id = 1;
  Yscan.huff_table = 0x00;

  fwrite(&Yscan, sizeof(SComp), 1, fp);

  if (num_colors > 1)
  {
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
