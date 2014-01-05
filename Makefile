all:
	gcc -std=c99 jpeg.c -lm -fopenmp -march=native -O3 -o jpeg

beta:
	gcc -std=c99 jpeg_no_quant.c -fopenmp -lm -w -march=native -O3 -o jpeg_beta

debug:
	gcc -std=c99 jpeg.c -g -lm -fopenmp -march=native -o jpeg

dct:
	gcc -std=c99 DCT.c -lm -o DCT

writer:
	gcc -std=c99 jpeg_new_writer.c -fopenmp -lm -w -march=native -O3 -o jpeg_beta

wtest:
	gcc -std=c99 mulitbitwritertest.c -o mbtest
