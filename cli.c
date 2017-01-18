#include "jpeg.c"

/* Takes a bmp image specified at the command line and
 * encodes it in JPEG format and writes to another file
 * specified at the command line */
int main(int argc, char *argv[])
{
    /* Command line switch for color and greyscale images */
    int num_colors = 3;
    if (argc == 4 && strcmp(argv[3], "-gs") == 0)
        num_colors = 1;

    char *input = argv[1];
    char *output = argv[2];

    convertFile(input, output, num_colors, true);
}
