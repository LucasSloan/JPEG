all:
	gcc -std=c99 jpeg.c -lm -o jpeg
	gcc -std=c99 DCT.c -lm -o DCT