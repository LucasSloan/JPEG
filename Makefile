all:
	gcc -std=c99 jpeg.c -lm -fopenmp -march=native -O3 -o jpeg

debug:
	gcc -std=c99 jpeg.c -g -lm -fopenmp -march=native -o jpeg
