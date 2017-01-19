#define runcount 100
#include "jpeg.c"

/* Takes a bmp image specified at the command line and
 * encodes it in JPEG format and writes to another file
 * specified at the command line */
int main(int argc, char *argv[])
{
    int num_colors = 3;
    char *input = "lena.bmp";
    char *output = "lena.jpg";

    struct timeval tv1, tv2;
    float read_bmp = 0, transform_colorspace = 0, dct = 0, generate_headers = 0, write_file = 0;

    gettimeofday(&tv1, 0);
    for (int i = 0; i < runcount; i++) {
        JPEGTimings timings = convertFile(input, output, num_colors, false);
        read_bmp += timings.read_bmp;
        transform_colorspace += timings.transform_colorspace;
        dct += timings.dct;
        generate_headers += timings.generate_headers;
        write_file += timings.write_file;
    }
    gettimeofday(&tv2, 0);

    double time = tv2.tv_sec - tv1.tv_sec + 1e-6 * (tv2.tv_usec - tv1.tv_usec);
    printf("Average time accoss %d runs: %f seconds.\n", runcount, time / runcount);
    printf("Read BMP: %f seconds.\n", read_bmp / runcount);
    printf("Transform colorspace: %f seconds.\n", transform_colorspace / runcount);
    printf("DCT: %f seconds.\n", dct / runcount);
    printf("Generate headers: %f seconds.\n", generate_headers / runcount);
    printf("Write to file: %f seconds.\n", write_file / runcount);

    compareFiles(output, "lena_golden.jpg");
}

int compareFiles(char *file1, char *file2)
{
    FILE *pFile1, *pFile2;
    long lSize1, lSize2; // file length
    int i = 0;
    char tmp1, tmp2;

    pFile1 = fopen(file1, "r");
    pFile2 = fopen(file2, "r");

    // obtain file size:
    fseek(pFile1, 0, SEEK_END);
    lSize1 = ftell(pFile1);
    rewind(pFile1);

    // obtain file size:
    fseek(pFile2, 0, SEEK_END);
    lSize2 = ftell(pFile2);
    rewind(pFile2);

    if (lSize1 != lSize2)
    {
        printf("File sizes differ, %d vs. %d\n", lSize1, lSize2);
        return;
    }
    for (i = 0; i < lSize1; i++)
    {
        fread(&tmp1, 1, 1, pFile1);
        fread(&tmp2, 1, 1, pFile2);
        if (tmp1 != tmp2)
        {
            printf("%x: tmp1 0x%x != tmp2 0x%x\n", i, tmp1, tmp2);
            return; // report error to caller
        }
    }
    printf("files match!\n");
}